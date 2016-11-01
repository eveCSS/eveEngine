/*
 * eveSMChannel.cpp
 *
 *  Created on: 04.06.2009
 *      Author: eden
 */

#include <stdio.h>
#include <cmath>
#include "eveSMChannel.h"
#include "eveEventRegisterMessage.h"
#include "eveError.h"
#include "eveScanModule.h"
#include "eveTimer.h"
#include "eveCounter.h"
#include "eveSMCounter.h"
#include "eveSMDetector.h"

#define checkNaN(val) (val != val)

/**
 *
 * normCalc = valueRaw/normRaw
 *
 * @param scanmodule QObject parent
 * @param definition corresponding detectorchannel definition
 * @return
 */
eveSMChannel::eveSMChannel(eveScanModule* scanmodule, eveSMDetector* smdetector, eveChannelDefinition* definition, QHash<QString, QString> parameter, QList<eveEventProperty* >* eventlist, eveSMChannel* normalizeWith)  :
    eveSMBaseDevice(scanmodule) {

    scanModule = scanmodule;
    signalCounter=0;
    haveValue = false;
    haveStop = false;
    haveTrigger = false;
    haveUnit = false;
    isDetectorTrigger = false;
    isDetectorUnit = false;
    isDetectorStop = false;
    isInterval = false;
    intervalRunning = false;
    triggerinterval = 0.0;
    delayedIntervalTrigger = false;
    deferredTrigger = false;
    name = definition->getName();
    xmlId = definition->getId();
    intervalTimer = NULL;
    valueTrans = NULL;
    stopTrans = NULL;
    triggerTrans = NULL;
    unitTrans = NULL;
    channelOK = false;
    ready = false;
    triggerValue = 1;
    valueRawMsg = NULL;
    normCalcMsg = NULL;
    normRawMsg = NULL;
    averageMsg = NULL;
    averageParamsMsg = NULL;
    stddevMsg = NULL;
    limitMsg = NULL;
    channelStatus = eveCHANNELINIT;
    channelType=definition->getChannelType();
    eventList = eventlist;
    unit = "";
    isTimer = false;
    // true if read timeout is <= 10s
    testAtInit = true;
    normalizeChannel = normalizeWith;
    detector = smdetector;
    sendreadyevent = false;
    delayedTrigger = false;
    redo = false;
    valueRaw = 0.0;
    normCalc = 0.0;
    normRaw = 0.0;
    longString = false;

    if ((definition->getValueCmd() != NULL) && (definition->getValueCmd()->getTrans()!= NULL)){
        eveTransportDefinition* transdef = (eveTransportDefinition*)definition->getValueCmd()->getTrans();
        if (transdef->getTransType() == eveTRANS_CA){
            valueTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getValueCmd()->getTrans());
            if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
            longString = ((eveCaTransport*) valueTrans)->getLongString();
        }
        else if (transdef->getTransType() == eveTRANS_LOCAL) {
            if (transdef->getName() == "Timer"){
                valueTrans = new eveTimer(this, xmlId, name, transdef);
                isTimer = true;
            }
            else if (transdef->getName() == "Counter"){
                valueTrans = new eveCounter(this, xmlId, name, transdef);
            }
            else if (transdef->getName() == "SM-Counter"){
                valueTrans = new eveSMCounter(scanModule, this, xmlId, name, transdef);
            }
        }
        if (transdef->getTimeout() > 10.0) testAtInit = false;
    }
    if (valueTrans != NULL)
        haveValue = true;
    else
        sendError(ERROR, 0, "Unknown Value Transport");

    if ((definition->getStopCmd() != NULL) && (definition->getStopCmd()->getTrans()!= NULL)){
        if (definition->getStopCmd()->getTrans()->getTransType() == eveTRANS_CA){
            stopTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getStopCmd()->getTrans());
            stopValue.setType(definition->getStopCmd()->getValueType());
            stopValue.setValue(definition->getStopCmd()->getValueString());
            if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
        }
    }
    else{
        stopTrans = detector->getStopTrans();
        stopValue = detector->getStopValue();
        isDetectorStop = true;
    }
    if (stopTrans != NULL) haveStop = true;

    if (definition->getTrigCmd() != NULL){
        if (definition->getTrigCmd()->getTrans()!= NULL){
            if (definition->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
                triggerTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getTrigCmd()->getTrans());
                triggerValue.setType(definition->getTrigCmd()->getValueType());
                triggerValue.setValue(definition->getTrigCmd()->getValueString());
                if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
            }
            if (definition->getTrigCmd()->getTrans()->getTimeout() > 10.0) testAtInit = false;
        }
    }
    else {
        triggerTrans = detector->getTrigTrans();
        triggerValue = detector->getTrigValue();
        isDetectorTrigger = true;
        if (detector->getTriggerTimeout() > 10.0) testAtInit = false;
    }
    if (triggerTrans != NULL) haveTrigger = true;

    if (definition->getUnitCmd() != NULL){
        if (definition->getUnitCmd()->getTrans()== NULL){
            unit = definition->getUnitCmd()->getValueString();
        }
        else if (definition->getUnitCmd()->getTrans()->getTransType() == eveTRANS_CA){
            unitTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getUnitCmd()->getTrans());
            if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
        }
    }
    else {
        unitTrans = detector->getUnitTrans();
        unit = detector->getUnitString();
        isDetectorUnit = true;
    }
    if (unitTrans != NULL) haveUnit = true;

    // evaluate parameter
    averageCount = 1;
    bool ok = true;
    if (parameter.contains("averagecount"))
        averageCount = parameter.value("averagecount").toInt(&ok);
    if (!ok) sendError(ERROR, 0, "Unable to evaluate averagecount");
    maxAttempts = 0x0FFFFFFF;
    ok = true;
    if (parameter.contains("maxattempts"))
        maxAttempts = parameter.value("maxattempts").toInt(&ok);
    if (!ok) sendError(ERROR, 0, "Unable to evaluate maxattempts");
    maxDeviation = 0.0;
    ok = true;
    if (parameter.contains("maxdeviation"))
        maxDeviation = parameter.value("maxdeviation").toDouble(&ok);
    if (!ok) sendError(ERROR, 0, "Unable to evaluate maxdeviation");
    minimum = 0.0;
    ok = true;
    if (parameter.contains("minimum"))
        minimum = parameter.value("minimum").toDouble(&ok);
    if (!ok) sendError(ERROR, 0, "Unable to evaluate minimum");
    if (parameter.contains("sendreadyevent"))
        sendreadyevent = (parameter.value("sendreadyevent").toLower() == "true");
    if (parameter.contains("deferredtrigger"))
        deferredTrigger = (parameter.value("deferredtrigger").toLower() == "true");
    if (parameter.contains("smchanneltype")) {
         if (parameter.value("smchanneltype").toLower() == "interval") isInterval = true;
    }
    ok = true;
    if (parameter.contains("triggerinterval")) {
        triggerinterval = parameter.value("triggerinterval").toDouble(&ok);
        if (!ok) sendError(ERROR, 0, "Unable to evaluate triggerinterval");
    }

    if (isInterval) {
        if (averageCount > 1) {
            sendError(ERROR, 0, "Unable to do average measurement with detector of type interval, switched to average count: 1");
            averageCount = 1;
        }
        testAtInit = false;
        intervalTimer = new QTimer(this);
        connect(intervalTimer, SIGNAL(timeout()), this, SLOT(intervalTimerReady()));
        intervalTimer->setSingleShot(true);
        intervalTimer->setInterval((int)(triggerinterval*1000.));
    }


    averageRaw = NULL;
    averageNormRaw = NULL;
    averageNormCalc = NULL;
    /*
 repeatOnRedo = false;
 if (parameter.contains("repeatonredo"))
  repeatOnRedo = parameter.value("repeatonredo").startsWith("true",Qt::CaseInsensitive);
 if (!ok) sendError(ERROR, 0, "Unable to evaluate repeatonredo");
*/
}

