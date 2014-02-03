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
#include "eveEventProperty.h"
#include "eveStartTime.h"


eveScanModule::eveScanModule(eveScanManager *parent, eveXMLReader *parser, int chainid, int smid, smTypeT stype) :
	QObject(parent){

    manager = parent;
    nestedSM = NULL;
    appendedSM = NULL;
    smType = stype;
    currentStage = eveStgINIT;
    currentStageReady = false;
    currentStageCounter = 0;
    smId = smid;
    chainId = chainid;
//	delayedStart = false;
    expectedPositions = 1;
    currentPosition = 0;
    eventTrigger = false;
    manualTrigger = false;
    manDetTrigger = false;
    setAxisOffset = false;
    doRedo=false;
    doBreak=false;
    triggerRid = 0;
    triggerDetecRid = 0;
    perPosCount = 0;

	// convert times to msecs
    settleDelay = (int)(parser->getSMTagDouble(chainId, smId, "settletime", 0.0)*1000.0);
    triggerDelay = (int)(parser->getSMTagDouble(chainId, smId, "triggerdelay", 0.0)*1000.0);
    manualTrigger  = parser->getSMTagBool(chainId, smId, "triggerconfirmaxis", false);
    manDetTrigger = parser->getSMTagBool(chainId, smId, "triggerconfirmchannel", false);
    QString scanType = parser->getSMTag(chainId, smId, "type");
    storageHint = parser->getSMTag(chainId, smId, "storage");
    if (storageHint.length() == 0) storageHint = "default";
    // TODO Workaround until storageHint is implemented in viewer
    if ((scanType == "save_axis_positions") || (scanType == "save_channel_values"))
        storageHint = "alternate";

    stageHash.insert(eveStgINIT, &eveScanModule::stgInit);
    stageHash.insert(eveStgREADPOS, &eveScanModule::stgReadPos);
    stageHash.insert(eveStgSTARTEXECUTING, &eveScanModule::eveStgStartExecuting);
    stageHash.insert(eveStgGOTOSTART, &eveScanModule::stgGotoStart);
    stageHash.insert(eveStgPRESCAN, &eveScanModule::stgPrescan);
    stageHash.insert(eveStgSETTLETIME, &eveScanModule::stgSettleTime);
    stageHash.insert(eveStgTRIGWAIT, &eveScanModule::stgTrigWait);
    stageHash.insert(eveStgTRIGREAD, &eveScanModule::stgTrigRead);
    stageHash.insert(eveStgNEXTPOS, &eveScanModule::stgNextPos);
    stageHash.insert(eveStgPOSTSCAN, &eveScanModule::stgPostscan);
    stageHash.insert(eveStgENDPOS, &eveScanModule::stgEndPos);
    stageHash.insert(eveStgFINISH, &eveScanModule::stgFinish);

    // do inner scans
    int nestedNo = parser->getNested(chainId, smId);
    if (nestedNo > 0){
    	nestedSM = new eveScanModule(manager, parser, chainId, nestedNo, eveSmTypeNESTED);
    	connect (nestedSM, SIGNAL(SMready()), this, SLOT(execStage()), Qt::QueuedConnection);
    }

    // do appended scans
    int appendedNo = parser->getAppended(chainId, smId);
    if (appendedNo > 0){
    	appendedSM = new eveScanModule(manager, parser, chainId, appendedNo, eveSmTypeAPPENDED);
    	connect (appendedSM, SIGNAL(SMready()), this, SLOT(execStage()), Qt::QueuedConnection);
    }

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

	// get all used axes and their total steps
    axisList = parser->getAxisList(this, chainId, smId);
    foreach (eveSMAxis *axis, *axisList){
        if (!motorList.contains(axis->getMotor())) motorList.append(axis->getMotor());
        connect (axis, SIGNAL(axisDone()), this, SLOT(execStage()), Qt::QueuedConnection);
        if (axis->getExpectedPositions() > expectedPositions) expectedPositions = axis->getExpectedPositions();
    }

    // get values per position
    valuesPerPos = parser->getSMTagInteger(chainId, smId, "valuecount", 1);
    expectedPositions *= valuesPerPos;
    eveError::log(DEBUG, QString("Scan will have approx. %1 steps").arg(expectedPositions));

    // get all used detector channels and create a list of detectors
    // channels used as normalize channels are removed from this list
    channelList = parser->getChannelList(this, chainId, smId);
    foreach (eveSMChannel *channel, *channelList){
        connect (channel, SIGNAL(channelDone()), this, SLOT(execStage()), Qt::QueuedConnection);
        if (!detectorList.contains(channel->getDetector())) detectorList.append(channel->getDetector());
        if (channel->getNormalizeChannel() != NULL) if (!detectorList.contains(channel->getNormalizeChannel()->getDetector()))
                    detectorList.append(channel->getNormalizeChannel()->getDetector());
    }

    eventList = parser->getSMEventList(chainId, smId);

    // get the postscan positioner plugins
    posPluginDataList = parser->getPositionerPluginList(chainId, smId);
    while (!posPluginDataList->isEmpty()){
            QHash<QString, QString>* positionerHash = posPluginDataList->takeFirst();
            QString algorithm = positionerHash->value("pluginname", QString());
            QString xAxisId = positionerHash->value("axis_id", QString());
            QString channelId = positionerHash->value("channel_id", QString());
            QString normalizeId = positionerHash->value("normalize_id", QString());
            delete positionerHash;

            if (!algorithm.isEmpty() && !xAxisId.isEmpty() && !channelId.isEmpty()){
                    eveCalc* positioner = new eveCalc(manager, algorithm, xAxisId, channelId, normalizeId);
                    positionerList.append(positioner);
                    foreach (eveSMAxis *axis, *axisList){
                            if (axis->getXmlId() == xAxisId) {
                                    axis->addPositioner(positioner);
                            }
                    }
                    foreach (eveSMChannel *channel, *channelList){
                            if ((channel->getXmlId() == channelId) ||
                                    (channel->getXmlId() == normalizeId)) {
                                            channel->addPositioner(positioner);
                            }
                    }
            }

    }
    delete posPluginDataList;

    // connect signals
    connect (this, SIGNAL(sigExecStage()), this, SLOT(execStage()), Qt::QueuedConnection);
}

