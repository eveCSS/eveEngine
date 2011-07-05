/*
 * eveScanManager.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include <QTimer>
#include "eveScanThread.h"
#include "eveScanManager.h"
#include "eveScanModule.h"
#include "eveMessageHub.h"
#include "eveError.h"
#include "eveEventProperty.h"
#include "eveEventRegisterMessage.h"
#include "eveRequestManager.h"

/**
 *
 * @param parser current XML parser object
 * @param chainid our chain id
 *
 */
eveScanManager::eveScanManager(eveManager *parent, eveXMLReader *parser, int chainid, int storageChan) :
	eveMessageChannel(parent)
{
	QDomElement rootElement;

	chainId = chainid;
	storageChannel = storageChan;
	posCounter = 0;
	useStorage = false;
	sentData = false;
	manager = parent;
	shutdownPending = false;
	nextEventId = chainId*100;
	// TODO register global events

	// TODO check startevent

	printf("Constructor: ScanManager with chain %d\n", chainid);
	//parser->addScanManager(chainId, this);
	int rootId = parser->getRootId(chainId);
	if (rootId == 0)
		sendError(ERROR,0,"eveScanManager::eveScanManager: no root scanmodule found");
	else {
		// walk down the tree and create all ScanModules
		rootSM = new eveScanModule(this, parser, chainId, rootId);
		connect (rootSM, SIGNAL(SMready()), this, SLOT(smDone()), Qt::QueuedConnection);
	}

	// register with messageHub
	channelId = eveMessageHub::getmHub()->registerChannel(this, 0);

	connect (manager, SIGNAL(startSMs()), this, SLOT(smStart()), Qt::QueuedConnection);
	connect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()), Qt::QueuedConnection);
	connect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()), Qt::QueuedConnection);
	connect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()), Qt::QueuedConnection);
	connect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()), Qt::QueuedConnection);

	// collect various data
	// TODO never used
	addToHash(&chainHash, "confirmsave", parser);

	// collect all storage-related data
	savePluginHash = parser->getChainPlugin(chainId, "saveplugin");
	addToHash(savePluginHash, "savefilename", parser);
	addToHash(savePluginHash, "autonumber", parser);
	addToHash(savePluginHash, "confirmsave", parser);
	addToHash(savePluginHash, "savescandescription", parser);

	if (savePluginHash->contains("savefilename")) useStorage = true;

	sendStatusTimer = new QTimer(this);
	sendStatusTimer->setInterval(5000);
	sendStatusTimer->setSingleShot(false);
	connect(sendStatusTimer, SIGNAL(timeout()), this, SLOT(sendRemainingTime()));
	sendStatusTimer->start(3000);

}

eveScanManager::~eveScanManager() {
	//delete sendStatusTimer;
}

/**
 * \brief  initialization, called if thread starts
 *
 * connect signals from manager thread
 */
void eveScanManager::init() {

	// init StorageModule if we have a Datafilename
	if (useStorage) {
		// TODO before sending this we must make sure that the corresponding
		// storage channel has been registered with messageHub
		sendMessage(new eveStorageMessage(chainId, channelId, savePluginHash, 0, storageChannel));
	}
	rootSM->initialize();
}

/**
 * \brief shutdown associated chain and this thread
 */
void eveScanManager::shutdown(){

	eveError::log(1, QString("eveScanManager: shutdown"));

	if (!shutdownPending) {
		disconnect (manager, SIGNAL(startSMs()), this, SLOT(smStart()));
		disconnect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()));
		disconnect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()));
		disconnect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()));
		disconnect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()));

		// unregister events, do not delete them
		foreach(eveEventProperty* eprop, eventPropList){
			addMessage(new eveEventRegisterMessage(false, eprop));
		}
		eventPropList.clear();

		// stop input Queue
		disableInput();

		// delete all SMs
		if (rootSM) delete rootSM;
		rootSM = NULL;
	}

	// make sure mHub reads all outstanding messages before closing the channel
	if (unregisterIfQueueIsEmpty()){
		eveError::log(1, QString("eveScanManager: shutdown done"));
		QThread::currentThread()->quit();
	}
	else {
		// connect with messageTaken to call shutdown again until successfully unregistered
		if (!shutdownPending) connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}
	shutdownPending = true;
}


/**
 * \brief  start SM (or continue if previously paused)
 *
 * usually called by a signal from manager thread
 */
void eveScanManager::smStart() {
	sendError(INFO,0,"eveScanManager::smStart: starting root");
	if (rootSM) rootSM->startChain();
}

