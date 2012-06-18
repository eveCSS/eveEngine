/*
 * eveSMAxis.cpp
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#include <stdio.h>
#include <exception>
#include <QTimer>
#include "eveSMAxis.h"
#include "eveError.h"
#include "eveScanModule.h"
#include "eveTimer.h"
#include "eveCounter.h"
#include "eveSMMotor.h"

// TODO
// trigger axis after setting new position if a trigger is available
// read unit string

eveSMAxis::eveSMAxis(eveScanModule *sm, eveAxisDefinition* motorAxisDef, evePosCalc *poscalc) :
eveSMBaseDevice(sm){

	posCalc = poscalc;
	haveDeadband = false;
	haveTrigger = false;
	haveUnit = false;
	readUnit = false;
	haveStatus = false;
	haveStop = false;
	haveGoto = false;
	havePos = false;
	inDeadband = true;
	isTimer=false;
	gotoTrans = NULL;
	posTrans = NULL;
	stopTrans = NULL;
	statusTrans = NULL;
	triggerTrans = NULL;
	deadbandTrans = NULL;
	unitTrans = NULL;
	axisStatus = eveAXISINIT;
	signalCounter=0;
	xmlId = motorAxisDef->getId();
	name = motorAxisDef->getName();
	scanModule = sm;
	ready = true;
	axisOK = false;
	axisStop = false;
	curPosition = NULL;
	unit="";
	positioner = NULL;
	isMotorTrigger = false;
	isMotorUnit = false;
	motor = motorAxisDef->getMotorDefinition()->getMotor();


	if ((motorAxisDef->getGotoCmd() != NULL) && (motorAxisDef->getGotoCmd()->getTrans() != NULL)){
		eveTransportDef* transdef = (eveTransportDef*)motorAxisDef->getGotoCmd()->getTrans();
		if (transdef->getTransType() == eveTRANS_CA){
			gotoTrans = new eveCaTransport(this, xmlId, name, transdef);
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
		else if (transdef->getTransType() == eveTRANS_LOCAL){
			if (transdef->getName() == "Timer"){
				gotoTrans = new eveTimer(this, xmlId, name, transdef);
				posTrans = gotoTrans;
				isTimer = true;
			}
			else if (transdef->getName() == "Counter"){
				gotoTrans = new eveCounter(this, xmlId, name, transdef);
				posTrans = gotoTrans;
			}
		}
	}
	if (gotoTrans != NULL)
		haveGoto = true;
	else
		sendError(ERROR, 0, "Unknown GOTO Transport");

	if ((motorAxisDef->getPosCmd() != NULL) && (motorAxisDef->getPosCmd()->getTrans() != NULL)){
		if (motorAxisDef->getPosCmd()->getTrans()->getTransType() == eveTRANS_CA){
			posTrans = new eveCaTransport(this, xmlId, name, (eveTransportDef*)motorAxisDef->getPosCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (posTrans != NULL)
		havePos = true;
	else
		sendError(ERROR, 0, "Unknown Position Transport");

	if ((motorAxisDef->getTrigCmd() != NULL)){
		if (motorAxisDef->getTrigCmd()->getTrans() != NULL){
			if (motorAxisDef->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
				triggerTrans = new eveCaTransport(this, xmlId, name, (eveTransportDef*)motorAxisDef->getTrigCmd()->getTrans());
				triggerValue.setType(motorAxisDef->getTrigCmd()->getValueType());
				triggerValue.setValue(motorAxisDef->getTrigCmd()->getValueString());
				if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
			}
		}
	}
	else {
		triggerTrans = motor->getTrigTrans();
		triggerValue = motor->getTrigValue();
		isMotorTrigger = true;
	}
	if (triggerTrans != NULL)
		haveTrigger = true;

	if ((motorAxisDef->getStopCmd() != NULL) && (motorAxisDef->getStopCmd()->getTrans() != NULL)){
		if (motorAxisDef->getStopCmd()->getTrans()->getTransType() == eveTRANS_CA){
			stopTrans = new eveCaTransport(this, xmlId, name, (eveTransportDef*)motorAxisDef->getStopCmd()->getTrans());
			stopValue.setType(motorAxisDef->getStopCmd()->getValueType());
			stopValue.setValue(motorAxisDef->getStopCmd()->getValueString());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (stopTrans != NULL) haveStop = true;

	if ((motorAxisDef->getStatusCmd() != NULL) && (motorAxisDef->getStatusCmd()->getTrans() != NULL)){
		if (motorAxisDef->getStatusCmd()->getTrans()->getTransType() == eveTRANS_CA){
			statusTrans = new eveCaTransport(this, xmlId, name, (eveTransportDef*)motorAxisDef->getStatusCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (statusTrans != NULL) haveStatus = true;

	if ((motorAxisDef->getDeadbandCmd() != NULL) && (motorAxisDef->getDeadbandCmd()->getTrans() != NULL)){
		if (motorAxisDef->getDeadbandCmd()->getTrans()->getTransType() == eveTRANS_CA){
			deadbandTrans = new eveCaTransport(this, xmlId, name, (eveTransportDef*)motorAxisDef->getDeadbandCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (deadbandTrans != NULL) haveDeadband = true;

	if (motorAxisDef->getUnitCmd() != NULL){
		if (motorAxisDef->getUnitCmd()->getTrans()== NULL){
			unit = motorAxisDef->getUnitCmd()->getValueString();
		}
		else if (motorAxisDef->getUnitCmd()->getTrans()->getTransType() == eveTRANS_CA){
			unitTrans = new eveCaTransport(this, xmlId, name, (eveTransportDef*)motorAxisDef->getUnitCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	else {
		unitTrans = motor->getUnitTrans();
		unit = motor->getUnitString();
		isMotorUnit = true;
	}
	if (unitTrans != NULL) haveUnit = true;

}

eveSMAxis::~eveSMAxis() {
	try
	{
		if (gotoTrans == posTrans) havePos = false;
		if (haveGoto) delete gotoTrans;
		if (havePos) delete posTrans;
		if (haveStop) delete stopTrans;
		if (haveStatus) delete statusTrans;
		if (haveDeadband) delete deadbandTrans;
		if (haveTrigger && !isMotorTrigger) delete triggerTrans;
		if (haveUnit && !isMotorUnit) delete unitTrans;

		delete posCalc;
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL, 0, QString("C++ Exception in ~eveSMAxis %1").arg(e.what()));
	}
}

/**
 * \brief initialization must be done in the correct thread
 */
