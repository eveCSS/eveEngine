/*
 * eveScanModule.cpp
 *
 *  Created on: 16.09.2008
 *      Author: eden
 */

#include <exception>
#include <QTimer>
#include "eveError.h"
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
    smStatus = eveSmNOTSTARTED;
    smLastStatus = eveSmEXECUTING;
    smId = smid;
    chainId = chainid;
    triggerDelay = 0.0;
	catchedRedo = false;
	catchedTrigger=false;

	stageHash.insert(eveStgINIT, &eveScanModule::stgInit);
	stageHash.insert(eveStgREADPOS, &eveScanModule::stgReadPos);
	stageHash.insert(eveStgGOTOSTART, &eveScanModule::stgGotoStart);
	stageHash.insert(eveStgPRESCAN, &eveScanModule::stgPrescan);
	stageHash.insert(eveStgSETTLETIME, &eveScanModule::stgSettleTime);
	stageHash.insert(eveStgTRIGREAD, &eveScanModule::stgTrigRead);
	stageHash.insert(eveStgNEXTPOS, &eveScanModule::stgNextPos);
	stageHash.insert(eveStgPOSTSCAN, &eveScanModule::stgPostscan);
	stageHash.insert(eveStgENDPOS, &eveScanModule::stgEndPos);
	stageHash.insert(eveStgFINISH, &eveScanModule::stgFinish);

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
    preScanList = parser->getPreScanList(this, chainId, smId);
	foreach (eveSMDevice *device, *preScanList){
	    connect (device, SIGNAL(deviceDone()), this, SLOT(execStage()), Qt::QueuedConnection);
	}
    postScanList = parser->getPostScanList(this, chainId, smId);
	foreach (eveSMDevice *device, *postScanList){
	    connect (device, SIGNAL(deviceDone()), this, SLOT(execStage()), Qt::QueuedConnection);
	}

	// get all used motors
    axisList = parser->getAxisList(this, chainId, smId);
	foreach (eveSMAxis *axis, *axisList){
	    connect (axis, SIGNAL(axisDone()), this, SLOT(execStage()), Qt::QueuedConnection);
	}

	// get all used detector channels
    channelList = parser->getChannelList(this, chainId, smId);
	foreach (eveSMChannel *channel, *channelList){
	    connect (channel, SIGNAL(channelDone()), this, SLOT(execStage()), Qt::QueuedConnection);
	}

	eventList = parser->getSMEventList(chainId, smId);

	postPosPlugin = parser->getPositioningPlugin(chainId, smId, "positioning");

	// connect signals
    connect (this, SIGNAL(sigExecStage()), this, SLOT(execStage()), Qt::QueuedConnection);

}

eveScanModule::~eveScanModule() {

	try
	{
		// TODO delete all devices, axes etc.
		if (nestedSM != NULL) delete nestedSM;
		if (appendedSM != NULL) delete appendedSM;

		foreach (eveSMDevice *device, *preScanList) delete device;
		foreach (eveSMDevice *device, *postScanList) delete device;
		foreach (eveSMAxis *axis, *axisList) delete axis;
		foreach (eveSMChannel *channel, *channelList) delete channel;
		delete preScanList;
		delete postScanList;
		delete axisList;
		delete channelList;
	}
	catch (std::exception& e)
	{
		printf("C++ Exception in ~eveScanModule %s\n",e.what());
		sendError(FATAL, 0, QString("C++ Exception in ~eveScanModule %1").arg(e.what()));
	}

}

/**
 * \brief initialization, will be called after thread init before sm start
 *
 * initialization:
 */
void eveScanModule::initialize() {
	sendError(DEBUG, 0, "initialize");
	(this->*stageHash.value(eveStgINIT))();
}

/**
 * \brief initialization stage
 *
 * connect all Devices (connect PVs)
 * do not proceed with next stage if done
 */
