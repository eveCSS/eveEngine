/*
 * eveScanManager.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include "eveScanThread.h"
#include "eveScanManager.h"
#include "eveScanModule.h"
#include "eveMessageHub.h"
#include "eveError.h"

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
	chainStatus = eveEngIDLEXML;
	manager = parent;
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

	// parsing is finished, clear all DomElements
}

eveScanManager::~eveScanManager() {
}

/**
 * \brief set the root scanModule
 * \return for now we return true only if no rootSM was set before
bool eveScanManager::setRootSM(eveScanModule *newRoot) {
	if (rootSM == NULL){
		rootSM = newRoot;
		return true;
	}
	return false;
}
*/

/**
 * \brief  initialization, called if thread starts
 *
 * connect signals from manager thread
 */
void eveScanManager::init() {

	printf("Leaving eveScanManager::init() success! \n");
	rootSM->initialize();
}

void eveScanManager::shutdown(){
	eveError::log(1, QString("eveScanManager: shutdown"));

	if (rootSM) delete rootSM;
	rootSM = NULL;
	// TODO make sure mHub reads all messages before closing the channel
	// this is the workaround
	((eveScanThread *)QThread::currentThread())->millisleep(1000);

	eveMessageHub::getmHub()->unregisterChannel(channelId);
	QThread::currentThread()->quit();
}


/**
 * \brief  start SM (or continue if previously paused)
 *
 * usually called by a signal from manager thread
 */
void eveScanManager::smStart() {
	sendError(INFO,0,"eveScanManager::smStart: starting root");
	if (rootSM) rootSM->startSM(true);
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
	if (rootSM) rootSM->haltSM(true);
}

/**
 * \brief  stop current SM, do postscan actions, resume with the next SM.
 *
 * break after the current step is ready, resume with current postscan
 * do nothing, if we are halted or stopped
 */
void eveScanManager::smBreak() {
	if (rootSM) rootSM->breakSM(true);
}

/**
 * \brief  stop current SM, do postscan actions, terminate chain, save data
 *
 * stop after the current step is ready, resume with current postscan
 * terminate the chain.
 * do nothing, if we are already halted, clear a break
 */
void eveScanManager::smStop() {
	if (rootSM) rootSM->stopSM(true);
}

/**
 * \brief Pause current SMs (continue with start command).
 *
 * pause after the current stage is ready, do not stop moving motors
 * do only if we are currently executing
 */
void eveScanManager::smPause() {
	if (rootSM) rootSM->pauseSM(true);
}

/**
 * \brief signals an redo event.
 *
 */
void eveScanManager::smRedo() {
	if (rootSM) rootSM->redoSM(true);
}

/**
 * \brief
 *
 */
void eveScanManager::smDone() {

	if (rootSM->isDone()){
		sendStatus(0, eveChainDONE);
		disconnect (manager, SIGNAL(startSMs()), this, SLOT(smStart()));
		disconnect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()));
		disconnect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()));
		disconnect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()));
		disconnect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()));
	}
}

void eveScanManager::sendMessage(eveMessage *message){
	addMessage(message);
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

	addMessage(new eveChainStatusMessage(status, chainId, smid, posCounter));
}