eveScanModule::~eveScanModule() {

    try
    {
        // TODO
        // unregister events ? (or is it done elsewhere)

        if (nestedSM != NULL) delete nestedSM;
        if (appendedSM != NULL) delete appendedSM;

        foreach (eveSMDevice *device, *preScanList) delete device;
        foreach (eveSMDevice *device, *postScanList) delete device;
        foreach (eveSMAxis *axis, *axisList) delete axis;
        foreach (eveSMChannel *channel, *channelList) delete channel;
        foreach (eveSMDetector* detector, detectorList) {
                sendError(DEBUG, 0, QString("deleting detector %1").arg(detector->getName()));
                delete detector;
        }
        foreach (eveSMMotor* motor, motorList) {
                sendError(DEBUG, 0, QString("deleting motor %1").arg(motor->getName()));
                delete motor;
        }
        foreach (eveCalc *positioner, positionerList) delete positioner;
        delete preScanList;
        delete postScanList;
        delete axisList;
        delete channelList;
    }
    catch (std::exception& e)
    {
            sendError(FATAL, 0, QString("C++ Exception in ~eveScanModule %1").arg(e.what()));
    }

}

/**
 * \brief initialization, will be called from manager for rootSM only
 *
 */
void eveScanModule::initialize() {
    sendError(DEBUG, 0, "initialize");
    emit sigExecStage();
}

/**
 * \brief initialization stage
 *
 * connect all Devices (connect PVs)
 * do not proceed with next stage if done
 */
void eveScanModule::stgInit() {

    if (currentStageCounter == 0){
        sendError(DEBUG, 0, "stgInit starting");
        // SMStatus eveSmINITIALIZING is unused, but we send it to GUI if
        // this SM is not already running
        if (myStatus.getStatus() == eveSmNOTSTARTED) manager->setStatus(smId, eveSmINITIALIZING, 0);

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
            if (evprop->getActionType() == eveEventProperty::TRIGGER) eventTrigger=true;
            else if (evprop->getActionType() == eveEventProperty::REDO) doRedo=true;
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
            // we need to check all devices before proceeding
            bool allInitDone = true;
            if (allInitDone) foreach (eveSMDevice *device, *preScanList) if (!device->isDone()) allInitDone = false;
            if (allInitDone) foreach (eveSMDevice *device, *postScanList) if (!device->isDone()) allInitDone = false;
            if (allInitDone) foreach (eveSMAxis *axis, *axisList) if (!axis->isDone()) allInitDone = false;
            if (allInitDone) foreach (eveSMChannel *channel, *channelList) if (!channel->isDone()) allInitDone = false;
            if ((nestedSM != NULL) && (!nestedSM->isStageDone())) allInitDone = false;
            if ((appendedSM != NULL) && (!appendedSM->isStageDone())) allInitDone = false;
            if (allInitDone){
                currentStageReady=true;
                emit sigExecStage();
                sendError(DEBUG, 0, "stgInit done");
            }
        }
    }
}