void eveSMAxis::init() {

	ready = false;

	if (haveGoto){
		connect (gotoTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		gotoTrans->connectTrans();
	}
	if (havePos){
		connect (posTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		posTrans->connectTrans();
	}
	if (haveTrigger){
		connect (triggerTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		triggerTrans->connectTrans();
	}
	if (haveStop){
		connect (stopTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		stopTrans->connectTrans();
	}
	if (haveStatus){
		connect (statusTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		statusTrans->connectTrans();
	}
	if (haveDeadband){
		connect (deadbandTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		deadbandTrans->connectTrans();
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
 * A motor axis needs an operational goto and pos command to be flagged as "OK"
 *
 */
void eveSMAxis::initAll() {

	axisStatus = eveAXISIDLE;
	if (haveGoto && !gotoTrans->isConnected()) {
		haveGoto=false;
		sendError(ERROR, 0, "Unable to connect Goto Transport");
	}
	if (havePos && !posTrans->isConnected()) {
		havePos=false;
		sendError(ERROR, 0, "Unable to connect Position Transport");
	}
	if (haveTrigger && !triggerTrans->isConnected()) {
		haveTrigger=false;
		sendError(ERROR, 0, "Unable to connect Trigger Transport");
	}
	if (haveStop && !stopTrans->isConnected()) {
		haveStop=false;
		sendError(ERROR, 0, "Unable to connect Stop Transport");
	}
	if (haveStatus && !statusTrans->isConnected()) {
		haveStatus=false;
		sendError(ERROR, 0, "Unable to connect Status Transport");
	}
	if (haveDeadband && !deadbandTrans->isConnected()) {
		haveDeadband=false;
		sendError(ERROR, 0, "Unable to connect Deadband Transport");
	}
	if (haveUnit && !unitTrans->isConnected()) {
		haveUnit=false;
		sendError(ERROR, 0, "Unable to connect Unit Transport");
	}
	//TODO check if all are done
	if (haveGoto && havePos) axisOK = true;
}

/**
 * \brief called by underlying transport if activity is ready
 * @param status 0 if all ok, 1 is error
 */
void eveSMAxis::transportReady(int status) {

	if (axisStop) {
		axisStop = false;
		return;
	}

	if (axisStatus == eveAXISINIT){
		if (status != 0) sendError(ERROR,0,"Error while connecting");
		--signalCounter;
		if (signalCounter <= 0) {
			initAll();
			if (axisOK){
				// read initial position
				axisStatus = eveAXISREADPOS;
				signalCounter = 0;
				if (haveDeadband){
					if (deadbandTrans->readData(true))
						sendError(ERROR,0,"error reading retry deadband");
					++signalCounter;
				}
				if (haveUnit){
					readUnit = true;
					if (unitTrans->readData(true))
						sendError(ERROR,0,"error reading unit");
					++signalCounter;
				}
				if (posTrans->readData(false))
					sendError(ERROR,0,"error reading position");
			}
			else {
				signalReady();
			}
		}
	}
	else if (axisStatus == eveAXISWRITEPOS){
		// motor stopped moving, read current position
		axisStatus = eveAXISREADPOS;
		signalCounter = 0;
		if (haveDeadband){
			if (deadbandTrans->readData(true)){
				sendError(ERROR, 0, "error reading retry deadband");
			}
			else {
				++signalCounter;
			}
		}
		if (posTrans->readData(false))
			sendError(ERROR, 0, "error reading position");
	}
	else if (axisStatus == eveAXISREADPOS){
		// we try to read deadband and value, this may be called twice
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			eveDataMessage *tmpPosition = posTrans->getData();
			if (tmpPosition == NULL)
				sendError(ERROR, 0, "unable to read current position");
			else {
				if (curPosition != NULL) delete curPosition;
				curPosition = tmpPosition;
				currentPosition = curPosition->toVariant();
			}
			inDeadband = true;
			if (haveDeadband){
				eveDataMessage *curDeadband = deadbandTrans->getData();
				if (curDeadband == NULL)
					sendError(ERROR, 0, "unable to read deadband");
				else {
					deadbandValue = curDeadband->toVariant();
					delete curDeadband;
				}
				eveVariant test = targetPosition - currentPosition;
				if (test.abs() > deadbandValue.abs()) {
					inDeadband = false;
					sendError(ERROR, 0, "not within retry deadband");
				}
			}
			if (readUnit){
				readUnit=false;
				if (unitTrans->haveData()) {
					eveDataMessage *unitData = unitTrans->getData();
					if (unitData == NULL)
						sendError(ERROR, 0, "unable to read unit");
					else {
						unit = unitData->toVariant().toString();
						delete unitData;
					}
				}

			}
			axisStatus = eveAXISIDLE;
			signalCounter=0;
			signalReady();
		}
	}
}

/**
 * \brief signal axisDone if ready
 */
void eveSMAxis::signalReady() {
	ready = true;
	sendError(INFO, 0, "is done");
	emit axisDone();
}

/**
 * \brief get start position and go there, reset internal counter
 * @param queue if true don't send the command, instead put it in the send queue
 */
void eveSMAxis::gotoStartPos(bool queue) {

	ready = false;
	// currentPosition does not change for motors, but must be fetched
	// for timer
	if (isTimer){
		// TODO improve this: here we set current position to 0 when not DateTime
		if (!currentPosition.setValue(((eveTimer*)gotoTrans)->getStartTime()))
			if (!currentPosition.setValue((int)0))
				sendError(ERROR, 0, QString("SMAxis: Unable to set Start Pos to current DateTime or 0"));
	}
	posCalc->setOffset(currentPosition);
	posCalc->reset();
	gotoPos(posCalc->getStartPos(), queue);
	sendError(INFO, 0, QString("SMAxis to Start Pos at %1").arg(QTime::currentTime().toString()));
}

/**
 * \brief	get next position and go there,
 * @param	queue if true don't send the command, instead put it in the send queue
 */
void eveSMAxis::gotoNextPos(bool queue) {

	ready = false;
	if (posCalc->isAtEndPos())
		signalReady();
	else
		gotoPos(posCalc->getNextPos(), queue);
}

/**
 * \brief drive axis to given position
 * @param newpos	new target position
 * @param queue		if true, queue the message, don't flush the queue
 */
void eveSMAxis::gotoPos(eveVariant newpos, bool queue) {

	ready = false;
	if (!axisOK) {
		sendError(ERROR, 0, "Axis not operational");
		signalReady();
		return;
	}

	if (eveAXISIDLE) {
		if (posCalc->motionDisabled()){
			// skip the move, read only current position
			if (haveDeadband) {
				delete deadbandTrans;
				haveDeadband = false;
			}
			axisStatus = eveAXISREADPOS;
			if (posTrans->readData(queue)){
				sendError(ERROR, 0, "error reading position");
				signalReady();
			}
		}
		else {
			targetPosition = newpos;
			axisStatus = eveAXISWRITEPOS;
			if (gotoTrans->writeData(targetPosition, queue)) {
				sendError(ERROR,0,"error writing goto data");
				signalReady();
			}
		}
	}
}

bool eveSMAxis::execPositioner(){
	if (positioner == NULL) return false;
	if (positioner->calculate()){
		gotoPos(positioner->getXResult(), false);
		return true;
	}
	else {
		sendError(MINOR,0,"unable to calculate target position for postscan positioning");
	}
	return false;
}

/**
 * \brief send a stop signal to axis
 *
 * TODO
 * STOP is the only command which may interrupt other commands
 * make sure the signalReady does not interfere with the previous commands
 * signalReady
 */
void eveSMAxis::stop() {

	ready = false;
	if (!haveStop) {
		sendError(INFO, 0, "Stop Command not operational");
		signalReady();
		return;
	}
	else {
		if (stopTrans->writeData(stopValue, false)) {
			sendError(ERROR, 0, "error writing stop command");
			signalReady();
		}
		else
			axisStop = true;
	}
}

/**
 *
 * @return current position value
 */
eveDataMessage* eveSMAxis::getPositionMessage(){
	eveDataMessage* return_data = curPosition;
	curPosition = NULL;
	return return_data;
}

/**
 *
 * @return pointer to a message with all info about this axis
 */
eveDevInfoMessage* eveSMAxis::getDeviceInfo(){

	QStringList* sl;
	if (haveGoto)
		sl = gotoTrans->getInfo();
	else
		sl = new QStringList();
	sl->prepend(QString("Name:%1").arg(name));
	sl->prepend(QString("XML-ID:%1").arg(xmlId));
	sl->append(QString("unit:%1").arg(unit));
	sl->append(QString("DeviceType:Axis"));
	if (curPosition != NULL){
		sl->append(QString("Position:%1").arg(currentPosition.toString()));
	}
	return new eveDevInfoMessage(xmlId, name, sl);
}

/**
 * @brief flush the send queue
 */
void eveSMAxis::execQueue() {
	if (transportList.contains(eveTRANS_CA)) eveCaTransport::execQueue();
}

/**
 * \brief send an error message
 * @param severity
 * @param errorType
 * @param message
 */
void eveSMAxis::sendError(int severity, int errorType,  QString message){

	sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
							errorType,  QString("Axis %1: %2").arg(name).arg(message));
}

void eveSMAxis::sendError(int severity, int facility, int errorType, QString message){
	scanModule->sendError(severity, facility, errorType, message);
}

void eveSMAxis::setTimer(QDateTime start) {
	if (isTimer){
		((eveTimer*)gotoTrans)->setStartTime(start);
	}
}