void eveScanModule::stgInit() {

	sendError(DEBUG, 0, "stgInit");
	if (currentStageCounter == 0){
		manager->setStatus(smId, eveSmINITIALIZING);

		signalCounter = 0;

		// init preScan
		foreach (eveSMDevice *device, *preScanList){
			sendError(INFO,0,QString("initializing device %1").arg(device->getName()));
			device->init();
			++signalCounter;
		}
		// init postScan
		foreach (eveSMDevice *device, *postScanList){
			sendError(INFO,0,QString("initializing device %1").arg(device->getName()));
			device->init();
			++signalCounter;
		}
		// init axes
		foreach (eveSMAxis *axis, *axisList){
			sendError(INFO,0,QString("initializing axis %1").arg(axis->getName()));
			axis->init();
			++signalCounter;
		}
		// init detector channels
		foreach (eveSMChannel *channel, *channelList){
			sendError(INFO,0,QString("initializing detector channel %1").arg(channel->getName()));
			channel->init();
			++signalCounter;
		}
		// init events
		foreach (eveEventProperty* evprop, *eventList){
			sendError(DEBUG, 0, QString("registering event for ..."));
			manager->registerEvent(smId, evprop, false);
		}
		delete eventList;
		eventList = NULL;

		if (nestedSM != NULL) {
			nestedSM->initialize();
			++signalCounter;
		}
		if (appendedSM != NULL) {
			appendedSM->initialize();
			++signalCounter;
		}
		currentStageCounter=1;
		emit sigExecStage();

	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			// doing device diagnostics here is unsafe, since the last device might not be
			// ready yet, if an sigExecStage has been emitted by the start button first
			currentStageReady=true;
			emit sigExecStage();
			emit SMready();
			sendError(DEBUG, 0, "stgInit done");
		}
	}
}

void eveScanModule::readPos() {
	sendError(DEBUG, 0, "calling stgReadPos");
	(this->*stageHash.value(eveStgREADPOS))();
}
/**
 * \brief read all motor positions of this sm
 *
 */
void eveScanModule::stgReadPos() {

	sendError(DEBUG,0,"stgReadPos");
	if (currentStageCounter == 0){
		currentStageCounter = 1;
		signalCounter = 0;
		foreach (eveSMAxis *axis, *axisList){
			sendMessage(axis->getDeviceInfo());
			eveVariant dummy = axis->getPos();
			sendError(DEBUG, 0, QString("%1 position is %2").arg(axis->getName()).arg(dummy.toDouble()));
		}
		foreach (eveSMChannel *channel, *channelList){
			sendMessage(channel->getDeviceInfo());
			sendError(INFO,0,QString("testing detector channel %1").arg(channel->getName()));
			channel->triggerRead(false);
			++signalCounter;
		}
		if (nestedSM != NULL) {
			nestedSM->readPos();
			++signalCounter;
		}
		if (appendedSM != NULL) {
			appendedSM->readPos();
			++signalCounter;
		}
		emit sigExecStage();
	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			bool ready = true;
			foreach (eveSMAxis *axis, *axisList){
				if (!axis->isDone()) {
					ready = false;
					sendError(INFO, 0, QString("stgReadPos: axis %1 not ready").arg(axis->getName()));
				}
			}
			if (ready) {
				foreach (eveSMAxis *axis, *axisList){
					sendMessage(axis->getPositionMessage());
				}
				foreach (eveSMChannel *channel, *channelList){
					sendMessage(channel->getValueMessage());
				}
				currentStageReady=true;
				emit SMready();
				emit sigExecStage();
				sendError(DEBUG, 0, "stgReadPos Done");
			}
		}
	}
}


void eveScanModule::gotoStart() {
	sendError(DEBUG, 0, "calling stgGotoStart");
	// this is possible, if parent scan received a break previously
	if (currentStage != eveStgGOTOSTART) currentStage = eveStgGOTOSTART;
	(this->*stageHash.value(eveStgGOTOSTART))();
}
/**
 * \brief Goto Start Position
 *
 * move motors and all motors of all nested scans to start position,
 * - read motor positions
 * - send motor positions
 *
 */
