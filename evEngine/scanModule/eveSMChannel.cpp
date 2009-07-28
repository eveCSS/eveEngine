/*
 * eveSMChannel.cpp
 *
 *  Created on: 04.06.2009
 *      Author: eden
 */

#include "eveSMChannel.h"
#include "eveError.h"
#include "eveScanModule.h"

/**
 *
 * @param scanmodule QObject parent
 * @param definition corresponding detectorchannel definition
 * @return
 */
eveSMChannel::eveSMChannel(eveScanModule* scanmodule, eveDetectorChannel* definition, QHash<QString, QString> parameter)  :
	QObject(scanmodule) {

	scanModule = scanmodule;
	signalCounter=0;
	haveValue = false;
	haveStop = false;
	haveTrigger = false;
	haveUnit = false;
	name = definition->getName();
	valueTrans = NULL;
	stopTrans = NULL;
	triggerTrans = NULL;
	unitTrans = NULL;
	channelOK = false;
	ready = false;
	triggerValue = 1;
	curValue = NULL;
	channelStatus = eveCHANNELINIT;
	channelType=definition->getChannelType();

	if (definition->getValueCmd() != NULL){
		if (definition->getValueCmd()->getTrans()->getTransType() == eveTRANS_CA){
			valueTrans = new eveCaTransport(this, name, (eveCaTransportDef*)definition->getValueCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (valueTrans != NULL)
		haveValue = true;
	else
		sendError(ERROR, 0, "Unknown Value Transport");

	if (definition->getStopCmd() != NULL){
		if (definition->getStopCmd()->getTrans()->getTransType() == eveTRANS_CA){
			stopTrans = new eveCaTransport(this, name, (eveCaTransportDef*)definition->getStopCmd()->getTrans());
			stopValue.setType(definition->getStopCmd()->getValueType());
			stopValue.setValue(definition->getStopCmd()->getValueString());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (stopTrans != NULL) haveStop = true;

	if (definition->getTrigCmd() != NULL){
		if (definition->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
			triggerTrans = new eveCaTransport(this, name, (eveCaTransportDef*)definition->getTrigCmd()->getTrans());
			triggerValue.setType(definition->getTrigCmd()->getValueType());
			triggerValue.setValue(definition->getTrigCmd()->getValueString());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (triggerTrans != NULL) haveTrigger = true;

	if (definition->getUnitCmd() != NULL){
		if (definition->getUnitCmd()->getTrans()->getTransType() == eveTRANS_CA){
			unitTrans = new eveCaTransport(this, name, (eveCaTransportDef*)definition->getUnitCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (unitTrans != NULL) haveUnit = true;

	// evaluate parameter
	averageCount = 1;
	bool ok = true;
	if (parameter.contains("averagecount"))
		averageCount = parameter.value("averagecount").toInt(&ok);
	if (!ok) sendError(ERROR, 0, "Unable to evaluate averagecount");
	maxAttempts = 0;
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
	confirmTrigger = false;
	if (parameter.contains("confirmtrigger"))
		confirmTrigger = parameter.value("confirmtrigger").startsWith("true",Qt::CaseInsensitive);
	if (!ok) sendError(ERROR, 0, "Unable to evaluate confirmtrigger");
	repeatOnRedo = false;
	if (parameter.contains("repeatonredo"))
		repeatOnRedo = parameter.value("repeatonredo").startsWith("true",Qt::CaseInsensitive);
	if (!ok) sendError(ERROR, 0, "Unable to evaluate repeatonredo");
}

eveSMChannel::~eveSMChannel() {
	try
	{
		if (haveValue) delete valueTrans;
		if (haveStop) delete stopTrans;
		if (haveTrigger) delete triggerTrans;
		if (haveUnit) delete unitTrans;
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL, 0, QString("C++ Exception in ~eveSMChannel %1").arg(e.what()));
	}
}

/**
 * \brief initialization (must be done in the correct thread)
 */
void eveSMChannel::init() {

	signalCounter = 0;
	ready = false;

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
	//TODO check if all are done
	if (haveValue) channelOK = true;
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
				// TODO for now we delete the position message and extended information
				delete unitData;
			}
		}
		channelStatus = eveCHANNELIDLE;
		signalReady();
	}
	else if (channelStatus == eveCHANNELTRIGGER){
		// trigger is ready, we do nothing
		signalReady();
	}
	else if (channelStatus == eveCHANNELTRIGGERREAD){
		// trigger is ready, we call read
		if (status == 0) {
			read(false);
		}
		else {
			sendError(ERROR, 0, "error while triggering");
			signalReady();
		}
	}
	else if (channelStatus == eveCHANNELREAD){
		if ((status == 0) && valueTrans->haveData()){
			if (curValue != NULL) delete curValue;
			curValue = valueTrans->getData();
		}
		if (curValue == NULL)
			sendError(ERROR, 0, "unable to read current value");
		channelStatus = eveCHANNELIDLE;
		signalReady();
	}
}

/**
 * \brief signal channelDone if ready
 */
void eveSMChannel::signalReady() {
	ready = true;
	sendError(INFO, 0, "is done");
	emit channelDone();
}

/**
 * \brief trigger detector if it has a trigger command, else read
 * @param queue if true queue the request, if false send it immediately
 *
 * if detector has a trigger this method only triggers the detector,
 *   read must be called explicitly
 * deviceDone is always signaled
 */
void eveSMChannel::trigger(bool queue) {

	if (channelOK) {
		ready = false;
		if (haveTrigger){
			channelStatus = eveCHANNELTRIGGER;
			if (triggerTrans->writeData(triggerValue, queue)) {
				sendError(ERROR,0,"error triggering");
				transportReady(1);
			}
		}
		else {
			read(queue);
		}
	}
	else {
		sendError(ERROR,0,"not operational");
		signalReady();
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
	if (channelOK) {
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
 * deviceDone is always signaled
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
 * @param queue if true queue the request, if false send it immediately
 *
 * deviceDone is always signaled
 */
void eveSMChannel::stop(bool queue) {

	if (channelOK && haveStop && (channelStatus != eveCHANNELIDLE)){
		channelStatus = eveCHANNELSTOP;
		if (stopTrans->writeData(stopValue, queue)){
			sendError(ERROR,0,"error stopping channel");
			transportReady(1);
		}
	}
}

/**
 *
 * @return current detector value
 */
eveDataMessage* eveSMChannel::getValueMessage(){
	eveDataMessage* return_data = curValue;
	curValue = NULL;
	return return_data;
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