eveSMChannel::~eveSMChannel() {
    try
    {
        if (normalizeChannel){
            delete normalizeChannel;
            normalizeChannel = NULL;
        }
        if (haveValue) {
            delete valueTrans;
            haveValue = false;
        }
        if (haveStop && !isDetectorStop) {
            delete stopTrans;
            haveStop = false;
        }
        if (haveTrigger && !isDetectorTrigger) {
            delete triggerTrans;
            haveTrigger = false;
        }
        if (haveUnit && !isDetectorUnit) {
            delete unitTrans;
            haveUnit = false;
        }

        foreach (eveEventProperty* evprop, *eventList){
            // may not work, if events are already deleted
            // disconnect(evprop, SIGNAL(signalEvent(eveEventProperty*)), this, SLOT(newEvent(eveEventProperty*)));
            eveEventRegisterMessage* regmessage = new eveEventRegisterMessage(false, evprop);
            scanModule->sendMessage(regmessage);
        }
        delete eventList;
    }
    catch (std::exception& e)
    {
        //printf("C++ Exception %s\n",e.what());
        sendError(FATAL, 0, QString("C++ Exception in ~eveSMChannel %1").arg(e.what()));
    }
}

/**
 * \brief initialization (must be done in the correct thread)
 */
void eveSMChannel::init() {

    signalCounter = 0;
    ready = false;

    if (normalizeChannel){
        connect (normalizeChannel, SIGNAL(channelDone()), this, SLOT(normalizeChannelReady()), Qt::QueuedConnection);
        normalizeChannel->init();
        ++signalCounter;
    }
    if (haveValue){
        connect (valueTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
        ++signalCounter;
        valueTrans->connectTrans();
    }
    if (haveStop){
        connect (stopTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
        ++signalCounter;
        stopTrans->connectTrans();
    }
    if (haveTrigger){
        connect (triggerTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
        ++signalCounter;
        triggerTrans->connectTrans();
    }
    if (haveUnit){
        connect (unitTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
        ++signalCounter;
        unitTrans->connectTrans();
    }

    foreach (eveEventProperty* evprop, *eventList){
        sendError(DEBUG, 0, QString("registering detector event %1").arg(evprop->getName()));
        connect(evprop, SIGNAL(signalEvent(eveEventProperty*)), this, SLOT(newEvent(eveEventProperty*)), Qt::QueuedConnection);
        eveEventRegisterMessage* regmessage = new eveEventRegisterMessage(true, evprop);
        scanModule->sendMessage(regmessage);
    }

}

/**
 * \brief complete initialization
 *
 * called if all transports are done
 */
void eveSMChannel::initAll() {

    if (haveValue && !valueTrans->isConnected()) {
        haveValue=false;
        sendError(ERROR, 0, "Unable to connect Value Transport");
    }
    if (haveStop && !stopTrans->isConnected()) {
        haveStop=false;
        sendError(ERROR, 0, "Unable to connect Stop Transport");
    }
    if (haveTrigger && !triggerTrans->isConnected()) {
        haveTrigger=false;
        sendError(ERROR, 0, "Unable to connect Trigger Transport");
    }
    if (haveUnit && !unitTrans->isConnected()) {
        haveUnit=false;
        sendError(ERROR, 0, "Unable to connect Unit Transport");
    }
    if ((normalizeChannel) && (!normalizeChannel->isOK())){
        sendError(ERROR, 0, QString("Unable to use channel %1 for normalization").arg(normalizeChannel->getName()));
        delete normalizeChannel;
        normalizeChannel = NULL;
    }

    if (haveValue) channelOK = true;

    if (averageCount > 1){
        averageRaw = new eveAverage(averageCount, maxAttempts, minimum, maxDeviation);
        if (normalizeChannel){
            averageNormRaw = new eveAverage(averageCount, 0, 0.0, 0.0);
            averageNormCalc = new eveAverage(averageCount, 0, 0.0, 0.0);
        }
    }
}

/**
 * \brief is usually called by underlying transport
 * @param status	0 = success, 1 = error
 */
void eveSMChannel::transportReady(int status) {

    if (channelStatus == eveCHANNELINIT){
        if (status != 0) sendError(ERROR,0,"Error while connecting");
        --signalCounter;
        if (signalCounter <= 0) {
            initAll();
            if (haveUnit){
                channelStatus = eveCHANNELREADUNIT;
                if (unitTrans->readData(false)){
                    sendError(ERROR,0,"error reading units");
                    transportReady(1);
                }
            }
            else {
                channelStatus = eveCHANNELIDLE;
                signalReady();
            }
        }
    }
    else if (channelStatus == eveCHANNELREADUNIT){
        if ((status == 0) && unitTrans->haveData()) {
            eveDataMessage *unitData = unitTrans->getData();
            if (unitData == NULL)
                sendError(ERROR, 0, "unable to read unit");
            else {
                unit = unitData->toVariant().toString();
                delete unitData;
            }
        }
        channelStatus = eveCHANNELIDLE;
        signalReady();
    }
    else if (channelStatus == eveCHANNELTRIGGERREAD){
        --signalCounter;
        if (signalCounter <= 0) {
            // trigger is ready, we call read
            if (status == 0) {
                read(false);
            }
            else {
                sendError(ERROR, 0, "trigger error");
                signalReady();
            }
        }
    }
    else if (channelStatus == eveCHANNELREAD){
        --signalCounter;
        bool normalizeCheck = ((!(normalizeChannel)) || ((normalizeChannel) && (normalizeChannel->isDone())));
        if ((signalCounter <= 0) && normalizeCheck) {
            retrieveData();

            channelStatus = eveCHANNELIDLE;

            if (redo){
                // for now we repeat average calculations from start
                // if needed we could just repeat the last detector value
                if (averageCount > 1) {
                    averageRaw->reset();
                    if (averageNormCalc) averageNormCalc->reset();
                    if (averageNormRaw) averageNormRaw->reset();
                    sendError(INFO, 0, QString("Channel %1 (%2), got redo event, beginning a new average measurement").arg(name).arg(xmlId));
                }
                else if (isInterval){
                    intervalVariance.Clear();
                    sendError(INFO, 0, QString("Channel %1 (%2), got redo event, beginning a new interval measurement").arg(name).arg(xmlId));
                }
                else
                    sendError(INFO, 0, QString("Channel %1 (%2), got redo event, beginning a new measurement").arg(name).arg(xmlId));

                triggerRead(false);
            }
            else if (valueRawMsg == NULL){
                // message and bail out
                sendError(ERROR, 0, "unable to read current value");
                signalReady();
            }
            else if (averageCount > 1){
                // we need to do average measurements
                if (normalizeChannel){
                    if (checkNaN(valueRaw) || checkNaN(normCalc) || checkNaN(normRaw)){
                        sendError(ERROR, 0, QString("Channel %1 (%2) or its normalized value is NaN").arg(name).arg(xmlId));
                    }
                    else if (averageRaw->addValue(valueRaw)){
                        averageNormCalc->addValue(normCalc);
                        averageNormRaw->addValue(normRaw);
                    }
                    else {
                        sendError(MINOR, 0, QString("Channel %1 (%2), missed deviation test (normalization active)").arg(name).arg(xmlId));
                        averageNormCalc->replaceValue(normCalc, 0);
                        averageNormRaw->replaceValue(normRaw, 0);
                    }
                }
                else {
                    if (checkNaN(valueRaw))
                        sendError(MINOR, 0, QString("Channel %1 (%2), value is NaN").arg(name).arg(xmlId));
                    else if (!averageRaw->addValue(valueRaw))
                        sendError(MINOR, 0, QString("Channel %1 (%2), missed deviation test").arg(name).arg(xmlId));
                }

                if(averageRaw->isDone()){
                    // ready with average measurements
                    if (valueRawMsg != NULL) delete valueRawMsg;
                    valueRawMsg = averageRaw->getResultMessage();
                    valueRawMsg->setXmlId(xmlId);
                    valueRawMsg->setName(name);

                    QVector<int> averageArr(2);
                    averageArr[0] = averageCount;
                    averageArr[1] = averageRaw->getAttempts();
                    if (averageMsg != NULL) delete averageMsg;
                    averageMsg = new eveDataMessage(xmlId, "Attempts", eveDataStatus(), DMTaverageParams, eveTime::getCurrent(), averageArr);
                    averageMsg->setAuxString("AverageCount");
                    QVector<int> averageParamsArr(1);
                    averageParamsArr[0] = maxAttempts;
                    if (averageParamsMsg != NULL) delete averageParamsMsg;
                    averageParamsMsg = new eveDataMessage(xmlId, "MaxAttempts", eveDataStatus(), DMTaverageParams, eveTime::getCurrent(), averageParamsArr);
                    averageParamsMsg->setAuxString("MaxAttempts");
                    if (maxDeviation > 0.0) {
                        QVector<double> deviationArr(2);
                        deviationArr[0] = minimum;
                        deviationArr[1] = maxDeviation;
                        limitMsg = new eveDataMessage(xmlId, "maxDeviation", eveDataStatus(), DMTaverageParams, eveTime::getCurrent(), deviationArr);
                        limitMsg->setAuxString("Limit");
                    }
                    valueRaw = valueRawMsg->toVariant().toDouble();
                    if (normalizeChannel){
                        normCalcMsg = averageNormCalc->getResultMessage();
                        normCalcMsg->setXmlId(xmlId);
                        normCalcMsg->setName(name);
                        normCalcMsg->setNormalizeId(normalizeChannel->getXmlId());
                        normCalcMsg->setDataMod(DMTnormalized);
                        normCalc = normCalcMsg->toVariant().toDouble();
                        normRawMsg = averageNormRaw->getResultMessage();
                        normRawMsg->setXmlId(normalizeChannel->getXmlId());
                        normRawMsg->setName(normalizeChannel->getName());
                        normRaw = normRawMsg->toVariant().toDouble();

                        sendError(DEBUG, 0, QString("Channel %1, Average Value %2, Average Normalized %3, Average channel %4 is %5").arg(xmlId).arg(valueRaw).arg(normCalc).arg(normRawMsg->getName()).arg(normRaw));
                        averageNormCalc->reset();
                        averageNormRaw->reset();
                    }
                    else
                        sendError(DEBUG, 0, QString("Channel %1, Average Value %2").arg(xmlId).arg(valueRaw));

                    averageRaw->reset();
                    signalReady();
                }
                else
                    triggerRead(false);
            }
            else if (isInterval){
                sendError(DEBUG, 0, QString("Interval Channel %1, addValue").arg(xmlId));
                if (normalizeChannel)
                    intervalVariance.Push(normCalc);
                else
                    intervalVariance.Push(valueRaw);

                if (intervalRunning) {
                    if (delayedIntervalTrigger){
                        delayedIntervalTrigger = false;
                        triggerRead(false);
                    }
                }
                else
                    intervalComplete();
            }
            else {
                valueRawMsg->setXmlId(xmlId);
                valueRawMsg->setName(name);
                if (normalizeChannel){
                    normCalcMsg = new eveDataMessage(xmlId, name, valueRawMsg->getDataStatus(), DMTnormalized, valueRawMsg->getDataTimeStamp(), QVector<double>(1, normCalc));
                    normCalcMsg->setNormalizeId(normalizeChannel->getXmlId());
                }
                signalReady();
            }
        }
    }
}
void eveSMChannel::intervalComplete() {
    QVector<double> dblArr1(1);
    QVector<double> dblArr2(2);
    dblArr1[0] = intervalVariance.Mean();
    if (valueRawMsg != NULL) delete valueRawMsg;
    valueRawMsg = new eveDataMessage(xmlId, name, eveDataStatus(), DMTunmodified, eveTime::getCurrent(), dblArr1);
    if (normalizeChannel){
        valueRawMsg->setNormalizeId(normalizeChannel->getXmlId());
        valueRawMsg->setDataMod(DMTnormalized);
    }
    dblArr2[0] = (double)intervalVariance.NumDataValues();
    dblArr2[1] = intervalVariance.StandardDeviation();
    stddevMsg = new eveDataMessage(xmlId, name, eveDataStatus(), DMTstandarddev, eveTime::getCurrent(), dblArr2);
    stddevMsg->setAuxString("Count");
    sendError(DEBUG, 0, QString("Interval Channel %1, Mean %2, Stddev %3, Counts %4 ").arg(xmlId).arg(dblArr1[0]).arg(dblArr2[1]).arg(dblArr2[0]));
    intervalVariance.Clear();
    if (normCalcMsg != NULL){
        delete normCalcMsg;
        normCalcMsg = NULL;
    }
    if (normRawMsg != NULL) {
        delete normRawMsg;
        normRawMsg = NULL;
    }
    signalReady();
}

void eveSMChannel::intervalTimerReady() {

    if(intervalRunning) {
        sendError(DEBUG, 0, QString("called intervalTimerReady: %1").arg(name));
        if (channelStatus != eveCHANNELIDLE){
            // device is not ready yet, delay trigger
            delayedIntervalTrigger = true;
        }
        else {
            triggerRead(false);
        }
    }
}

/**
 * \brief signal channelDone if ready
 */
void eveSMChannel::signalReady() {
    ready = true;
    sendError(DEBUG, 0, "is done");
    emit channelDone();
    if (sendreadyevent) {
        QString eventId = QString("D-%1-%2-%3").arg(scanModule->getChainId()).arg(scanModule->getSmId()).arg(xmlId);
        scanModule->sendMessage(new eveMessageText(EVEMESSAGETYPE_DETECTORREADY, eventId));
        sendError(DEBUG, 0, QString("sending detector ready for %1").arg(eventId));
    }
}

/**
 * \brief combined trigger and read sequence
 * @param queue if true queue the request, if false send it immediately
 *
 * if detector has a trigger this method triggers the detector
 *  and reads the detector when trigger is ready
 *  without trigger just read
 * deviceDone is always signaled
 */
void eveSMChannel::triggerRead(bool queue) {

    ready = false;
    if (channelOK && (channelStatus == eveCHANNELIDLE)) {
        if (redo){
            delayedTrigger = true;
        }
        else {
            signalCounter = 1;
            if (normalizeChannel) {
                ++signalCounter;
                normalizeChannel->triggerRead(queue);
            }
            if (haveTrigger){
                channelStatus = eveCHANNELTRIGGERREAD;
                if (triggerTrans->writeData(triggerValue, queue)) {
                    sendError(ERROR,0,"error triggering");
                    transportReady(1);
                }
            }
            else {
                read(queue);
            }
            if (isInterval) {
                sendError(DEBUG, 0, QString("Trigger Interval Channel %1").arg(xmlId));
                intervalTimer->start();
                intervalRunning = true;
            }
        }
    }
    else {
        sendError(ERROR,0,"not operational");
        signalReady();
    }
}

/**
 * \brief read detector value
 * @param queue if true queue the request, if false send it immediately
 *
 * deviceDone is always signaled,
 * method is private since redo is done properly only by triggerRead
 *
 */
void eveSMChannel::read(bool queue) {

    if (channelOK) {
        ready = false;
        channelStatus = eveCHANNELREAD;
        if (valueTrans->readData(queue)) {
            sendError(ERROR,0,"error reading value");
            transportReady(1);
        }
    }
    else {
        sendError(ERROR,0,"not operational");
        signalReady();
    }
}

/**
 * \brief stop a running measurement
 *
 * deviceDone is always signaled
 */
void eveSMChannel::stop() {

    if (channelOK && haveStop && (channelStatus != eveCHANNELIDLE)){
        channelStatus = eveCHANNELSTOP;
        if (stopTrans->writeData(stopValue, false)){
            sendError(ERROR,0,"error stopping channel");
            transportReady(1);
        }
    }
}

/**
 *
 * @return current channel value or NULL
 */
eveDataMessage* eveSMChannel::getValueMessage(resultType rtype){

    eveDataMessage* return_data = valueRawMsg;
    switch (rtype) {
    case RAWVAL:
        return_data = valueRawMsg;
        valueRawMsg = NULL;
        break;
    case NORMVAL:
        return_data = normCalcMsg;
        normCalcMsg = NULL;
        break;
    case NORMRAW:
        return_data = normRawMsg;
        normRawMsg = NULL;
        break;
    case AVERAGE:
        return_data = averageMsg;
        averageMsg = NULL;
        break;
    case AVERAGEPARAMS:
        return_data = averageParamsMsg;
        averageParamsMsg = NULL;
        break;
    case LIMIT:
        return_data = limitMsg;
        limitMsg = NULL;
        break;
    case STDDEV:
        return_data = stddevMsg;
        stddevMsg = NULL;
        break;
    default:
        break;
    }

    return return_data;
}

/**
 *
 * @return true
 */
bool eveSMChannel::retrieveData(){

    if (valueRawMsg != NULL) delete valueRawMsg;
    if (normCalcMsg != NULL) delete normCalcMsg;
    if (normRawMsg != NULL) delete normRawMsg;

    normRawMsg = NULL;
    normCalcMsg = NULL;

    valueRawMsg = valueTrans->getData();
    if (valueRawMsg == NULL) {
        sendError(ERROR,0,QString("unable to retrieve value of Channel %1").arg(xmlId));
        return true;
    }
    if ((averageCount > 1) && (valueRawMsg->getArraySize() > 1)) {
        sendError(ERROR,0,QString("unable to do average calculations with array data: %1").arg(xmlId));
        averageCount = 1;
    }
    if ((normalizeChannel) && (valueRawMsg->getArraySize() > 1)) {
        sendError(ERROR,0,QString("unable to do normalisation with array data: %1").arg(xmlId));
        delete normalizeChannel;
        normalizeChannel=NULL;
    }

    valueRaw = valueRawMsg->toVariant().toDouble();
    sendError(DEBUG, 0, QString("Channel %1, Raw Value %2").arg(xmlId).arg(valueRaw));

    if (normalizeChannel){
        normRaw = 1.0;
        normCalc = NAN;

        normRawMsg = normalizeChannel->getValueMessage(RAWVAL);
        if (normRawMsg == NULL){
            sendError(ERROR,0,QString("unable to retrieve value of normalized Channel %1").arg(normalizeChannel->getXmlId()));
        }
        else if (normRawMsg->getArraySize() > 1){
            sendError(ERROR,0,QString("normalize Channel may not have array data%1").arg(normalizeChannel->getXmlId()));
        }
        else {
            bool ok = true;
            normRaw = normRawMsg->toVariant().toDouble(&ok);
            if (!ok){
                sendError(ERROR,0,QString("error converting normalize Channel to type double %1").arg(normalizeChannel->getXmlId()));
            }
            else if (normRaw == 0.0) {
                sendError(ERROR,0,QString("normalize Channel has value zero %1").arg(normalizeChannel->getXmlId()));
            }
            else
                normCalc =  valueRaw/normRaw;
        }
        sendError(DEBUG, 0, QString("Normalize Channel %1, Raw Value %2").arg(normalizeChannel->getXmlId()).arg(normRaw));
        sendError(DEBUG, 0, QString("Channel %1, Normalized Value %2").arg(xmlId).arg(normCalc));
    }
    return true;
}

/**
 *
 * @return pointer to a message with all info about this channel
 */
eveDevInfoMessage* eveSMChannel::getDeviceInfo(bool normalizeInfo){

    QStringList* sl;
    QString auxInfo = "";
    eveDataModType dataMod = DMTunmodified;

    if (haveValue)
        sl = valueTrans->getInfo();
    else
        sl = new QStringList();

    sl->prepend(QString("Name:%1").arg(name));
    sl->prepend(QString("XML-ID:%1").arg(xmlId));
    sl->append(QString("Unit:%1").arg(unit));
    sl->append(QString("DeviceType:Channel"));
    if (valueRawMsg != NULL){
        sl->append(QString("Value:%1").arg(valueRawMsg->toVariant().toString()));
    }
    if (averageCount > 1) {
        sl->append(QString("SelectedAverageCount:%1").arg(averageCount));
        sl->append(QString("MaxAttempts:%1").arg(maxAttempts));
        sl->append(QString("MaxDeviation:%1").arg(maxDeviation));
        sl->append(QString("Minimum:%1").arg(minimum));
    }
    if (isInterval) {
        sl->append(QString("Detectortype:Interval"));
        sl->append(QString("Triggerinterval:%1").arg(triggerinterval));
    }
    else {
        sl->append(QString("Detectortype:Standard"));
    }
    if (normalizeChannel && normalizeInfo){
        sl->append(QString("NormalizeChannelID:%1").arg(normalizeChannel->getXmlId()));
        dataMod = DMTnormalized;
        auxInfo = normalizeChannel->getXmlId();
    }
    if (longString)sl->append(QString("longString:true"));

    return new eveDevInfoMessage(xmlId, name, sl, dataMod, auxInfo);
}

void eveSMChannel::loadPositioner(int pc){
    foreach (eveCalc* positioner, positionerList){
        positioner->addValue(xmlId, pc, eveVariant(valueRaw));
        if (normalizeChannel) {
            if (positioner->getNormalizeExternal())
                positioner->addValue(normalizeChannel->getXmlId(), pc, eveVariant(normCalc));
            else
                positioner->addValue(normalizeChannel->getXmlId(), pc, eveVariant(normRaw));
        }
    }
}


void eveSMChannel::newEvent(eveEventProperty* evprop) {

    if (evprop == NULL){
        sendError(ERROR,0,"invalid event property");
        return;
    }

    if (evprop->getActionType() == eveEventProperty::REDO){
        if (evprop->getOn()){
            sendError(DEBUG, 0, "received redo start event");
            redo = true;
        }
        else {
            sendError(DEBUG, 0, "received redo done event");
            redo = false;
            if (delayedTrigger){
                delayedTrigger = false;
                triggerRead(false);
            }
        }
    }
    else if (evprop->getActionType() == eveEventProperty::STOP){
        if(intervalRunning) {
            intervalRunning = false;
            if (channelStatus == eveCHANNELIDLE)
                intervalComplete();
        }
    }
}

void eveSMChannel::setTimer(QDateTime start) {
    if (isTimer){
        ((eveTimer*)valueTrans)->setStartTime(start);
    }
    if (normalizeChannel) normalizeChannel->setTimer(start);
}

void eveSMChannel::normalizeChannelReady(){
    sendError(DEBUG, 0, "received normalize channel ready");
    emit transportReady(0);
}

/**
 * \brief send an error message to display
 * @param severity
 * @param errorType
 * @param message
 */
void eveSMChannel::sendError(int severity, int errorType,  QString message){

    scanModule->sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
                          errorType,  QString("DetectorChannel %1: %2").arg(name).arg(message));
}

void eveSMChannel::sendError(int severity, int facility, int errorType, QString message){
    scanModule->sendError(severity, facility, errorType, message);
}