void eveScanModule::stgGotoStart() {

	if (currentStageCounter == 0){
		sendError(DEBUG,0,"stgGotoStart");
		sendNextPos();
		currentStageCounter = 1;
		signalCounter = 0;
		foreach (eveSMAxis *axis, *axisList){
			sendError(DEBUG, 0, QString("Moving axis %1").arg(axis->getName()));
			axis->gotoStartPos(false);
			++signalCounter;
		}
		if (nestedSM != NULL) {
			nestedSM->gotoStart();
			++signalCounter;
		}
		emit sigExecStage();
	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			foreach (eveSMAxis *axis, *axisList){
				eveDataMessage* posMesg = axis->getPositionMessage();
				if (posMesg == NULL)
					sendError(ERROR,0,QString("%1: no position data available").arg(axis->getName()));
				else {
					bool ok = false;
					double dval = posMesg->toVariant().toDouble(&ok);
					if (ok) sendError(INFO,0,QString("%1: at position %2").arg(axis->getName()).arg(dval));
					sendMessage(posMesg);
				}
			}
			sendError(DEBUG, 0, "stgGotoStart done");
			currentStageReady=true;
			emit sigExecStage();
			// if we are not executing, we are called from parent scan
			if (smStatus != eveSmEXECUTING) emit SMready();
		}
	}
}

/**
 * \brief prescan actions
 *
 * - read all device values which are resetted in postscan
 * - set prescan values
 * - read prescan values (and check status)
 */
void eveScanModule::stgPrescan() {

//  make sure all reads are done before writing
//	since both lists may have the same PVs

	if (currentStageCounter == 0){
		sendError(INFO,0,"stgPrescan read");
		currentStageCounter=1;
		signalCounter = 0;
		foreach (eveSMDevice *device, *postScanList){
			if (device->resetNeeded()) {
				device->readValue(false);
				++signalCounter;
			}
		}
		if (!signalCounter) emit sigExecStage();
	}
	else if (currentStageCounter == 1){
		// TODO shouldn't we loop over device->isDone() here, to be sure?
		sendError(INFO,0,"stgPrescan write");
		currentStageCounter=2;
		signalCounter = 0;
		foreach (eveSMDevice *device, *preScanList){
			device->writeValue(false);
			++signalCounter;
		}
		emit sigExecStage();
	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			// TODO shouldn't we loop over device->isDone() here, to be sure?
			sendError(INFO,0,"stgPrescan Done");
			currentStageReady=true;
			emit sigExecStage();
		}
	}
}

/**
 * \brief wait SettleTime
 *
 * every nested scan waits its own settle time
 */
