/*
 * eveScanModule.cpp
 *
 *  Created on: 16.09.2008
 *      Author: eden
 */

#include "eveScanModule.h"
#include "eveScanThread.h"

eveScanModule::eveScanModule(eveScanManager *parent, eveXMLReader *parser, QDomElement domelem) :
	QObject(parent){

	manager = parent;
	nestedSM = NULL;
	appendedSM = NULL;
	isRoot = false;
	currentStage = eveStgINIT;
	currentStageReady = false;
	currentStageCounter = 0;
    smstatus = eveSmNOTSTARTED;
	stageHash.insert(eveStgMOTORINITIAL, &eveScanModule::stgReadMotorInitial);
	stageHash.insert(eveStgINIT, &eveScanModule::stgInit);
	stageHash.insert(eveStgGOTOSTART, &eveScanModule::stgGotoStart);
	stageHash.insert(eveStgPRESCAN, &eveScanModule::stgPrescan);
	stageHash.insert(eveStgSETTLETIME, &eveScanModule::stgSettleTime);
	stageHash.insert(eveStgNEXT, &eveScanModule::stgNext);
	stageHash.insert(eveStgTRIGREAD, &eveScanModule::stgTrigRead);
	stageHash.insert(eveStgPOSCHECK, &eveScanModule::stgPosCheck);
	stageHash.insert(eveStgPOSTSCAN, &eveScanModule::stgPostscan);
	stageHash.insert(eveStgFINISH, &eveScanModule::stgFinish);

	// get our id, we already know it has an attribute id
	smId = QString(domelem.attribute("id")).toUInt();

	// find the parent scan (do we need this?)
	QDomElement domElement = domelem.firstChildElement("parent");
    if (!domElement.isNull()){
    	if (domElement.text().toUInt() == 0) isRoot = true;
    }

	// do inner scans
	domElement = domelem.firstChildElement("nested");
    if (!domElement.isNull()){
    	if (!manager->getDomElement(domElement.text().toUInt()).isNull()){
    		nestedSM = new eveScanModule(manager, parser, manager->getDomElement(domElement.text().toUInt()));
    		connect (nestedSM, SIGNAL(SMready()), this, SLOT(execStage()), Qt::QueuedConnection);
    	}
    }

	// do appended scans
	domElement = domelem.firstChildElement("appended");
    if (!domElement.isNull()){
    	if (!manager->getDomElement(domElement.text().toUInt()).isNull()){
    		appendedSM = new eveScanModule(manager, parser, manager->getDomElement(domElement.text().toUInt()));
			connect (appendedSM, SIGNAL(SMready()), this, SLOT(execStage()), Qt::QueuedConnection);
		}
    }

	// TODO register scanmodule local events

	// TODO check startevent

	// TODO get all prescan actions

	// TODO get all used motors

	// TODO get all used detectors

    // connect signals
    connect (this, SIGNAL(sigExecStage()), this, SLOT(execStage()), Qt::QueuedConnection);

}

eveScanModule::~eveScanModule() {

	if (nestedSM != NULL) delete nestedSM;
	if (appendedSM != NULL) delete appendedSM;
}

/**
 * \brief initialization, may be called before Start
 *
 * initialization:
 * -connect all Devices (connect PVs)
 * -activate Timer
 */
void eveScanModule::initialize() {

	// TODO for now we remain in status eveChainSmINITIALIZING until started
	manager->sendStatus(smId, eveChainSmINITIALIZING);

	// TODO dummy activities, wait 1 second
	((eveScanThread *)QThread::currentThread())->millisleep(1000);

	if (nestedSM != NULL) nestedSM->initialize();
	if (appendedSM != NULL) appendedSM->initialize();
}

/**
 * \brief initialization, may be called before Start
 *
 * all stage Methods
 * - have signals connected to their group actions, so they are called
 *   if the group received a done event by underlaying transport
 * - if called they check if their group action is finished
 *   and set currentStageReady and emit execStage.
 * - execStage decides which stage comes next.
 *
 * initialization
 * -connect all Devices (connect PVs)
 * -activate Timer
 */
void eveScanModule::stgInit() {

	// TODO for now we remain in status init until started
	// TODO dummy activities, wait 1 second
	((eveScanThread *)QThread::currentThread())->millisleep(1000);

	// set true if stage done
	currentStageReady=true;

	emit sigExecStage();
}

/**
 * \brief read the positions of all involved motors
 *
 * -this is done in the root SM only
 *
 */
void eveScanModule::stgReadMotorInitial() {

	if (true) {
		manager->sendError(INFO,0,"readMotorInitial");
		// TODO dummy activities, wait 1 second
		((eveScanThread *)QThread::currentThread())->millisleep(1000);

		// set true if stage done
		currentStageReady=true;

		emit sigExecStage();
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
 * \brief move to the next step
 *
 * - calculate the next position
 * - decide, if we do another step or if we are done
 * - if necessary, wait for position-event
 * - goto next position
 * - goto startPosition of nested scan
 * - read current position (and check status)
 * - signal ready if last position has been reached
 * - decide, if we do another step
 * - send position value
 *
 */
void eveScanModule::stgNext() {

	if (true) {
		manager->sendError(INFO,0,"stgNext");
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
 * \brief check if we are done, or if we go to the next position
 *
 * may be obsolete, instead call nextPos
 */
void eveScanModule::stgPosCheck() {

	if (true) {
		manager->sendError(INFO,0,"stgPosCheck");
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
		currentStage = eveStgINIT;
		currentStageReady = false;
		currentStageCounter = 0;
		emit sigExecStage();
	}
}

