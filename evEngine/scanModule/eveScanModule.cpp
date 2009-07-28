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
	// TODO we don't need this any more, call stgInit() instead
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
			eveVariant dummy = axis->getPos();
			sendError(DEBUG, 0, QString("%1 position is %2").arg(axis->getName()).arg(dummy.toDouble()));
		}
		if (nestedSM != NULL) {
			nestedSM->readPos();
			++signalCounter;
		}
		if (appendedSM != NULL) {
			appendedSM->stgReadPos();
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
					manager->sendMessage(posMesg);
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
			sendError(INFO,0,"stgPrescan Done");
			currentStageReady=true;
			emit sigExecStage();
		}
	}
}

/**
 * \brief wait SettleTime
 *
 * -if we have nested Scans, wait the maximum time of all nested scans
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
			nestedSM->startSM(false);
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
						manager->sendMessage(dmesg);
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
			foreach (eveSMAxis *axis, *axisList){
				++signalCounter;
				sendError(INFO, 0, QString("Moving axis %1").arg(axis->getName()));
				// calling gotoNextPos if axis is at end pos just does nothing
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
					double dval = posMesg->toVariant().toDouble(&ok);
					if (ok) sendError(INFO,0,QString("%1: at position %2").arg(axis->getName()).arg(dval));
					manager->sendMessage(posMesg);
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
		appendedSM->startSM(false);
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

/**
 * \brief if this scanmodule is paused, start it and call
 *        its nested SM or check appended scanmodules;
 * \return true if changed from paused to start otherwise false
 *
 * resume executing after a pause
 * walk down the scanmodules and start all nested (appended) paused scans
 *
 */
bool eveScanModule::resumeSM() {

	bool success = false;
	if (nestedSM) success = nestedSM->resumeSM();
	if (!success && (smStatus == eveSmAPPEND)) {
		success = appendedSM->resumeSM();
	}
	else if (smStatus == eveSmPAUSED){
		smStatus = smLastStatus;
		manager->setStatus(smId, smStatus);
		emit sigExecStage();
	}
	return success;
}
/**
 * \brief start this scanmodule or resume if paused
 *
 * @param chainSignal if true, is signal from chain to be applied on all scanmodules
 */
void eveScanModule::startSM(bool chainSignal) {

	sendError(DEBUG, 0, "start signal received");
	if (smStatus == eveSmNOTSTARTED){
		sendError(DEBUG, 0, "starting scan");
		smStatus = eveSmEXECUTING;
		manager->setStatus(smId, smStatus);
		emit sigExecStage();
	}
	else if (chainSignal) {
		resumeSM();
	}
	else if (smStatus == eveSmPAUSED){
		smStatus = smLastStatus;
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
 * \brief pause this scanmodule
 *
 * @param chainSignal if true, is signal from chain to be applied on all scanmodules
 */
void eveScanModule::pauseSM(bool chainSignal) {

	if (chainSignal) {
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT)) {
			if (nestedSM) nestedSM->pauseSM(true);
		}
		else if (smStatus == eveSmAPPEND){
			appendedSM->pauseSM(true);
		}
	}
	if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmTRIGGERWAIT)) {
		smLastStatus = smStatus;
		smStatus = eveSmPAUSED;
		manager->setStatus(smId, smStatus);
	}
}

/**
 * \brief if scanModule is executing, jump forward to stage stgFinish and execute
 * @param chainSignal if true, is signal from chain to be applied to all scanmodules
 *
 * a stop signal always applies to the whole chain,
 * call the managers stop method which calls us
 *
 */
void eveScanModule::stopSM(bool chainSignal) {

	if (!chainSignal) {
		manager->smStop();
	}
	else {
		if (nestedSM != NULL) nestedSM->stopSM(true);
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT)) {
			if (smStatus != eveSmEXECUTING) manager->setStatus(smId, eveSmEXECUTING);
			smStatus = eveSmEXECUTING;
			currentStage = eveStgFINISH;
			currentStageReady = false;
			currentStageCounter = 1;
			emit sigExecStage();
		}
		else if (smStatus == eveSmAPPEND) appendedSM->stopSM(true);
	}
}

/**
 * \brief emergency halt: if scanModule is executing, stop running motors,
 * set stage after postscan and execute
 * @param chainSignal if not set, call the managers halt method which calls us
 *
 *
 */
void eveScanModule::haltSM(bool chainSignal) {

	if (!chainSignal) {
		manager->smHalt();
	}
	else {
		if (nestedSM != NULL) nestedSM->haltSM(true);
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT)) {
			foreach (eveSMAxis *axis, *axisList){
				axis->stop();
			}
			if (smStatus != eveSmEXECUTING) manager->setStatus(smId, eveSmEXECUTING);
			smStatus = eveSmEXECUTING;
			currentStage = eveStgFINISH;
			currentStageReady = false;
			currentStageCounter = 1;
			emit sigExecStage();
		}
		else if (smStatus == eveSmAPPEND) appendedSM->haltSM(true);
	}
}

void eveScanModule::breakSM(bool chainSignal) {

	if (chainSignal) {
		breakNestedSM();
	}
	else if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT)) {
		smStatus = eveSmEXECUTING;
		currentStage = eveStgNEXTPOS;
		currentStageReady = true;
		emit sigExecStage();
	}
}

bool eveScanModule::breakNestedSM() {

	if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
				|| (smStatus == eveSmTRIGGERWAIT)) {
		if (!((nestedSM) && nestedSM->breakNestedSM())){
			smStatus = eveSmEXECUTING;
			currentStage = eveStgNEXTPOS;
			currentStageReady = true;
			emit sigExecStage();
		}
		return true;
	}
	else if (smStatus == eveSmAPPEND) {
		return appendedSM->breakNestedSM();
	}
	return false;
}

void eveScanModule::redoSM(bool chainSignal) {

	if (chainSignal) {
		if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
					|| (smStatus == eveSmTRIGGERWAIT)) {
			if (nestedSM) nestedSM->redoSM(true);
			catchedRedo = true;
			emit sigExecStage();
		}
		else if (smStatus == eveSmAPPEND) {
			appendedSM->redoSM(true);
		}
	}
	else if ((smStatus == eveSmEXECUTING) || (smStatus == eveSmPAUSED)
			|| (smStatus == eveSmTRIGGERWAIT)) {
		catchedRedo = true;
		emit sigExecStage();
	}
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
