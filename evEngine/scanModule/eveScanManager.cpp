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

/**
 *
 * @param parser current XML parser object
 * @param chainid our chain id
 *
 */
eveScanManager::eveScanManager(eveManager *parent, eveXMLReader *parser, int chainid) :
	eveMessageChannel(parent)
{
	QDomElement rootElement;

	chainId = chainid;
	storageChannel = 0;
	posCounter = 0;
	useStorage = false;
	sentData = false;
	chainStatus = eveEngIDLEXML;
	manager = parent;
	waitForMessageBeforeStart = false;
	delayedStart = false;
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

	addToChainHash("savefilename", parser);
	addToChainHash("saveformat", parser);
}

eveScanManager::~eveScanManager() {
}

/**
 * \brief  initialization, called if thread starts
 *
 * connect signals from manager thread
 */
void eveScanManager::init() {

	// init StorageModule if we have a Datafilename
	if (!chainHash.value("savefilename").isEmpty()) {
		useStorage = true;
		// TODO before sending this we must make sure that the corresponding
		// storage channel has been registered with messageHub
		sendMessage(getStorageMessage());
		waitForMessageBeforeStart = true;
	}
	rootSM->initialize();
}

/**
 * \brief shutdown associated chain and this thread
 */
void eveScanManager::shutdown(){

	eveError::log(1, QString("eveScanManager: shutdown"));

	if (!shutdownPending) {
		shutdownPending = true;
		disconnect (manager, SIGNAL(startSMs()), this, SLOT(smStart()));
		disconnect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()));
		disconnect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()));
		disconnect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()));
		disconnect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()));

		// unregister events
		foreach(eveEventProperty* eprop, eventPropList){
			unregisterEvent(eprop);
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
		QThread::currentThread()->quit();
	}
	else {
		// call us again
		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
		// QTimer::singleShot(500, this, SLOT(shutdown()));
	}
}


/**
 * \brief  start SM (or continue if previously paused)
 *
 * usually called by a signal from manager thread
 */
void eveScanManager::smStart() {
	sendError(INFO,0,"eveScanManager::smStart: starting root");
	if (waitForMessageBeforeStart){
		sendError(INFO,0,"eveScanManager::smStart: delayed start");
		delayedStart = true;
	}
	else {
		if (rootSM) rootSM->startChain();
	}
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
		sendStatus(0, eveChainDONE);
		// call shutdown to end this thread
		//QTimer::singleShot(0, this, SLOT(shutdown()));
		shutdown();
	}
}

void eveScanManager::sendMessage(eveMessage *message){
	// a message with destination zero is sent to all storageModules
	if ((message->getType() == EVEMESSAGETYPE_DATA)
		|| (message->getType() == EVEMESSAGETYPE_DEVINFO)) {
		message->setDestination(storageChannel);
	}
	if (message->getType() == EVEMESSAGETYPE_DATA) {
		((eveDataMessage*)message)->setPositionCount(posCounter);
		sentData = true;
	}
	addMessage(message);
}

/**
 * \brief process messages sent to eveScanManager
 * @param message pointer to the message which eveScanManager received
 */
void eveScanManager::handleMessage(eveMessage *message){

	eveError::log(4, "eveScanManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_STORAGEACK:
			// Storage is ready with init, if we already got a start signal
			// we start now
			storageChannel = ((eveMessageInt*)message)->getInt();
			if (waitForMessageBeforeStart){
				waitForMessageBeforeStart = false;
				if (delayedStart){
					delayedStart = false;
					sendError(DEBUG,0,"handleMessage: delayed start, starting now");
					smStart();
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
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility where the error occured
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
		sendStatus(smid, eveChainSmINITIALIZING);
	else if (status == eveSmNOTSTARTED)
		sendStatus(smid, eveChainSmIDLE);
	else if (status == eveSmEXECUTING)
		sendStatus(smid, eveChainSmEXECUTING);
	else if (status == eveSmPAUSED)
		sendStatus(smid, eveChainSmPAUSED);
	else if (status == eveSmTRIGGERWAIT)
		sendStatus(smid, eveChainSmTRIGGERWAIT);
	else if (status == eveSmDONE)
		sendStatus(smid, eveChainSmDONE);
}
/**
 *
 * @param smid id of scanmodule
 * @param status current chain status
 */
void eveScanManager::sendStatus(int smid, chainStatusT status){

	// fill in our storage channel
	addMessage(new eveChainStatusMessage(status, chainId, smid, posCounter, epicsTime::getCurrent(), 0, 0, storageChannel));
}

eveStorageMessage* eveScanManager::getStorageMessage(){

	QString empty;
	return new eveStorageMessage(chainId, channelId, chainHash.value("savefilename", empty),
			chainHash.value("saveformat", empty),
			chainHash.value("pluginname", empty),
			chainHash.value("pluginpath", empty),
			0, EVECHANNEL_STORAGE);
}

void eveScanManager::addToChainHash(QString key, eveXMLReader* parser){

	QString value;
	value = parser->getChainString(chainId, key);
	if (!value.isEmpty()) chainHash.insert(key, value);

}

/**
 * increment position counter and send next-position-message
 * do not increment position counter if no data has been received
 * between two calls to nextPos
 */
void eveScanManager::nextPos(){
	// send nextPos message
	if (sentData){
		sentData = false;
		++posCounter;
	}
}

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
	++nextEventId;
	evproperty->setEventId(nextEventId);
	evproperty->setSmId(smid);
	evproperty->setChainAction(chain);
	evproperty->connectEvent(this);
	eventPropList.append(evproperty);
	eveEventRegisterMessage* regmessage = new eveEventRegisterMessage(true, evproperty);
	addMessage(regmessage);
}

void eveScanManager::unregisterEvent(eveEventProperty* evproperty) {
	addMessage(new eveEventRegisterMessage(false, evproperty));
}

void eveScanManager::newEvent(eveEventProperty* evprop) {

	if (shutdownPending) return;

	if (evprop == NULL){
		sendError(ERROR,0,"eveScanManager: invalid event property");
		return;
	}
	switch (evprop->getActionType()){
	case eveEventActionSTART:
		// ignore all startevents before storage is ready
		if (!waitForMessageBeforeStart){
			if (evprop->isChainAction()){
				if (rootSM) rootSM->startChain();
			}
			else {
				if (rootSM) rootSM->startSM(evprop->getSmId());
			}
		}
		break;
	case eveEventActionPAUSE:
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
	case eveEventActionHALT:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->haltChain();
		}
		else {
			if (rootSM) rootSM->haltSM(evprop->getSmId());
		}
		break;
	case eveEventActionBREAK:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->breakChain();
		}
		else {
			if (rootSM) rootSM->breakSM(evprop->getSmId());
		}
		break;
	case eveEventActionSTOP:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->stopChain();
		}
		else {
			if (rootSM) rootSM->stopSM(evprop->getSmId());
		}
		break;
	case eveEventActionREDO:
		if (evprop->isChainAction()){
			if (rootSM) rootSM->redoChain();
		}
		else {
			if (rootSM) rootSM->redoSM(evprop->getSmId());
		}
		break;
	default:
		sendError(ERROR,0,"eveScanManager: unknown event action");
		break;

	}
}