/**
 *  will be called from root SM for nested/appended SMs only
 *
 */
void eveScanModule::readPos() {
    sendError(DEBUG, 0, "calling stgReadPos");
    currentStage = eveStgREADPOS;
    currentStageCounter = 0;
    currentStageReady = false;
    emit sigExecStage();
}
/**
 * \brief read all motor positions and detector values of this and all
 * nested and appended SMs, but only if this is the root-SM
 *
 */
void eveScanModule::stgReadPos() {

    if (currentStageCounter == 0){
        sendError(DEBUG, 0, "stgReadPos starting");
        currentStageCounter = 1;
        signalCounter = 0;
//        QDateTime startTime = eveStartTime::getStartTime();
        foreach (eveSMAxis *axis, *axisList){
//            axis->setTimer(startTime);
            sendMessage(axis->getDeviceInfo());
            eveVariant dummy = axis->getPos();
            sendError(DEBUG, 0, QString("%1 position is %2").arg(axis->getName()).arg(dummy.toDouble()));
        }
        foreach (eveSMChannel *channel, *channelList){
//            channel->setTimer(startTime);
            sendMessage(channel->getDeviceInfo(false));
            if (channel->getNormalizeChannel() != NULL) {
                sendMessage(channel->getDeviceInfo(true));
                sendMessage(channel->getNormalizeChannel()->getDeviceInfo(false));
            }
            //read channel value to check if it is working, skip channels with long integration time;
            if (channel->readAtInit()){
                sendError(INFO,0,QString("testing detector channel %1").arg(channel->getName()));
                channel->triggerRead(false);
                ++signalCounter;
            }
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
            foreach (eveSMChannel *channel, *channelList){
                if (channel->readAtInit() && !channel->isDone()) {
                    ready = false;
                    sendError(INFO, 0, QString("stgReadPos: channel %1 not ready").arg(channel->getName()));
                }
            }
            if ((nestedSM != NULL) && (!nestedSM->isStageDone())) ready = false;
            if ((appendedSM != NULL) && (!appendedSM->isStageDone())) ready = false;
            if (ready) {
                currentStageReady=true;
                emit sigExecStage();
                sendError(DEBUG, 0, "stgReadPos Done");
            }
        }
    }
}

/**
 * \brief first stage to be executed if scan has been started (and init is done)
 *
 */
void eveScanModule::eveStgStartExecuting() {

    QDateTime startTime = eveStartTime::getStartTime();
    foreach (eveSMAxis *axis, *axisList) axis->setTimer(startTime);
    foreach (eveSMChannel *channel, *channelList) channel->setTimer(startTime);

    if (smType == eveSmTypeROOT) {
        // reset manager status from "initialization" to "idle"
        manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
    }

    currentStageReady = true;
    emit sigExecStage();
}

/**
 * \brief Goto Start Position
 *
 * move motors  to start position,
 * - read motor positions
 * - send motor positions
 *
 */
