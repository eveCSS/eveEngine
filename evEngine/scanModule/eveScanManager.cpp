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
	parser->addScanManager(chainId, this);
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

	connect (manager, SIGNAL(startSMs(int)), this, SLOT(smStart(int)), Qt::QueuedConnection);
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

	delete rootSM;

	// TODO make sure mHub reads all messages before closing the channel
	// this is the workaround
	((eveScanThread *)QThread::currentThread())->millisleep(1000);

	eveMessageHub::getmHub()->unregisterChannel(channelId);
	QThread::currentThread()->quit();
}


/**
 * \brief  start SM (or continue if previously paused)
 * \param eventId registered id of start event
 *
 * usually called by a signal from manager thread
 */
void eveScanManager::smStart(int eventId) {

	int dummy = eventId;
	++ dummy;

	if (chainStatus == eveEngIDLEXML){
		chainStatus = eveEngEXECUTING;
		if (rootSM != NULL) rootSM->startSM();
	}
	else {
		chainStatus = eveEngEXECUTING;
		if (rootSM != NULL) rootSM->resumeSM();
	}
}

/**
 * \brief  Halt SM, stop all motors
 *
 * Emergency: stop current SMs, stop running motors, terminate chain, save data
 * do not do prescan, do always
 */
void eveScanManager::smHalt() {
	bool resume;
	// TODO
	// walk down the motor list tree and HALT all running motors
	if (chainStatus == eveEngPAUSED) resume = true;
	chainStatus = eveEngHALTED;
	doBreak = false;
	if (resume && (rootSM != NULL)) rootSM->resumeSM();
}

/**
 * \brief  stop current SM, do postscan actions, resume with the next SM.
 *
 * break after the current step is ready, resume with current postscan
 * do nothing, if we are halted or stopped
 */
void eveScanManager::smBreak() {
	if ((chainStatus == eveEngEXECUTING) || (chainStatus == eveEngPAUSED)) {
		doBreak = true;
	}
}

/**
 * \brief  stop current SM, do postscan actions, terminate chain, save data
 *
 * stop after the current step is ready, resume with current postscan
 * terminate the chain.
 * do nothing, if we are already halted, clear a break
 */
void eveScanManager::smStop() {
	bool resume;
	if ((chainStatus == eveEngEXECUTING) || (chainStatus == eveEngPAUSED)) {
		if (chainStatus == eveEngPAUSED) resume = true;
		chainStatus = eveEngSTOPPED;
		doBreak = false;
		if (resume && (rootSM != NULL)) rootSM->resumeSM();
	}
}

/**
 * \brief Pause current SMs (continue with start command).
 *
 * pause after the current stage is ready, do not stop moving motors
 * do only if we are currently executing
 */
void eveScanManager::smPause() {
	if (chainStatus == eveEngEXECUTING) {
		chainStatus = eveEngPAUSED;
	}
}

/**
 * \brief signals an redo event.
 * \param eventId registered id of redo event
 *
 * there may be several redo events for the various scanModules
 * eventId shows us, which scanModule to inform
 */
void eveScanManager::smRedo(int eventId) {

	int dummy = eventId;
	++ dummy;
}

/**
 * \brief
 *
 */
void eveScanManager::smDone() {

	sendStatus(0, eveChainDONE);
	disconnect (manager, SIGNAL(startSMs(int)), this, SLOT(smStart(int)));
	disconnect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()));
	disconnect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()));
	disconnect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()));
	disconnect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()));

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