/**
 * \brief  Slot for halt signal; stop all motors, halt chain
 *
 * Emergency: stop all running motors, set all executing SM to last stage,
 * terminate chain, save data do not do postscan actions.
 *
 *
 */
void eveScanManager::smHalt() {
	if (rootSM) rootSM->haltChain();
}

/**
 * \brief  stop current SM, do postscan actions, resume with the next SM.
 *
 * break after the current step is ready, resume with current postscan
 * do nothing, if we are halted or stopped
 */
void eveScanManager::smBreak() {
	if (rootSM) rootSM->breakChain();
}

/**
 * \brief  stop current SM, do postscan actions, terminate chain, save data
 *
 * stop after the current step is ready, resume with current postscan
 * terminate the chain.
 * do nothing, if we are already halted, clear a break
 */
void eveScanManager::smStop() {
	if (rootSM) rootSM->stopChain();
}

/**
 * \brief Pause current SMs (continue with start command).
 *
 * pause after the current stage is ready, do not stop moving motors
 * do only if we are currently executing
 */
void eveScanManager::smPause() {
	if (rootSM) rootSM->pauseChain();
}


/**
 * \brief signals an redo event.
 *
 */
void eveScanManager::smRedo() {
	if (rootSM) rootSM->redoChain();
}

/**
 * \brief
 *
 */
void eveScanManager::smDone() {

	if (rootSM && rootSM->isDone()){
		currentStatus = eveChainDONE;
		sendStatus(0, 0);
		// call shutdown to end this thread
		//QTimer::singleShot(0, this, SLOT(shutdown()));
		shutdown();
	}
}

void eveScanManager::sendMessage(eveMessage *message){
	// a message with destination zero is sent to all storageModules
	if ((message->getType() == EVEMESSAGETYPE_DATA)
		|| (message->getType() == EVEMESSAGETYPE_DEVINFO)
		|| (message->getType() == EVECHANNEL_STORAGE)) {
		message->setDestination(storageChannel);
	}
	if (message->getType() == EVEMESSAGETYPE_DATA) {
		((eveDataMessage*)message)->setPositionCount(posCounter);
		sentData = true;
	}
	addMessage(message);
}

void eveScanManager::sendRemainingTime(){

	if (currentStatus == eveChainSmEXECUTING){
		int remainTime = rootSM->getRemainingTime();
		if (remainTime > -1) sendStatus(0, remainTime);
	}
}

/**
 * \brief process messages sent to eveScanManager
 * @param message pointer to the message which eveScanManager received
 */
void eveScanManager::handleMessage(eveMessage *message){

	eveError::log(4, "eveScanManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_REQUESTANSWER:
		{
			int rid = ((eveRequestAnswerMessage*)message)->getReqId();
			if (requestHash.contains(rid)) {
				if (rootSM) rootSM->triggerSM(requestHash.value(rid), rid);
				requestHash.remove(rid);
			}
		}
		break;
		default:
			sendError(ERROR,0,"eveScanManager::handleMessage: unknown message");
			break;
	}
	delete message;
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveScanManager::sendError(int severity, int errorType,  QString errorString){
	sendError(severity, EVEMESSAGEFACILITY_SCANCHAIN, errorType, errorString);
}
/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveScanManager::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}

/**
 *
 * @param smid id of scanmodule
 * @param status current chain status
 */
void eveScanManager::setStatus(int smid, smStatusT status){
	// TODO
	// makes not much  sense to have different types smStatusT and chainStatusT
	if (status == eveSmINITIALIZING)
		currentStatus = eveChainSmINITIALIZING;
	else if (status == eveSmNOTSTARTED)
		currentStatus = eveChainSmIDLE;
	else if (status == eveSmEXECUTING)
		currentStatus = eveChainSmEXECUTING;
	else if (status == eveSmPAUSED)
		currentStatus = eveChainSmPAUSED;
	else if (status == eveSmTRIGGERWAIT)
		currentStatus = eveChainSmTRIGGERWAIT;
	else if (status == eveSmAPPEND)
		currentStatus = eveChainSmDONE;
	else if (status == eveSmDONE)
		currentStatus = eveChainSmDONE;

	sendStatus (smid, -1);
}
/**
 *
 * @param smid id of scanmodule
 * @param status current chain status
 * @param remainTime remaining time until scan is done (defaults to -1)
 */
void eveScanManager::sendStatus(int smid, int remainTime){

	// fill in our storage channel
	addMessage(new eveChainStatusMessage(currentStatus, chainId, smid, posCounter, eveTime::getCurrent(), remainTime, 0, storageChannel));
}

