/*
 * eveScanModule.cpp
 *
 *  Created on: 16.09.2008
 *      Author: eden
 */

#include "eveScanModule.h"
#include "eveScanThread.h"
#include "eveDeviceList.h"

eveScanModule::eveScanModule(eveScanManager *parent, eveXMLReader *parser, int chainid, int smid) :
	QObject(parent){

	manager = parent;
	nestedSM = NULL;
	appendedSM = NULL;
	isRoot = false;
	currentStage = eveStgINIT;
	currentStageReady = false;
	currentStageCounter = 0;
    smstatus = eveSmNOTSTARTED;
    smId = smid;
    chainId = chainid;

	stageHash.insert(eveStgINIT, &eveScanModule::stgInit);
	stageHash.insert(eveStgGOTOSTART, &eveScanModule::stgGotoStart);
	stageHash.insert(eveStgPRESCAN, &eveScanModule::stgPrescan);
	stageHash.insert(eveStgSETTLETIME, &eveScanModule::stgSettleTime);
	stageHash.insert(eveStgTRIGREAD, &eveScanModule::stgTrigRead);
	stageHash.insert(eveStgNEXTPOS, &eveScanModule::stgNextPos);
	stageHash.insert(eveStgPOSTSCAN, &eveScanModule::stgPostscan);
	stageHash.insert(eveStgENDPOS, &eveScanModule::stgEndPos);
	stageHash.insert(eveStgFINISH, &eveScanModule::stgFinish);

	printf("Constructor: Scanmodule with id %d\n", smid);
	// do inner scans
	int nestedNo = parser->getNested(chainId, smId);
    if (nestedNo > 0){
    	nestedSM = new eveScanModule(manager, parser, chainId, nestedNo);
    	connect (nestedSM, SIGNAL(SMready()), this, SLOT(execStage()), Qt::QueuedConnection);
    }

	// do appended scans
	int appendedNo = parser->getAppended(chainId, smId);
    if (appendedNo > 0){
    	appendedSM = new eveScanModule(manager, parser, chainId, appendedNo);
    	connect (appendedSM, SIGNAL(SMready()), this, SLOT(execStage()), Qt::QueuedConnection);
    }

	// TODO register scanmodule local events

	// TODO check startevent

	// get all prescan/postscan actions
    preScanList = parser->getPreScanList(chainId, smId);
    postScanList = parser->getPostScanList(chainId, smId);

	// get all used motors
    axisList = parser->getAxisList(chainId, smId);

	// TODO get all used detectors

    // connect signals
    connect (this, SIGNAL(sigExecStage()), this, SLOT(execStage()), Qt::QueuedConnection);

}

eveScanModule::~eveScanModule() {

	// TODO delete all devices, axes etc.
	if (nestedSM != NULL) delete nestedSM;
	if (appendedSM != NULL) delete appendedSM;
}

/**
 * \brief initialization, will be called after thread init before sm start
 *
 * initialization:
 */
void eveScanModule::initialize() {

	// TODO we don't need this any more, call stgInit() instead
	manager->sendError(INFO,0,"initialize");
	eveScanModule::stgInit();
}

/**
 * \brief initialization, will be called after thread init before sm start
 *
  * -connect all Devices (connect PVs)
 */
void eveScanModule::stgInit() {

	manager->sendError(INFO,0,"stgInit");
	if (currentStageCounter ==0){
		// TODO for now we remain in status eveChainSmINITIALIZING until started
		manager->sendStatus(smId, eveChainSmINITIALIZING);

		// init devices
		foreach (eveSMAxis *axis, *axisList){
			axis->init();
		}

		if (nestedSM != NULL) nestedSM->initialize();
		if (appendedSM != NULL) appendedSM->initialize();
		currentStageCounter=1;
	}
	else {
		// TODO for now we remain in status init until started

		bool ready = true;
		foreach (eveSMAxis *axis, *axisList){
			ready = axis->isDone();
		}
		if (ready){
			// TODO dummy activities, wait 1 second
			((eveScanThread *)QThread::currentThread())->millisleep(1000);
			currentStageReady=true;
			manager->sendStatus(smId, eveChainSmIDLE);
			emit sigExecStage();
		}
	}
}

/**
 * \brief Goto Start Position
 *
 * move motors and all motors of all nested scans to start position,
 * (do not move them, if they have relative positioning).
 * - read motor positions
 * - send motor positions
 *
 */
void eveScanModule::stgGotoStart() {

	if (true) {
		manager->sendError(INFO,0,"stgGotoStart");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

/* gleichzeitiger Start von CA-Kommandos:
 * - eine GroupTransportListe initialisieren
 * - alle SMAxis->gotoStart mit GroupTransportListe aufrufen.
 *   dabei einen CAGroupTransport erzeugen, wenn notwendig (optional einen anderen Transport)
 * - die Achsen schicken ihre Befehle ab ohne ca_flush
 * - am Ende die GroupTransportListe abfeuern (flush)
 *   feuert die caGroup und optional weitere Transports
 *
 *  unabhängiger Start:
 *  - SMAxis->gotoStart(NULL ) erzeugt sich eigenen Transport und starte sich selbst.
 *
 */
		emit sigExecStage();
	}
}

/**
 * \brief prescan actions
 *
 * - read all prescan values which are resetted in postscan
 * - set prescan values
 * - read prescan values (and check status)
 */
