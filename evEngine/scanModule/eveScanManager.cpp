/*
 * eveScanManager.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include <QTimer>
#include <QTime>
#include <QDate>
#include "eveScanThread.h"
#include "eveStartTime.h"
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
	neverStarted = true;;
	nextEventId = chainId*100;
	// TODO register global events

	// TODO check startevent

	//parser->addScanManager(chainId, this);
	int rootId = parser->getRootId(chainId);
	if (rootId == 0)
		sendError(ERROR,0,"eveScanManager::eveScanManager: no root scanmodule found");
	else {
		// walk down the tree and create all ScanModules
		rootSM = new eveScanModule(this, parser, chainId, rootId, eveSmTypeROOT);
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
	addToHash(chainHash, "confirmsave", parser);


	eventList = parser->getChainEventList(chainId);

	// collect all storage-related data
	savePluginHash = parser->getChainPlugin(chainId, "saveplugin");
	addToHash(savePluginHash, "savefilename", parser);
	addToHash(savePluginHash, "autonumber", parser);
	addToHash(savePluginHash, "confirmsave", parser);
	addToHash(savePluginHash, "savescandescription", parser);
	addToHash(savePluginHash, "comment", parser);

	if (savePluginHash.contains("savefilename")) useStorage = true;

	sendStatusTimer = new QTimer(this);
	sendStatusTimer->setInterval(5000);
	sendStatusTimer->setSingleShot(false);
	connect(sendStatusTimer, SIGNAL(timeout()), this, SLOT(sendRemainingTime()));
	sendStatusTimer->start(3000);

}

eveScanManager::~eveScanManager() {
	//delete sendStatusTimer;

	// TODO
	// unregister events ? (or is it done elsewhere)
}

/**
 * \brief  initialization, called if thread starts
 *
 * connect signals from manager thread
 */
void eveScanManager::init() {

	if (!rootSM) {
		sendError(ERROR,0,"init: no root scanmodule found");
		return;
	}
	// init StorageModule if we have a Datafilename
	if (useStorage) {
		sendMessage(new eveStorageMessage(chainId, channelId, savePluginHash, 0, storageChannel));
	}
	rootSM->initialize();
	// init events
	bool haveRedoEvent = false;
	foreach (eveEventProperty* evprop, *eventList){
		sendError(DEBUG, 0, QString("registering chain event"));
		if (haveRedoEvent) sendError(MINOR, 0, QString("more than one redo event in a chain is unsupported"));
		registerEvent(0, evprop, true);
		if (evprop->getActionType() == eveEventProperty::REDO) haveRedoEvent = true;
	}
	delete eventList;
	eventList = NULL;

}

/**
 * \brief shutdown associated chain and this thread
 */
void eveScanManager::shutdown(){

	eveError::log(DEBUG, QString("eveScanManager: shutdown"));

	if (!shutdownPending) {
		shutdownPending = true;
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

		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}

	// make sure mHub reads all outstanding messages before closing the channel
	if (shutdownThreadIfQueueIsEmpty()) eveError::log(DEBUG, QString("eveScanManager: shutdown done"));
}


/**
 * \brief  start SM (or continue if previously paused)
 *
 * usually called by a signal from manager thread
 */
void eveScanManager::smStart() {
	sendError(INFO,0,"eveScanManager::smStart: starting root");
	if (neverStarted){
		sendStartTime();
		neverStarted = false;
	}
	eveEventProperty evprop(eveEventProperty::START, 0);
	if (rootSM) rootSM->newEvent(&evprop);
//	if (rootSM) rootSM->startChain();
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
	eveEventProperty evprop(eveEventProperty::HALT, 0);
	if (rootSM) rootSM->newEvent(&evprop);
//	if (rootSM) rootSM->haltChain();
}

/**
 * \brief  stop current SM, do postscan actions, resume with the next SM.
 *
 * break after the current step is ready, resume with current postscan
 * do nothing, if we are halted or stopped
 */
void eveScanManager::smBreak() {
	eveEventProperty evprop(eveEventProperty::BREAK, 0);
	if (rootSM) rootSM->newEvent(&evprop);
//	if (rootSM) rootSM->breakChain();
}

/**
 * \brief  stop current SM, do postscan actions, terminate chain, save data
 *
 * stop after the current step is ready, resume with current postscan
 * terminate the chain.
 * do nothing, if we are already halted, clear a break
 */
void eveScanManager::smStop() {
	eveEventProperty evprop(eveEventProperty::STOP, 0);
	if (rootSM) rootSM->newEvent(&evprop);
//	if (rootSM) rootSM->stopChain();
}

/**
 * \brief Pause current SMs (continue with start command).
 *
 * pause after the current stage is ready, do not stop moving motors
 * do only if we are currently executing
 */