void eveScanManager::addToHash(QHash<QString, QString>* hash, QString key, eveXMLReader* parser){

	QString value;
	value = parser->getChainString(chainId, key);
	if (!value.isEmpty()) hash->insert(key, value);

}

/**
 * increment position counter and send next-position-message
 * do not increment position counter if no data has been received
 * between two calls to nextPos
 */
void eveScanManager::nextPos(){
	// send nextPos message
//	if (sentData){
//		sentData = false;
//		++posCounter;
//	}
	++posCounter;
}

/**
 * \brief connect this ScanManagers newEvent-Slot with the EventProperty object and send
 * the EventProperty to EventManager.
 *
 * @param smid
 * @param evproperty the eveEventProperty Object to be registered
 * @param chain true if it is a chain event, else false
 */
void eveScanManager::registerEvent(int smid, eveEventProperty* evproperty, bool chain) {

	if (evproperty == NULL){
		sendError(ERROR, 0, "Not registering event; invalid event property");
		return;
	}
	if (nextEventId > 999999) {
		delete evproperty;
		sendError(ERROR, 0, "Not registering event; eventId overflow");
		return;
	}
	sendError(DEBUG, 0, "registering event");
	++nextEventId;
	evproperty->setEventId(nextEventId);
	evproperty->setSmId(smid);
	evproperty->setChainAction(chain);
	connect(evproperty, SIGNAL(signalEvent(eveEventProperty*)), this, SLOT(newEvent(eveEventProperty*)), Qt::QueuedConnection);
	eventPropList.append(evproperty);
	eveEventRegisterMessage* regmessage = new eveEventRegisterMessage(true, evproperty);
	addMessage(regmessage);
}

void eveScanManager::newEvent(eveEventProperty* evprop) {

	if (shutdownPending) return;

	if (evprop == NULL){
		sendError(ERROR,0,"eveScanManager: invalid event property");
		return;
	}

	if (evprop->getEventType() == eveEventTypeSCHEDULE )
		sendError(DEBUG, 0, QString("eveScanManager: received new schedule event, action type %1").arg(evprop->getActionType()));
	else
		sendError(DEBUG, 0, QString("eveScanManager: received new monitor event"));

	switch (evprop->getActionType()){
	case eveEventProperty::START:
		// TODO ignore all startevents before storage is ready
			if (evprop->isChainAction()){
				if (rootSM) rootSM->startChain();
			}
			else {
				if (rootSM) rootSM->startSM(evprop->getSmId());
			}
		break;
	case eveEventProperty::PAUSE:
		if (rootSM) {
			if (evprop->getOn()){
				// pause on
				if (evprop->isChainAction()){
					if (rootSM) rootSM->pauseChain();
				}
				else {
					if (rootSM) rootSM->pauseSM(evprop->getSmId());
				}
			}
			else {
				// pause off
				if (evprop->isChainAction()){
					if (rootSM) rootSM->resumeChain();
				}
				else {
					if (rootSM) rootSM->resumeSM(evprop->getSmId());
				}
			}
		}
		break;
	case eveEventProperty::HALT:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->haltChain();
		}
		else {
			if (rootSM) rootSM->haltSM(evprop->getSmId());
		}
		break;
	case eveEventProperty::BREAK:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->breakChain();
		}
		else {
			if (rootSM) rootSM->breakSM(evprop->getSmId());
		}
		break;
	case eveEventProperty::STOP:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->stopChain();
		}
		else {
			if (rootSM) rootSM->stopSM(evprop->getSmId());
		}
		break;
	case eveEventProperty::REDO:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->redoChain();
		}
		else {
			if (rootSM) rootSM->redoSM(evprop->getSmId());
		}
		break;
	case eveEventProperty::TRIGGER:
		if (!evprop->isChainAction()){
			if (rootSM) rootSM->triggerSM(evprop->getSmId(), 0);
		}
		break;
	default:
		sendError(ERROR,0,"eveScanManager: unknown event action");
		break;

	}
}

int eveScanManager::sendRequest(int smid, QString text){
	int newrid = eveRequestManager::getRequestManager()->newId(channelId);
	requestHash.insert(newrid, smid);
	addMessage(new eveRequestMessage(newrid,EVEREQUESTTYPE_TRIGGER, text));
	return newrid;
}

void eveScanManager::cancelRequest(int smid, int rid){
	if (requestHash.contains(rid)) {
		requestHash.remove(rid);
		addMessage(new eveRequestCancelMessage(rid));
	}
}