void eveScanModule::stgGotoStart() {

    if (currentStageCounter == 0){
        // increment posCounter unless we are nested
        if (smType != eveSmTypeNESTED) sendNextPos();
        sendError(DEBUG, 0, "stgGotoStart");
        currentStageCounter = 1;
        signalCounter = 0;
        foreach (eveCalc *positioner, positionerList) {
            positioner->reset();
        }
        foreach (eveSMAxis *axis, *axisList){
            sendError(DEBUG, 0, QString("Moving axis %1").arg(axis->getName()));
            axis->setTimer(QDateTime::currentDateTime());
            // gotoStartPos but do not queue, don't set offset the first time, don't ignore timer
            axis->gotoStartPos(false, setAxisOffset, false);
            setAxisOffset = true;
            ++signalCounter;
        }
        emit sigExecStage();
    }
    else {
        if (signalCounter > 0){
            --signalCounter;
        }
        else {
            // check if all axes are done
            bool allDone = true;
            foreach (eveSMAxis *axis, *axisList) if (!axis->isDone()) allDone = false;
            if (allDone){
                foreach (eveSMAxis *axis, *axisList){
                    eveDataMessage* posMesg = axis->getPositionMessage();
                    if (posMesg == NULL)
                        sendError(ERROR,0,QString("%1: no position data available").arg(axis->getName()));
                    else {
                        bool ok = false;
                        double dval = posMesg->toVariant().toDouble(&ok);
                        if (ok) sendError(INFO,0,QString("%1: at position %2").arg(axis->getName()).arg(dval));
                        axis->loadPositioner(manager->getPositionCount());
                        sendMessage(posMesg);
                    }
                }
                sendError(DEBUG, 0, "stgGotoStart done");
                currentStageReady=true;
                emit sigExecStage();
            }
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
        emit sigExecStage();
    }
    else if (currentStageCounter == 1){
        if (signalCounter > 0){
            --signalCounter;
        }
        else {
            bool allDone = true;
            foreach (eveSMDevice *device, *postScanList)
                if (device->resetNeeded() && !device->isDone()) allDone = false;;
            if (allDone) {
                sendError(INFO,0,"stgPrescan write");
                currentStageCounter=2;
                signalCounter = 0;
                foreach (eveSMDevice *device, *preScanList){
                    device->writeValue(false);
                    ++signalCounter;
                }
                emit sigExecStage();
            }
        }
    }
    else {
        if (signalCounter > 0){
            --signalCounter;
        }
        else {
            bool allDone = true;
            foreach (eveSMDevice *device, *preScanList) if (!device->isDone()) allDone = false;
            if (allDone) {
                sendError(INFO,0,"stgPrescan Done");
                currentStageReady=true;
                emit sigExecStage();
            }
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
        //sendError(DEBUG,0,"stgSettleTime");
        currentStageCounter=1;
        QTimer::singleShot((settleDelay), this, SLOT(execStage()));
        settleTime.start();
    }
    else {
        if (settleTime.elapsed() < settleDelay){
            QTimer::singleShot(settleDelay-settleTime.elapsed(), this, SLOT(execStage()));
        }
        else {
            //sendError(DEBUG,0,"stgsettleDelay Done");
            currentStageReady=true;
            emit sigExecStage();
        }
    }
}

/**
 * \brief wait for trigger delay and manual trigger
 *
 * - wait triggerDelay
 * - if necessary, wait for trigger-event or a manual trigger via request
 *
 */
void eveScanModule::stgTrigWait() {

    if (currentStageCounter == 0){
        ++perPosCount;
        sendError(DEBUG,0,"stgTrigWait start");
        currentStageCounter=1;
        triggerTime.start();
        QTimer::singleShot(triggerDelay, this, SLOT(execStage()));
    }
    else if (currentStageCounter == 1){
        if (triggerTime.elapsed() < triggerDelay){
            QTimer::singleShot(triggerDelay-triggerTime.elapsed(), this, SLOT(execStage()));
        }
        else {
            currentStageCounter=2;
            signalCounter = 0;
            if (manDetTrigger){
                triggerDetecRid = manager->sendRequest(smId, "Trigger Detector");
                if (myStatus.triggerDetecStart(triggerDetecRid))
                    manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
            }
            else
                emit sigExecStage();
        }
    }
    else if (currentStageCounter == 2){
        if (manDetTrigger){
            if (!myStatus.isTriggerDetecWait()){
                currentStageReady=true;
                manager->cancelRequest(triggerDetecRid);
                emit sigExecStage();
            }
        }
        else {
            currentStageReady=true;
            emit sigExecStage();
        }
    }
}

/**
 * \brief trigger detectors and read their values
 *
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

    if (currentStageCounter == 0){
        sendError(DEBUG,0,"stgTrigRead start");
        if (!(doRedo && myStatus.isRedo())) {
            if (doRedo) myStatus.redoStart();
            currentStageCounter=1;
            signalCounter = 0;
            foreach (eveSMChannel *channel, *channelList){
                if (!channel->isDeferred()) {
                    sendError(INFO,0,QString("trigger detector channel %1").arg(channel->getName()));
                    channel->triggerRead(true);
                    ++signalCounter;
                }
            }
            triggerPosCount=manager->getPositionCount();
            if (nestedSM) {
                nestedSM->startExec();
                ++signalCounter;
            }
            // execute the Transport Queue
            // TODO merge the detectors transportlists and execute their queues
            // For now we have just CA as a transport and execute directly
            eveCaTransport::execQueue();
            emit sigExecStage();
        }
        else
            sendError(DEBUG,0,"stgTrigRead redo on");

    }
    else if (currentStageCounter == 1){
        if (signalCounter > 0){
            --signalCounter;
        }
        else if (doRedo && myStatus.redoStatus()){
            // redo occcured, go back to previous step
            sendError(INFO,0,"got redo event, repeating trigger stage");
            currentStageCounter=0;
            emit sigExecStage();
        }
        else {
            bool ready = true;
            foreach (eveSMChannel *channel, *channelList) if (!channel->isDeferred() && !channel->isDone()) ready = false;
            if (ready) {
                currentStageCounter=2;
                signalCounter = 0;
                foreach (eveSMChannel *channel, *channelList){
                    if (channel->isDeferred()) {
                        sendError(INFO,0,QString("triggering detector channel %1").arg(channel->getName()));
                        channel->triggerRead(true);
                        ++signalCounter;
                    }
                }
                eveCaTransport::execQueue();
                emit sigExecStage();
            }
        }
    }
    else {
        if (signalCounter > 0){
            --signalCounter;
        }
        else if (doRedo && myStatus.redoStatus()){
            // redo occcured, go back to previous step
            sendError(INFO,0,"got redo event, repeating deferred trigger stage");
            currentStageCounter=1;
            emit sigExecStage();
        }
        else {
            bool ready = true;
            foreach (eveSMChannel *channel, *channelList) if (!channel->isDone()) ready = false;
            if (nestedSM) ready = nestedSM->isDone();
            if (ready) {
                foreach (eveSMChannel *channel, *channelList){
                    eveDataMessage* normMsg = channel->getNormValueMessage();
                    eveDataMessage* normRawMsg = channel->getNormRawValueMessage();
                    eveDataMessage* dataMsg = channel->getValueMessage();
                    if (normMsg != NULL){
                        normMsg->setPositionCount(triggerPosCount);
                        bool ok = false;
                        double dval = normMsg->toVariant().toDouble(&ok);
                        if (ok) sendError(DEBUG, 0, QString("%1: normalized value %2").arg(channel->getName()).arg(dval));
                        sendMessage(normMsg);
                    }
                    if (normRawMsg != NULL){
                        normRawMsg->setPositionCount(triggerPosCount);
                        bool ok = false;
                        double dval = normRawMsg->toVariant().toDouble(&ok);
                        if (ok) sendError(DEBUG, 0, QString("%2: channel (used for normalization by %1) raw value: %3").arg(channel->getName()).arg(normRawMsg->getName()).arg(dval));
                        sendMessage(normRawMsg);
                    }
                    if (dataMsg != NULL) {
                        dataMsg->setPositionCount(triggerPosCount);
                        bool ok = false;
                        double dval = dataMsg->toVariant().toDouble(&ok);
                        if (ok) sendError(DEBUG, 0, QString("%1: raw value %2").arg(channel->getName()).arg(dval));
                        if (dataMsg->getDataType() == eveInt32T) {
                            sendError(DEBUG, 0, QString("Severity: %1, Alarm: %2").arg(dataMsg->getDataStatus().getSeverityString()).arg(dataMsg->getDataStatus().getAlarmString()));
                            sendError(DEBUG, 0, QString("arraySize: %1, ptr: %2").arg(dataMsg->getArraySize()).arg((unsigned int)((quint64)dataMsg->getIntArray().constData())));
                            int *iarray= (int*) dataMsg->getIntArray().constData();
                            for (int i=0; i< dataMsg->getArraySize(); ++i) {
                                if ((iarray[i] < 0) || (iarray[i] > 10000))
                                    sendError(DEBUG,0,QString("%1: %2").arg(i).arg(iarray[i]));
                            }
                        }
                        sendMessage(dataMsg);
                        channel->loadPositioner(manager->getPositionCount());
                    }
                    else {
                        sendError(ERROR, 0, QString(" no data available for channel %1").arg(channel->getName()));
                    }
                }
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
 * - check if we need another measurement at this position (perPosCount < valuesPerPos)
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
        sendError(DEBUG, 0, "stgNextPos");
        signalCounter = 0;

        if (perPosCount < valuesPerPos){
            // don't move just read position, skip stage 1
            currentStageCounter=2;
            foreach (eveSMAxis *axis, *axisList){
                ++signalCounter;
                sendError(INFO, 0, QString("Read axis position %1").arg(axis->getName()));
                axis->readPos(false);
            }
        }
        else {
            currentStageCounter=1;
            perPosCount = 0;
            // check if all axes are done
            bool allDone = true;
            foreach (eveSMAxis *axis, *axisList) if (!axis->isAtEndPos()) allDone = false;

            if (allDone){
                sendError(INFO,0,"stgNextPos Done");
                currentStageReady=true;
            }
            else {
                if (manualTrigger){
                    triggerRid = manager->sendRequest(smId, "Trigger Positioning");
                    if (myStatus.triggerManualStart(triggerRid)) manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                    ++signalCounter;
                }
                if (eventTrigger){
                    if (myStatus.triggerEventStart()) manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                    ++signalCounter;
                }
            }
        }
        emit sigExecStage();
    }
    else if (currentStageCounter == 1){

        if (signalCounter > 0){
            --signalCounter;
        }
        else {
            if (!myStatus.isTriggerEventWait() && !myStatus.isManualTriggerWait()){
                currentStageCounter=2;
                signalCounter = 0;

                if (manualTrigger) manager->cancelRequest(triggerRid);

                foreach (eveSMAxis *axis, *axisList){
                    ++signalCounter;
                    sendError(INFO, 0, QString("Moving axis %1").arg(axis->getName()));
                    axis->gotoNextPos(false);
                }
                emit sigExecStage();
            }
        }
    }
    else {
        if (signalCounter > 0){
            --signalCounter;
        }
        else {
            // check if all axes are done
            bool allDone = true;
            foreach (eveSMAxis *axis, *axisList) if (!axis->isDone()) allDone = false;
            if (allDone){
                sendNextPos();
                ++currentPosition;
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
                        axis->loadPositioner(manager->getPositionCount());
                        sendMessage(posMesg);
                    }
                }
                // go back to stage TRIGWAIT
                currentStage = eveStgTRIGWAIT;
                currentStageReady=false;
                currentStageCounter=0;
                sendError(INFO,0,"stgNextPos Done");
                emit sigExecStage();
            }
        }
    }
}

/**
 * \brief do all postscan actions
 *
 */
void eveScanModule::stgPostscan() {

    if (currentStageCounter == 0){
        sendError(DEBUG,0,"stgPostScan");
        currentStageCounter=1;
        signalCounter = 0;
        foreach (eveSMDevice *device, *postScanList){
            sendError(DEBUG,0,"stgPostScan writing");
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
            bool allDone = true;
            foreach (eveSMDevice *device, *postScanList) if (!device->isDone()) allDone = false;
            if (allDone) {
                sendError(DEBUG,0,"stgPostScan Done");
                currentStageReady=true;
                emit sigExecStage();
            }
        }
    }
}

/**
 * \brief execute position plugins
 *
 */
void eveScanModule::stgEndPos() {

    if (currentStageCounter == 0){
        sendError(DEBUG,0,"stgEndPos");
        currentStageCounter = 1;
        signalCounter = 0;
        foreach (eveSMAxis *axis, *axisList){
            if (axis->execPositioner()) ++signalCounter;
        }
        emit sigExecStage();
    }
    else {
        if (signalCounter > 0){
            --signalCounter;
        }
        else {
            bool allDone = true;
            foreach (eveSMAxis *axis, *axisList) if (axis->havePositioner() && !axis->isDone()) allDone = false;
            if (allDone){
                foreach (eveSMAxis *axis, *axisList){
                    if (axis->havePositioner()){
                        eveDataMessage* posMesg = axis->getPositionMessage();
                        if (posMesg != NULL){
                            bool ok;
                            double dval = posMesg->toVariant().toDouble(&ok);
                            if (ok) sendError(INFO,0,QString("%1: at position %2").arg(axis->getName()).arg(dval));
                        }
                        else
                            sendError(MINOR,0,QString("Positioning: %1: no position data available").arg(axis->getName()));
                    }
                }
                sendError(DEBUG, 0, "stgEndPos done");
                currentStageReady=true;
                emit sigExecStage();
            }
        }
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

    sendError(DEBUG,0,"stgFinish");
    if (appendedSM && (currentStageCounter == 0)) {
        currentStageCounter = 1;
        if (myStatus.setStatus(eveSmAPPEND)) manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
        appendedSM->startExec();
    }
    else {
        if (myStatus.getStatus() == eveSmAPPEND)
            currentStageReady=appendedSM->isDone();
        else
            currentStageReady=true;

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

    // wait with break until stage is ready
    if (currentStageReady && doBreak){
        currentStage = eveStgNEXTPOS;
        doBreak = false;
    }
    if (currentStage == eveStgFINISH) {
        if (currentStageReady){
            if (myStatus.setStatus(eveSmDONE)) manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
            emit SMready();
            return;
        }
        else
            (this->*stageHash.value(currentStage))();
    }
    // if executing, proceed with next stage, if stage is ready
    else if (myStatus.isExecuting()) {
        if (currentStageReady){
            if (!myStatus.isPaused()){
                currentStage = (stageT)(((int) currentStage)+1);
                currentStageReady = false;
                currentStageCounter = 0;
                (this->*stageHash.value(currentStage))();
            }
        }
        else
            (this->*stageHash.value(currentStage))();
    }
    else if ((currentStage == eveStgINIT) || (currentStage == eveStgREADPOS)) {
        // if not executing these stages need to be called from their parent,
        // execute only for root sm
        if (currentStageReady){
            if (smType == eveSmTypeROOT) {
                currentStage = (stageT)(((int) currentStage)+1);
                currentStageReady = false;
                currentStageCounter = 0;
                if (currentStage == eveStgINIT) (this->*stageHash.value(currentStage))();
            }
            else {
                emit SMready();
            }
        }
        else
            (this->*stageHash.value(currentStage))();
    }
    else if (currentStage == eveStgSTARTEXECUTING){
        // do nothing
    }
    return;
}

/**
 * set Status to eveSmEXECUTING which will make the states to proceed after init.
 *
 */
void eveScanModule::startExec() {

	if (myStatus.getStatus() == eveSmNOTSTARTED){
		sendError(DEBUG, 0, "starting scan");
		// always send status executing first
        myStatus.setStatus(eveSmEXECUTING);
        manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
		emit sigExecStage();
	}
    else if (myStatus.getStatus() == eveSmDONE){
    	sendError(DEBUG, 0, "restarting scan");
        if (myStatus.setStatus(eveSmEXECUTING)) manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
        currentStage = eveStgGOTOSTART;
        currentStageReady = false;
        currentStageCounter = 0;
        emit sigExecStage();
    }
}

/**
 *
 * @param evprop
 * @return
 */
bool eveScanModule::newEvent(eveEventProperty* evprop) {

	bool found = false;

	if (evprop->isChainAction()){
		sendError(DEBUG, 0, QString("new Chain Event"));
		switch (evprop->getActionType()){
		// REDO und PAUSE werden an alle SMs weitergereicht
		// HALT und STOP wird an alle SM weitergereicht
		// START wird an alle SMs geschickt, wenn root läuft, sonst nur an rootSM
		// BREAK wird nur an das innerste laufende geschickt
		// manueller Trigger ist auch ein chainEvent
		case eveEventProperty::HALT:
                        sendError(INFO, 0, QString("Chain Halt Event: %1").arg(evprop->getName()));
                        foreach (eveSMAxis *axis, *axisList){
				sendError(DEBUG, 0, QString("Stopping axis %1").arg(axis->getName()));
				axis->stop();
			}
			foreach (eveSMChannel *channel, *channelList){
				sendError(DEBUG, 0, QString("Stopping channel %1").arg(channel->getName()));
				channel->stop();
			}
		case eveEventProperty::STOP:
            if (nestedSM) nestedSM->newEvent(evprop);
			if ((myStatus.isExecuting()) || (myStatus.getStatus() == eveSmNOTSTARTED)){
                if(myStatus.forceExecuting()) manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                                sendError(INFO, 0, QString("Chain Stop/Halt Event: %1; Scan will end now").arg(evprop->getName()));
				currentStage = eveStgFINISH;
				currentStageReady = false;
				currentStageCounter = 1;
				emit sigExecStage();
			}
			else if ((appendedSM) && (myStatus.getStatus() == eveSmAPPEND))
				appendedSM->newEvent(evprop);
			break;
        case eveEventProperty::REDO:
            if (nestedSM) nestedSM->newEvent(evprop);
            if (myStatus.setEvent(evprop)) {
                if (evprop->getOn())
                    sendError(INFO, 0, QString("Chain Redo Event: %1; Pause Scan").arg(evprop->getName()));
                else
                    sendError(INFO, 0, QString("Chain Redo Event: %1; Resume Scan").arg(evprop->getName()));
                manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                emit sigExecStage();
            }
            if (appendedSM) appendedSM->newEvent(evprop);
            break;
        case eveEventProperty::PAUSE:
            if (nestedSM) nestedSM->newEvent(evprop);
            if (myStatus.setEvent(evprop)) {
                if (evprop->isSwitchOn())
                    sendError(INFO, 0, QString("Chain Pause Event: %1; Pause Scan").arg(evprop->getName()));
                if (evprop->isSwitchOff())
                    sendError(INFO, 0, QString("Chain Pause Event: %1; Resume Scan").arg(evprop->getName()));
                manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                emit sigExecStage();
            }
            if (appendedSM) appendedSM->newEvent(evprop);
            break;
        case eveEventProperty::START:
			if (myStatus.getStatus() == eveSmNOTSTARTED) {
				sendError(DEBUG, 0, QString("Starting Scan Module"));
				startExec();
			}
			break;
		case eveEventProperty::BREAK:
			if (myStatus.isExecuting()){
				if (nestedSM && nestedSM->isExecuting()){
                                        nestedSM->newEvent(evprop);
				}
				else {
                                        sendError(INFO, 0, QString("Chain Break Event: %1; Skip rest of ScanModule").arg(evprop->getName()));
					myStatus.setEvent(evprop);
					doBreak = true;
					emit sigExecStage();
				}
			}
			else if ((appendedSM) && (myStatus.getStatus() == eveSmAPPEND)){
				if (appendedSM) appendedSM->newEvent(evprop);
                        }
			break;
		case eveEventProperty::TRIGGER:
			if (myStatus.isExecuting()){
				if (nestedSM && nestedSM->isExecuting()){
					nestedSM->newEvent(evprop);
				}
				if (myStatus.setEvent(evprop)){
                    manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
					emit sigExecStage();
				}
			}
			else if ((appendedSM) && (myStatus.getStatus() == eveSmAPPEND)){
				if (appendedSM) appendedSM->newEvent(evprop);
			}
			break;
		default:
			break;
		}
	}
	else {
		sendError(DEBUG, 0, QString("new SM Event for id %1").arg(evprop->getSmId()));
		if (smId == evprop->getSmId()){
			found = true;
			switch (evprop->getActionType()){
			// REDO und PAUSE werden an alle SMs weitergereicht
			// START wird an alle ROOT-SMs geschickt, wenn root läuft
			// BREAK wird nur an das innerste laufende geschickt
			case eveEventProperty::REDO:
                            myStatus.setEvent(evprop);
                            emit sigExecStage();
                            break;
			case eveEventProperty::PAUSE:
                            if (myStatus.setEvent(evprop)) {
                                sendError(INFO, 0, QString("Pause Event: %1; Pause Scan").arg(evprop->getName()));
                                manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                                emit sigExecStage();
                            }
                            break;
			case eveEventProperty::START:
                            if (myStatus.setEvent(evprop)) {
                                manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                                emit sigExecStage();
                            }
                            break;
			case eveEventProperty::BREAK:
                            if (myStatus.isExecuting()){
                                sendError(INFO, 0, QString("Break Event: %1; Skip rest of ScanModule").arg(evprop->getName()));
                                myStatus.setEvent(evprop);
                                doBreak = true;
                                emit sigExecStage();
                            }
                            break;
			case eveEventProperty::TRIGGER:
                            if (myStatus.setEvent(evprop)) {
                                manager->setStatus(smId, myStatus.getStatus(), myStatus.getPause());
                                emit sigExecStage();
                            }
                            break;
                        default:
				break;
			}
		}
		if (!found && nestedSM) found = nestedSM->newEvent(evprop);
		if (!found && appendedSM) found = appendedSM->newEvent(evprop);
	}
	return found;
}


/**
 *
 * @param message to be sent
 */
void eveScanModule::sendMessage(eveMessage* message){

	if (message != NULL){
		if ((message->getType() == EVEMESSAGETYPE_DEVINFO) ||
				(message->getType() == EVEMESSAGETYPE_DATA)){
			((eveBaseDataMessage*)message)->setChainId(chainId);
			((eveBaseDataMessage*)message)->setSmId(smId);
            ((eveBaseDataMessage*)message)->setStorageHint(storageHint);
		}
		manager->sendMessage(message);
	}
	else
		sendError(ERROR, 0, "discarding empty message");
}

eveSMAxis* eveScanModule::findAxis(QString axisid){

	foreach (eveSMAxis *axis, *axisList){
		if (axis->getXmlId() == axisid)
		return axis;
	}
	return NULL;
}

void eveScanModule::setDoRedo(){
    doRedo = true;
    if (nestedSM) nestedSM->setDoRedo();
    if (appendedSM) appendedSM->setDoRedo();
}

int eveScanModule::getTotalSteps(){

        int totalSteps = expectedPositions;
        if (nestedSM) totalSteps *= nestedSM->getTotalSteps();
        if (appendedSM) totalSteps += appendedSM->getTotalSteps();
        sendError(DEBUG, 0, QString("Scan Module %1, total steps: %2").arg(smId).arg(totalSteps));
        return totalSteps;
}

void eveScanModule::sendError(int severity, int errorType,  QString message){

	sendError(severity, EVEMESSAGEFACILITY_SCANMODULE, errorType,  message);
}

void eveScanModule::sendError(int severity, int facility, int errorType, QString message){

	manager->sendError(severity, facility,
							errorType,  QString("ScanModule %1/%2: %3").arg(chainId).arg(smId).arg(message));
}