void eveScanManager::smPause() {
	eveEventProperty evprop(eveEventProperty::PAUSE, 0);
	if (rootSM) rootSM->newEvent(&evprop);
//	if (rootSM) rootSM->pauseChain();
}


/**
 * \brief
 *
 */
void eveScanManager::smDone() {

	if (rootSM && rootSM->isInitializing()){
		rootSM->initialize();
	}
	else if (rootSM && rootSM->isDone()){
		// TODO
		currentStatus.setChainStatus(eveChainDONE);
		sendStatus(0, 0);
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
		if (((eveDataMessage*)message)->getPositionCount() == 0) ((eveDataMessage*)message)->setPositionCount(posCounter);
		sentData = true;
	}
	addMessage(message);
}

void eveScanManager::sendRemainingTime(){

	if (currentStatus.getStatus() == eveChainSmEXECUTING){
		int remainTime = rootSM->getRemainingTime();
		if (remainTime > -1) sendStatus(0, remainTime);
	}
}

/**
 * \brief process messages sent to eveScanManager
 * @param message pointer to the message which eveScanManager received
 */
void eveScanManager::handleMessage(eveMessage *message){

	eveError::log(DEBUG, "eveScanManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_REQUESTANSWER:
		{
			int rid = ((eveRequestAnswerMessage*)message)->getReqId();
			if (requestHash.contains(rid)) {
				eveEventProperty evprop(eveEventProperty::TRIGGER, rid);
				if (rootSM) rootSM->newEvent(&evprop);
//				if (rootSM) rootSM->triggerSM(requestHash.value(rid), rid);
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

void eveScanManager::sendStartTime() {
	eveMessageTextList* mtl = new eveMessageTextList(EVEMESSAGETYPE_METADATA, (QStringList() << "StartTime" << QTime::currentTime().toString("hh:mm:ss")));
	mtl->setChainId(chainId);
	addMessage(mtl);
	mtl = new eveMessageTextList(EVEMESSAGETYPE_METADATA, (QStringList() << "StartDate" << QDate::currentDate().toString("dd.MM.yyyy")));
	mtl->setChainId(chainId);
	addMessage(mtl);
}

/**
 *
 * @param smid id of scanmodule
 * @param status current chain status
 */
void eveScanManager::setStatus(int smid, smStatusT status){
    currentStatus.setStatus(status);
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
	addMessage(new eveChainStatusMessage(currentStatus.getStatus(), chainId, smid, posCounter, eveTime::getCurrent(), remainTime, 0, storageChannel));
}

void eveScanManager::addToHash(QHash<QString, QString>& hash, QString key, eveXMLReader* parser){

	QString value;
	value = parser->getChainString(chainId, key);
	if (!value.isEmpty()) hash.insert(key, value);

}

/**
 * increment position counter and send next-position-message
 * do not increment position counter if no data has been received
 * between two calls to nextPos
 */
void eveScanManager::nextPos(){
	++posCounter;

	eveDataMessage* message = new eveDataMessage("PositionCount", "", eveDataStatus(), DMTmetaData, eveTime::getCurrent(), QVector<int>(1, eveStartTime::getMSecsSinceStart()));
	message->setDestination(storageChannel);
	message->setPositionCount(posCounter);
	message->setChainId(chainId);
	addMessage(message);
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
	if (chain){
		++nextEventId;
		evproperty->setEventId(nextEventId);
	}
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

	if (evprop->isChainAction()){
		if (currentStatus.setEvent(evprop) && rootSM) rootSM->newEvent(evprop);
	}
	else {
		if (rootSM) rootSM->newEvent(evprop);
	}

/*
	switch (evprop->getActionType()){
	case eveEventProperty::START:
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
					sendError(DEBUG, 0, QString("eveScanManager: new event: pause on chain"));
					rootSM->pauseChain();
				}
				else {
					sendError(DEBUG, 0, QString("eveScanManager: new event: pause on smid %1").arg(evprop->getSmId()));
					rootSM->pauseSM(evprop->getSmId());
				}
			}
			else {
				// pause off
				if (evprop->isChainAction()){
					sendError(DEBUG, 0, QString("eveScanManager: new event: pause off chain"));
					rootSM->resumeChain();
				}
				else {
					sendError(DEBUG, 0, QString("eveScanManager: new event: pause off smid %1").arg(evprop->getSmId()));
					rootSM->resumeSM(evprop->getSmId());
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
*/
}

int eveScanManager::sendRequest(int smid, QString text){
	int newrid = eveRequestManager::getRequestManager()->newId(channelId);
	requestHash.insert(newrid, smid);
	addMessage(new eveRequestMessage(newrid,EVEREQUESTTYPE_TRIGGER, text));
	return newrid;
}

void eveScanManager::cancelRequest(int rid){
	if (requestHash.contains(rid)) {
		requestHash.remove(rid);
		addMessage(new eveRequestCancelMessage(rid));
	}
}
