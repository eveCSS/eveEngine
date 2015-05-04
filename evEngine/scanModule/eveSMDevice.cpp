/*
 * eveSMDevice.cpp
 *
 *  Created on: 04.02.2009
 *      Author: eden
 */

#include "eveSMDevice.h"
#include "eveError.h"
#include "eveScanModule.h"

/**
 *
 * @param scanmodule QObject parent
 * @param definition corresponding device definition
 * @param writeval what to write to the device
 * @param resetPrevious if true, we read the value in prescan before writing and reset in postscan
 * @return
 */
eveSMDevice::eveSMDevice(eveScanModule* scanmodule, eveDeviceDefinition* definition, eveVariant writeval, bool resetPrevious)  :
    eveSMBaseDevice(scanmodule) {

    setPrevious = resetPrevious;
    value = writeval;
    scanModule = scanmodule;
    deviceStatus = eveDEVICEINIT;
    signalCounter=0;
    haveResetValue = false;
    name = definition->getName();
    xmlId = definition->getId();
    ready = false;
    deviceOK = false;
    valueTrans = NULL;

    if ((definition->getValueCmd() != NULL) && (definition->getValueCmd()->getTrans()!= NULL)){
        if (definition->getValueCmd()->getTrans()->getTransType() == eveTRANS_CA){
            valueTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getValueCmd()->getTrans());
            if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
        }
    }
    if (valueTrans != NULL)
        haveValue = true;
    else
        sendError(ERROR, 0, "Unknown Value Transport");

    // do we have units?
    // so far we don't care

}

eveSMDevice::~eveSMDevice() {
    if (haveValue) {
        try
        {
            delete valueTrans;
        }
        catch (std::exception& e)
        {
            sendError(FATAL, 0, QString("C++ Exception in ~eveSMDevice %1").arg(e.what()));
        }
    }
}

/**
 * \brief initialization (must be done in the correct thread)
 */
void eveSMDevice::init() {

    signalCounter = 0;
    ready = false;
    if (haveValue){
        connect (valueTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
        ++signalCounter;
        valueTrans->connectTrans();
    }
}

/**
 * \brief is usually called by underlying transport
 * @param status	0: success, 1: error
 */
void eveSMDevice::transportReady(int status) {

    if (deviceStatus == eveDEVICEINIT){
        if (status != 0) sendError(ERROR,0,"Error while connecting");
        --signalCounter;
        if (signalCounter <= 0) {
            deviceStatus = eveDEVICEIDLE;
            deviceOK = true;
            if (haveValue && !valueTrans->isConnected()) {
                haveValue=false;
                deviceOK = false;
                delete valueTrans;
                sendError(ERROR, 0, "Unable to connect Value Transport");
            }
            ready = true;
            emit deviceDone();
        }
    }
    else if (deviceStatus == eveDEVICEREAD){
        if ((status == 0) && valueTrans->haveData()){
            value = valueTrans->getData()->toVariant();
            haveResetValue = true;
        }
        ready = true;
        deviceStatus = eveDEVICEIDLE;
        emit deviceDone();
    }
    else if (deviceStatus == eveDEVICEWRITE){
        if (status != 0)
            sendError(ERROR,0,"Error while writing");
        ready = true;
        deviceStatus = eveDEVICEIDLE;
        emit deviceDone();
    }
    else{
        sendError(MINOR, 0, "transport signaled ready to idle device");
    }
}

/**
 * \brief read the current value only if we need it to reset to original value
 * @param queue if true queue the request, if false send it immediately
 *
 * deviceDone is always signaled
 */
void eveSMDevice::readValue(bool queue) {

    ready = true;
    if (haveValue){
        if (setPrevious){
            ready = false;
            deviceStatus = eveDEVICEREAD;
            // trigger read of current value first
            if (valueTrans->readData(queue)){
                sendError(ERROR,0,"error reading current value");
                transportReady(1);
            }
        }
        else {
            sendError(INFO,0,"not reading, reset_originalvalue not set");
            emit deviceDone();
        }
    }
    else {
        sendError(ERROR,0,"device not operational");
        emit deviceDone();
    }
}

/**
 * \brief write the stored value to target
 * @param queue if true queue the request, if false send it immediately
 *
 * deviceDone is always signaled
 */
void eveSMDevice::writeValue(bool queue) {

    ready = true;
    if (haveValue){
        if (deviceStatus == eveDEVICEIDLE){
            if (value.isValid()) {
                deviceStatus = eveDEVICEWRITE;
                ready = false;
                if (valueTrans->writeData(value, queue)){
                    sendError(ERROR, 0, "error resetting to original value");
                    transportReady(1);
                }
            }
            else {
                bool ok = false;
                double newValue = value.toDouble(&ok);
                sendError(ERROR, 0, QString("invalid set value: %1 (%2)").arg(newValue).arg(ok));
                emit deviceDone();
            }
        }
        else {
            sendError(ERROR, 0, "unable to write, device is busy");
            emit deviceDone();
        }
    }
    else {
        sendError(ERROR,0,"device not operational");
        emit deviceDone();
    }
}

/**
 * \brief send an error message to display
 * @param severity
 * @param errorType
 * @param message
 */
void eveSMDevice::sendError(int severity, int errorType,  QString message){

    sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
              errorType,  QString("Device %1: %2").arg(name).arg(message));
}

void eveSMDevice::sendError(int severity, int facility, int errorType, QString message){
    scanModule->sendError(severity, facility, errorType, message);
}
