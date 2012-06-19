/*
 * eveSMChannel.cpp
 *
 *  Created on: 04.06.2009
 *      Author: eden
 */

#include <stdio.h>
#include "eveSMChannel.h"
#include "eveEventRegisterMessage.h"
#include "eveError.h"
#include "eveScanModule.h"
#include "eveTimer.h"
#include "eveCounter.h"
#include "eveSMDetector.h"

/**
 *
 * @param scanmodule QObject parent
 * @param definition corresponding detectorchannel definition
 * @return
 */
eveSMChannel::eveSMChannel(eveScanModule* scanmodule, eveChannelDefinition* definition, QHash<QString, QString> parameter, QList<eveEventProperty* >* eventlist, eveSMChannel* normalizeWith)  :
eveSMBaseDevice(scanmodule) {

	scanModule = scanmodule;
	signalCounter=0;
	haveValue = false;
	haveStop = false;
	haveTrigger = false;
	haveUnit = false;
	name = definition->getName();
	xmlId = definition->getId();
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
	eventList = eventlist;
	unit = "";
	isTimer = false;
	// true if read timeout is <= 10s
	timeoutShort = true;
	normalizeChannel = normalizeWith;
	isDetectorTrigger = false;
	isDetectorUnit = false;
	detector = definition->getDetectorDefinition()->getDetector();

	if ((definition->getValueCmd() != NULL) && (definition->getValueCmd()->getTrans()!= NULL)){
      eveTransportDefinition* transdef = (eveTransportDefinition*)definition->getValueCmd()->getTrans();
      if (transdef->getTransType() == eveTRANS_CA){
			valueTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getValueCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
      else if (transdef->getTransType() == eveTRANS_LOCAL) {
          if (transdef->getName() == "Timer"){
             valueTrans = new eveTimer(this, xmlId, name, transdef);
             isTimer = true;
          }
          else if (transdef->getName() == "Counter"){
             valueTrans = new eveCounter(this, xmlId, name, transdef);
          }
      }
      if (transdef->getTimeout() > 10.0) timeoutShort = false;
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
	if (stopTrans != NULL) haveStop = true;

	if (definition->getTrigCmd() != NULL){
		if (definition->getTrigCmd()->getTrans()!= NULL){
			if (definition->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
				triggerTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getTrigCmd()->getTrans());
				triggerValue.setType(definition->getTrigCmd()->getValueType());
				triggerValue.setValue(definition->getTrigCmd()->getValueString());
				if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
			}
			if (definition->getTrigCmd()->getTrans()->getTimeout() > 10.0) timeoutShort = false;
		}
	}
	else {
		triggerTrans = detector->getTrigTrans();
		triggerValue = detector->getTrigValue();
		isDetectorTrigger = true;
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
	maxAttempts = 1e8;
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
	if (parameter.contains("sendreadyevent"))
		sendreadyevent = (parameter.value("sendreadyevent").toLower() == "true");

	// we do average calculations if averageCount > 0
	valueCalc = NULL;
	if (averageCount > 1) valueCalc = new eveAverage(averageCount, maxAttempts, minimum, maxDeviation);
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
		if (haveValue) delete valueTrans;
		if (haveStop) delete stopTrans;
		if (haveTrigger && !isDetectorTrigger) delete triggerTrans;
		if (haveUnit && !isDetectorUnit) delete unitTrans;

		foreach (eveEventProperty* evprop, *eventList){
			disconnect(evprop, SIGNAL(signalEvent(eveEventProperty*)), this, SLOT(newEvent(eveEventProperty*)));
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
		sendError(DEBUG, 0, QString("registering detector event").arg(evprop->getName()));
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
//	else if (channelStatus == eveCHANNELTRIGGER){
//		// trigger is ready, we do nothing
//		signalReady();
//	}
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
		if ((signalCounter <= 0) && retrieveData()) {

			channelStatus = eveCHANNELIDLE;

			if (redo){
				// for now we repeat from start
				// if needed we could just repeat the last detector value
				valueCalc->reset();
				triggerRead(false);
			}
			else if (curValue == NULL){
				// message and bail out
				sendError(ERROR, 0, "unable to read current value");
				signalReady();
			}
			else if (valueCalc){
				// we need to do average measurements
				valueCalc->addValue(curValue->toVariant());
				sendError(DEBUG, 0, QString("Channel %1, Raw Value %2").arg(curValue->getXmlId()).arg(curValue->toVariant().toDouble()));
				if(valueCalc->isDone()){
					// ready with average measurements
					delete curValue;
					curValue = valueCalc->getResultMessage();
					valueCalc->reset();
					curValue->setXmlId(xmlId);
					curValue->setName(name);
					signalReady();
					sendError(DEBUG, 0, QString("Channel %1, Average Value %2").arg(curValue->getXmlId()).arg(curValue->toVariant().toDouble()));
				}
				else
					triggerRead(false);
			}
			else {
				curValue->setXmlId(xmlId);
				curValue->setName(name);
				currentValue = curValue->toVariant();
				signalReady();
			}
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
}

/**
 * \brief trigger detector if it has a trigger command
 * @param queue if true queue the request, if false send it immediately
 *
 * if detector has a trigger this method only triggers the detector,
 * read must be called explicitly
 * deviceDone is always signaled
 */
//void eveSMChannel::trigger(bool queue) {
//
//	if (channelOK) {
//		ready = false;
//		if (haveTrigger){
//			channelStatus = eveCHANNELTRIGGER;
//			if (triggerTrans->writeData(triggerValue, queue)) {
//				sendError(ERROR,0,"error triggering");
//				transportReady(1);
//			}
//		}
//		else {
//			signalReady();
//		}
//	}
//	else {
//		sendError(ERROR,0,"not operational");
//		signalReady();
//	}
//}

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
		redo = false;
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
 *
 * @return true if NormalizeChannel is ready
 */
bool eveSMChannel::retrieveData(){

	if ((normalizeChannel) && (!normalizeChannel->isDone())) return false;

	if (curValue != NULL) delete curValue;
	curValue = valueTrans->getData();
	if (curValue == NULL) return true;

	if (normalizeChannel){
		bool status = true;
		eveDataMessage* normChannelMesg = normalizeChannel->getValueMessage();
		if (normChannelMesg == NULL){
			sendError(ERROR,0,"unable to retrieve value of normalized Channel");
			return true;
		}
		else if ((normChannelMesg->getDataType() != curValue->getDataType()) ||
				!(normChannelMesg->isFloat() || normChannelMesg->isInteger())){
			sendError(ERROR,0,"unable to do normalization with current datatype");
		}
		else if (normChannelMesg->getArraySize() == curValue->getArraySize()){
			QVector<double> current = curValue->getDoubleArray();
			QVector<double> normalized = normChannelMesg->getDoubleArray();
			QVector<double> result(curValue->getArraySize());
			for(int i=0; i<curValue->getArraySize(); ++i){
				if (normalized[i] != 0.0){
					result[i]=current[i] / normalized[i];
				}
				else {
					sendError(ERROR,0,"value of normalization channel is zero");
					status = false;
					break;
				}
			}
			if (status){
				eveDataMessage* normalizedValue = new eveDataMessage(curValue->getXmlId(), curValue->getName(), curValue->getDataStatus(), DMTnormalized, curValue->getDataTimeStamp(), result);
				normalizedValue->setAuxString(normalizeChannel->getXmlId());
				delete curValue;
				curValue = normalizedValue;
			}
		}
		delete normChannelMesg;
	}
	return true;
}

/**
 *
 * @return pointer to a message with all info about this channel
 */
eveDevInfoMessage* eveSMChannel::getDeviceInfo(){

	QStringList* sl;
	QString auxInfo = "";
	eveDataModType dataMod = DMTunmodified;

	if (haveValue)
		sl = valueTrans->getInfo();
	else
		sl = new QStringList();

	sl->prepend(QString("Name:%1").arg(name));
	sl->prepend(QString("XML-ID:%1").arg(xmlId));
	sl->append(QString("unit:%1").arg(unit));
	sl->append(QString("DeviceType:Channel"));
	if (curValue != NULL){
		sl->append(QString("Value:%1").arg(curValue->toVariant().toString()));
	}
	if (averageCount > 1) {
		sl->append(QString("AverageCount:%1").arg(averageCount));
		sl->append(QString("maxAttempts:%1").arg(maxAttempts));
		sl->append(QString("maxDeviation:%1").arg(maxDeviation));
		sl->append(QString("minimum:%1").arg(minimum));
	}
	if (normalizeChannel){
		sl->append(QString("NormalizeChannelID:%1").arg(normalizeChannel->getXmlId()));
		dataMod = DMTnormalized;
		auxInfo = normalizeChannel->getXmlId();
	}

	return new eveDevInfoMessage(xmlId, name, sl, dataMod, auxInfo);
}

void eveSMChannel::loadPositioner(int pc){
	foreach (eveCalc* positioner, positionerList){
		positioner->addValue(xmlId, pc, currentValue);
	}
}


void eveSMChannel::newEvent(eveEventProperty* evprop) {

	if (evprop == NULL){
		sendError(ERROR,0,"invalid event property");
		return;
	}

	if (evprop->getActionType() == eveEventProperty::REDO){
		sendError(DEBUG, 0, "received redo event");
		redo = true;
	}
}

void eveSMChannel::setTimer(QDateTime start) {
	if (isTimer){
		((eveTimer*)valueTrans)->setStartTime(start);
	}
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