void eveScanModule::stgSettleTime() {

	if (currentStageCounter == 0){
		sendError(INFO,0,"stgSettleTime");
		currentStageCounter=1;
		QTimer::singleShot(1000, this, SLOT(execStage()));
	}
	else {
		sendError(INFO,0,"stgSettleTime Done");

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
 * - do not wait until all detectors are ready with trigger, before
 *    reading them.
 * - check lowLimit, maxAttempt, maxDeviation
 * - skip data, if redo-event occured
 * - do it again, if average values needed
 * - send detector value
 * - signal detector-ready
 * - wait for nested scan
 *
 */
void eveScanModule::stgTrigRead() {

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

	if (currentStageCounter == 0){
		currentStageCounter=1;
	    // TODO triggerDelay is not yet read from XML
		QTimer::singleShot((int)(triggerDelay * 1000.0), this, SLOT(execStage()));
	}
	else if (currentStageCounter == 1){
		currentStageCounter=2;
		signalCounter = 0;
		sendError(DEBUG, 0, "stgTrigRead");
		foreach (eveSMChannel *channel, *channelList){
			sendError(INFO,0,QString("triggering detector channel %1").arg(channel->getName()));
			channel->triggerRead(true);
			++signalCounter;
		}
		if (nestedSM) {
			nestedSM->start();
			++signalCounter;
		}
		// execute the Transport Queue
		// TODO merge the detectors transportlists and execute their queues
		// For now we have just CA as a transport and execute directly
		eveCaTransport::execQueue();
		emit sigExecStage();
	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			bool ready = true;
			foreach (eveSMChannel *channel, *channelList){
				if (channel->isDone()){
					eveDataMessage* dmesg = channel->getValueMessage();
					if (dmesg == NULL)
						sendError(ERROR, 0, QString("%1: no data available").arg(channel->getName()));
					else {
						bool ok = false;
						double dval = dmesg->toVariant().toDouble(&ok);
						if (ok) sendError(DEBUG, 0, QString("%1: value %2").arg(channel->getName()).arg(dval));
						sendMessage(dmesg);
					}
				}
				else {
					ready = false;
					sendError(ERROR, 0, QString("%1: not ready in TrigRead").arg(channel->getName()));
				}
			}
			if (nestedSM && !nestedSM->isDone()) {
				ready = false;
	 			sendError(DEBUG, 0, "stgTrigRead: called done but nested not ready");
			}
			if (ready) {
				sendError(DEBUG, 0, "stgTrigRead Done");
				currentStageReady=true;
				emit sigExecStage();
			}
		}
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

	if (currentStageCounter == 0){
		sendError(INFO, 0, "stgNextPos");
		signalCounter = 0;
		currentStageCounter=1;

		// check if all axes are done
		bool allDone = true;
		foreach (eveSMAxis *axis, *axisList){
			if(!axis->isAtEndPos()) allDone = false;
		}

		// if not all axes are done, go back to trigRead
		if (allDone){
			sendError(INFO,0,"stgNextPos Done");
			currentStageReady=true;
		}
		else {
			sendNextPos();
			foreach (eveSMAxis *axis, *axisList){
				++signalCounter;
				sendError(INFO, 0, QString("Moving axis %1").arg(axis->getName()));
				// gotoNextPos returns immediately, if axis is already at end pos
				axis->gotoNextPos(false);
			}
		}
		emit sigExecStage();
	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			foreach (eveSMAxis *axis, *axisList){
				eveDataMessage* posMesg = axis->getPositionMessage();
				if (posMesg == NULL)
					sendError(ERROR,0,QString("%1: no position data available").arg(axis->getName()));
				else {
					bool ok = false;
					bool iok = false;
					double dval = posMesg->toVariant().toDouble(&ok);
					int ival = posMesg->toVariant().toInt(&iok);
					if (ok && iok) sendError(INFO,0,QString("%1: at position %2 (%3)").arg(axis->getName()).arg(dval).arg(ival));
					sendMessage(posMesg);
				}
			}
			// go back to stage TRIGREAD
			currentStage = eveStgTRIGREAD;
			currentStageReady=false;
			currentStageCounter=0;
			sendError(INFO,0,"stgNextPos Done");
			emit sigExecStage();
		}
	}
}

/**
 * \brief do all postscan actions
 *
 */
void eveScanModule::stgPostscan() {

	if (currentStageCounter == 0){
		sendError(INFO,0,"stgPostScan");
		currentStageCounter=1;
		signalCounter = 0;
		foreach (eveSMDevice *device, *postScanList){
			sendError(INFO,0,"stgPostScan writing");
			device->writeValue(false);
			++signalCounter;
		}
		emit sigExecStage();
	}
	else {
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			sendError(INFO,0,"stgPostScan Done");
			currentStageReady=true;
			emit sigExecStage();
		}
	}
}

/**
 * \brief execute position plugins
 *
 */