void eveScanModule::stgPrescan() {

	if (true) {
		manager->sendError(INFO,0,"stgPrescan");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief wait SettleTime
 *
 * -if we have nested Scans, wait the maximum time of all nested scans
 */
void eveScanModule::stgSettleTime() {

	if (true) {
		manager->sendError(INFO,0,"stgSettleTime");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief trigger detectors and read their values
 *
 * - wait triggerDelay
 * - if necessary, wait for trigger-event
 * - trigger detecors and start nested scan
 * - read detectors  (and check status)
 * - check lowLimit, maxAttempt, maxDeviation
 * - skip data, if redo-event occured
 * - do it again, if average values needed
 * - send detector value
 * - signal detector-ready
 * - wait for nested scan
 *
 */
void eveScanModule::stgTrigRead() {

	if (true) {
		manager->sendError(INFO,0,"stgTrigRead");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief check if we are done and move to the next position (if any)
 *
 * - calculate the next position
 * - decide, if we do another step or if we are done
 * - if necessary, wait for position-event
 * - goto next position
 * - goto startPosition of nested scan
 * - read current position (and check status)
 * - signal ready if last position has been reached
 * - send position value
 *
 */
void eveScanModule::stgNextPos() {

	if (true) {
		manager->sendError(INFO,0,"stgNextPos");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief do all postscan actions
 *
 */
void eveScanModule::stgPostscan() {

	if (true) {
		manager->sendError(INFO,0,"stgPostScan");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief execute position plugins
 *
 */
void eveScanModule::stgEndPos() {

	if (true) {
		manager->sendError(INFO,0,"stgEndPos");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief signal done, finish and process appended SM
 *
 * - send done event
 * - start appended scan
 * - wait for finish signal of appended scan
 * - signal finish (to parent scan)
 *
 */
void eveScanModule::stgFinish() {

	manager->sendError(INFO,0,"stgFinish");
	if (appendedSM && currentStageCounter == 0) {
		++currentStageCounter;
		if (manager->getChainStatus() == eveEngEXECUTING)
			appendedSM->startSM();
		else
			emit sigExecStage();
	}
	else {
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		if ((appendedSM) && (manager->getChainStatus() == eveEngEXECUTING))
			currentStageReady=appendedSM->isDone();
		else
			currentStageReady=true;

		emit sigExecStage();
	}
}

/**
 * \brief check Stop/Break/Halt/Pause and call the appropriate Stage slot
 *
 *
 *
 */
void eveScanModule::execStage() {

	printf("eveScanModule::execStage: Entering\n");
	if ((currentStage == eveStgFINISH) && currentStageReady){
		// we are done
		manager->sendStatus(smId, eveChainSmDONE);
		// before leaving reset status
		smstatus = eveSmDONE;
		emit SMready();
		return;
	}

	if (manager->getChainStatus() == eveEngEXECUTING){
		if (smstatus == eveSmNOTSTARTED) {
			smstatus = eveSmEXECUTING;
			manager->sendStatus(smId, eveChainSmEXECUTING);
		}
		// increment stagecounter if current stage is finished
		if (currentStageReady){
			currentStage = (stageT)(((int) currentStage)+1);
			currentStageReady = false;
			currentStageCounter = 0;
		}
		// call stage method
		(this->*stageHash.value(currentStage))();
	}
	else if (manager->getChainStatus() == eveEngPAUSED){
		smstatus = eveSmPAUSED;
		manager->sendStatus(smId, eveChainSmPAUSED);
	}
	else if (manager->getChainStatus() == eveEngSTOPPED){
		if ((int)currentStage < (int)eveStgPOSTSCAN) {
			currentStage = eveStgPOSTSCAN;
			currentStageReady = false;
			currentStageCounter = 0;
		}
		else {
			if (currentStageReady){
				currentStage = (stageT)(((int) currentStage)+1);
				currentStageReady = false;
				currentStageCounter = 0;
			}
		}
		(this->*stageHash.value(currentStage))();
	}
	else if (manager->getChainStatus() == eveEngHALTED){
		if (currentStage != eveStgFINISH) {
			currentStage = eveStgFINISH;
			currentStageReady = false;
			currentStageCounter = 0;
		}
		(this->*stageHash.value(currentStage))();
	}
}

/**
 * \brief if this scanmodule is paused, start it otherwise call
 *        its nested or appended scanmodules;
 * \return true if changed from paused to start otherwise false
 *
 * start a chain or resumes executing after a pause
 * stop walking down the scanmodules tree when a paused scanmodule
 * was detected.
 */
bool eveScanModule::resumeSM() {

	bool success = false;
	if (smstatus == eveSmPAUSED){
		smstatus = eveSmEXECUTING;
		manager->sendStatus(smId, eveChainSmEXECUTING);
		emit sigExecStage();
		return true;
	}
	if (nestedSM != NULL) success = nestedSM->resumeSM();
	if (!success && (appendedSM != NULL)) success = appendedSM->resumeSM();
	return success;
}
/**
 * \brief start an appended or nested scanmodule
 *
 *
 */
void eveScanModule::startSM() {

	if ((smstatus == eveSmDONE) || (smstatus == eveSmNOTSTARTED)){
		manager->sendStatus(smId, eveChainSmEXECUTING);
		smstatus = eveSmEXECUTING;
		emit sigExecStage();
	}
}