void eveScanModule::stgEndPos() {

	if (currentStageCounter == 0){
		currentStageCounter=1;
		QTimer::singleShot(1000, this, SLOT(execStage()));
	}
	else {
		sendError(INFO,0,"stgEndPos");

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

	sendError(INFO,0,"stgFinish");
	if (appendedSM && (currentStageCounter == 0)) {
		++currentStageCounter;
		smStatus = eveSmAPPEND;
		appendedSM->start();
	}
	else {

		// set true if stage done
		currentStageReady=true;
		if ((appendedSM) && (manager->getChainStatus() == eveEngEXECUTING))
			currentStageReady=appendedSM->isDone();

		if (currentStageReady) emit sigExecStage();
	}
}

/**
 * \brief check Stop/Break/Halt/Pause and call the appropriate Stage slot
 *
 *
 *
 */
void eveScanModule::execStage() {

	if ((currentStage == eveStgFINISH) && currentStageReady){
		// we are done
		smStatus = eveSmDONE;
		manager->setStatus(smId, smStatus);
		emit SMready();
		return;
	}

	// if in executing-states we proceed if stage is ready
	if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT) || (smStatus == eveSmAPPEND)){
		// increment stagecounter if current stage is finished
		if (currentStageReady){
			currentStage = (stageT)(((int) currentStage)+1);
			currentStageReady = false;
			currentStageCounter = 0;
		}
		// call stage method
		(this->*stageHash.value(currentStage))();
	}
	// the stages which do not proceed if they are ready
	else if ((currentStage == eveStgINIT) || (currentStage == eveStgREADPOS) || (currentStage == eveStgGOTOSTART)){
		if (currentStageReady == false)
			(this->*stageHash.value(currentStage))();
		else {
			currentStage = (stageT)(((int) currentStage)+1);
			currentStageReady = false;
			currentStageCounter = 0;
		}
		return;
	}
}

void eveScanModule::start() {

	if (smStatus == eveSmNOTSTARTED){
		sendError(DEBUG, 0, "starting scan");
		smStatus = eveSmEXECUTING;
		manager->setStatus(smId, smStatus);
		emit sigExecStage();
	}
    else if (smStatus == eveSmDONE){
    	sendError(DEBUG, 0, "restarting scan");
        smStatus = eveSmEXECUTING;
        currentStage = eveStgGOTOSTART;
        currentStageReady = false;
        currentStageCounter = 0;
        manager->setStatus(smId, smStatus);
        emit sigExecStage();
    }
}


/**
 * \brief walk down until SM with smid is found and resume it, if it is paused
 *
 * \return true if SM with smid was found
 */
bool eveScanModule::resumeSM(int smid) {

	bool found = false;
	if (smId == smid){
		if (smStatus == eveSmPAUSED){
			smStatus = smLastStatus;
			manager->setStatus(smId, smStatus);
			emit sigExecStage();
		}
		found = true;
	}
	if (!found && nestedSM) found = nestedSM->resumeSM(smid);
	if (!found && appendedSM) found = appendedSM->resumeSM(smid);
	return found;
}
/**
 * \brief walk down the SM-tree and start all paused SMs
 * \return true if successfully resumed a paused SM
 */
bool eveScanModule::resumeChain() {

	bool success = false;

	if (nestedSM) success = nestedSM->resumeChain();
	if (!success && (smStatus == eveSmAPPEND)) {
		success = appendedSM->resumeChain();
	}
	if (smStatus == eveSmPAUSED){
		smStatus = smLastStatus;
		manager->setStatus(smId, smStatus);
		emit sigExecStage();
		success = true;
	}
	return success;
}

/**
 * \brief start this scanmodule if smid matches and if it has not been started yet
 *        walk down and start a nested or appended SM with matching smid if its parent
 *        was already started
 *
 * @return true if SM with smid was found
 */
bool eveScanModule::startSM(int smid) {

	bool found = false;
	if (smId == smid){
		if (smStatus == eveSmNOTSTARTED){
			sendError(DEBUG, 0, "starting scan");
			smStatus = eveSmEXECUTING;
			manager->setStatus(smId, smStatus);
			emit sigExecStage();
		}
		found = true;
	}
	else {
		if (nestedSM && ((smStatus == eveSmEXECUTING)||(smStatus == eveSmTRIGGERWAIT)))
			found = nestedSM->startSM(smid);
		if (!found && (smStatus == eveSmAPPEND)) appendedSM->startSM(smid);
	}
	return found;
}
/**
 * \brief if this SM has never been started, start it, else we must have been paused
 * 			and resume. This will only be called in rootSM
 *
 */
void eveScanModule::startChain() {

	sendError(DEBUG, 0, "start signal received");
	if (smStatus == eveSmNOTSTARTED){
		sendError(DEBUG, 0, "starting scan");
		smStatus = eveSmEXECUTING;
		manager->setStatus(smId, smStatus);
		emit sigExecStage();
	}
	else {
		resumeChain();
	}
}

/**
 * \brief walk down until the SM with smid is found and pause it,
 * 			if it is executing
 *
 * @param smid	smid of the sm to be paused
 * @return true if SM with smid was found
 *
 */
bool eveScanModule::pauseSM(int smid) {

	bool found = false;
	if (smId == smid){
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT)) {
			smLastStatus = smStatus;
			smStatus = eveSmPAUSED;
			manager->setStatus(smId, smStatus);
		}
		found = true;
	}
	if (!found && nestedSM) found = nestedSM->pauseSM(smid);
	if (!found && appendedSM) found = appendedSM->pauseSM(smid);
	return found;
}
/**
 * \brief walk down tree with executing SMs and pause them
 */
void eveScanModule::pauseChain() {

		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT)) {
			if (nestedSM) nestedSM->pauseChain();
			smLastStatus = smStatus;
			smStatus = eveSmPAUSED;
			manager->setStatus(smId, smStatus);
		}
		else if (smStatus == eveSmAPPEND){
			appendedSM->pauseChain();
		}
		return;
}

/**
 * \brief walk down until the SM with smid is found, call chainStop if SM is executing
 * 		(a stop signal always stops the whole chain)
 *
 * @param smid	smid of the sm to be stopped
 * @return true if SM with smid was found
 *
 */
bool eveScanModule::stopSM(int smid) {

	bool found = false;
	if (smId == smid){
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT)) manager->smStop();
	}
	if (!found && nestedSM) found = nestedSM->stopSM(smid);
	if (!found && appendedSM) found = appendedSM->stopSM(smid);
	return found;
}

/**
 * \brief find the all executing SMs in the SM-tree; there jump forward to stage stgFinish
 *
 *
 */
void eveScanModule::stopChain() {

		if (nestedSM != NULL) nestedSM->stopChain();
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT)) {
			if (smStatus != eveSmEXECUTING) manager->setStatus(smId, eveSmEXECUTING);
			smStatus = eveSmEXECUTING;
			currentStage = eveStgFINISH;
			currentStageReady = false;
			currentStageCounter = 1;
			emit sigExecStage();
		}
		else if (smStatus == eveSmAPPEND) appendedSM->stopChain();
}

/**
 * \brief walk down until the SM with smid is found, call chainHalt if SM is executing
 * 		(a halt signal always halts the whole chain)
 *
 * @param smid	smid of the sm to be stopped
 * @return true if SM with smid was found
 *
 */
bool eveScanModule::haltSM(int smid) {

	bool found = false;
	if (smId == smid){
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT)) manager->smHalt();
	}
	if (!found && nestedSM) found = nestedSM->haltSM(smid);
	if (!found && appendedSM) found = appendedSM->haltSM(smid);
	return found;
}

/**
 * \brief find the all executing SMs in the SM-tree; stop running motors,
 * 		jump forward to stage after postscan
 */
void eveScanModule::haltChain() {

		if (nestedSM != NULL) nestedSM->haltChain();
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT) || (smStatus == eveSmNOTSTARTED)) {

			if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)) {
				foreach (eveSMAxis *axis, *axisList) axis->stop();
			}
			if (smStatus != eveSmEXECUTING) manager->setStatus(smId, eveSmEXECUTING);
			smStatus = eveSmEXECUTING;
			currentStage = eveStgFINISH;
			currentStageReady = false;
			currentStageCounter = 1;
			emit sigExecStage();
		}
		else if (smStatus == eveSmAPPEND) appendedSM->haltChain();
}

/**
 * \brief walk down until the SM with smid is found and end it,
 * 			if it is executing
 *
 * @param smid	smid of the sm to be paused
 * @return true if SM with smid was found
 *
 */
bool eveScanModule::breakSM(int smid) {

	bool found = false;
	if (smId == smid){
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
				|| (smStatus == eveSmTRIGGERWAIT)) {
			smStatus = eveSmEXECUTING;
			currentStage = eveStgNEXTPOS;
			currentStageReady = true;
			emit sigExecStage();
		}
		found = true;
	}
	if (!found && nestedSM) found = nestedSM->breakSM(smid);
	if (!found && (smStatus == eveSmAPPEND)) found = appendedSM->breakSM(smid);
	return found;
}

/**
 * \brief find the lowest executing SM in the SM-tree and end it,
 *
 */
bool eveScanModule::breakChain() {

	if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
				|| (smStatus == eveSmTRIGGERWAIT)) {
		if (!((nestedSM) && nestedSM->breakChain())){
			smStatus = eveSmEXECUTING;
			currentStage = eveStgNEXTPOS;
			currentStageReady = true;
			emit sigExecStage();
		}
		return true;
	}
	else if (smStatus == eveSmAPPEND) {
		return appendedSM->breakChain();
	}
	return false;
}

/**
 * \brief walk down until the SM with smid is found and signal trigger,
 * 			if it is waiting for a trigger
 *
 * @param smid	smid of the sm to be triggered
 * @return true if SM with smid was found
 *
 */
bool eveScanModule::triggerSM(int smid) {

	bool found = false;
	if (smId == smid){
		found = true;
		if (smStatus == eveSmTRIGGERWAIT) {
			catchedTrigger=true;
			emit sigExecStage();
		}
	}
	if (!found && nestedSM) found = nestedSM->redoSM(smid);
	if (!found && (smStatus == eveSmAPPEND)) found = appendedSM->redoSM(smid);
	return found;
}

/**
 * \brief walk down until the SM with smid is found and signal redo,
 * 			if it is executing
 *
 * @param smid	smid of the sm to be paused
 * @return true if SM with smid was found
 *
 */
bool eveScanModule::redoSM(int smid) {

	bool found = false;
	if (smId == smid){
		found = true;
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT)) {
			catchedRedo = true;
			emit sigExecStage();
		}
	}
	if (!found && nestedSM) found = nestedSM->redoSM(smid);
	if (!found && (smStatus == eveSmAPPEND)) found = appendedSM->redoSM(smid);
	return found;
}

/**
 * \brief find the all executing SMs in the SM-tree; signal redo
 *
 */
void eveScanModule::redoChain() {

	if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
				|| (smStatus == eveSmTRIGGERWAIT)) {
		if (nestedSM) nestedSM->redoChain();
		catchedRedo = true;
		emit sigExecStage();
	}
	else if (smStatus == eveSmAPPEND) {
		appendedSM->redoChain();
	}
}

void eveScanModule::sendMessage(eveMessage* message){

	if (message != NULL){
		if ((message->getType() == EVEMESSAGETYPE_DEVINFO) ||
				(message->getType() == EVEMESSAGETYPE_DATA)){
			((eveBaseDataMessage*)message)->setChainId(chainId);
			((eveBaseDataMessage*)message)->setSmId(smId);

		}
		manager->sendMessage(message);
	}
	else
		sendError(ERROR, 0, "discarding empty message");
}

eveSMAxis* eveScanModule::findAxis(QString axisid){

	foreach (eveSMAxis *axis, *axisList){
		if (axis->getxmlId() == axisid)
		return axis;
	}
	return NULL;
}


void eveScanModule::sendError(int severity, int errorType,  QString message){

	sendError(severity, EVEMESSAGEFACILITY_SCANMODULE, errorType,  message);
}

void eveScanModule::sendError(int severity, int facility, int errorType, QString message){

	// for now we write output to local console too
	eveError::log(severity, QString("ScanModule %1/%2: %3").arg(chainId).arg(smId).arg(message));
	manager->sendError(severity, facility,
							errorType,  QString("ScanModule %1/%2: %3").arg(chainId).arg(smId).arg(message));
}
