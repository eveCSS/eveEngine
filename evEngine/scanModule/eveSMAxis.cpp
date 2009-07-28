/*
 * eveSMAxis.cpp
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#include <exception>
#include <QTimer>
#include "eveSMAxis.h"
#include "eveError.h"
#include "eveScanModule.h"

// TODO
// trigger axis after setting new position if a trigger is available
// read unit string

eveSMAxis::eveSMAxis(eveScanModule *sm, eveMotorAxis* motorAxisDef, evePosCalc *poscalc) :
	QObject(sm){

	posCalc = poscalc;
	axisDef = motorAxisDef;
	haveDeadband = false;
	haveTrigger = false;
	haveUnit = false;
	haveStatus = false;
	haveStop = false;
	haveGoto = false;
	havePos = false;
	inDeadband = true;
	gotoTrans = NULL;
	posTrans = NULL;
	stopTrans = NULL;
	statusTrans = NULL;
	triggerTrans = NULL;
	deadbandTrans = NULL;
	axisStatus = eveAXISINIT;
	signalCounter=0;
	id = axisDef->getId();
	name = axisDef->getName();
	scanModule = sm;
	ready = true;
	axisOK = false;
	axisStop = false;
	curPosition = NULL;

	if (axisDef->getGotoCmd() != NULL){
		if (axisDef->getGotoCmd()->getTrans()->getTransType() == eveTRANS_CA){
			gotoTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getGotoCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (gotoTrans != NULL)
		haveGoto = true;
	else
		sendError(ERROR, 0, "Unknown GOTO Transport");

	if (axisDef->getPosCmd() != NULL){
		if (axisDef->getPosCmd()->getTrans()->getTransType() == eveTRANS_CA){
			posTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getPosCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (posTrans != NULL)
		havePos = true;
	else
		sendError(ERROR, 0, "Unknown Position Transport");

	if (axisDef->getTrigCmd() != NULL){
		if (axisDef->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
			triggerTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getTrigCmd()->getTrans());
			triggerValue.setType(axisDef->getTrigCmd()->getValueType());
			triggerValue.setValue(axisDef->getTrigCmd()->getValueString());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (triggerTrans != NULL)
		haveTrigger = true;

	if (axisDef->getStopCmd() != NULL){
		if (axisDef->getStopCmd()->getTrans()->getTransType() == eveTRANS_CA){
			stopTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getStopCmd()->getTrans());
			stopValue.setType(axisDef->getStopCmd()->getValueType());
			stopValue.setValue(axisDef->getStopCmd()->getValueString());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (stopTrans != NULL) haveStop = true;

	if (axisDef->getStatusCmd() != NULL){
		if (axisDef->getStatusCmd()->getTrans()->getTransType() == eveTRANS_CA){
			statusTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getStatusCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (statusTrans != NULL) haveStatus = true;

	if (axisDef->getDeadbandCmd() != NULL){
		if (axisDef->getDeadbandCmd()->getTrans()->getTransType() == eveTRANS_CA){
			deadbandTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getDeadbandCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (deadbandTrans != NULL) haveDeadband = true;

	if (axisDef->getUnitCmd() != NULL){
		if (axisDef->getUnitCmd()->getTrans()->getTransType() == eveTRANS_CA){
			unitTrans = new eveCaTransport(this, name, (eveCaTransportDef*)axisDef->getUnitCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (unitTrans != NULL) haveUnit = true;

}

eveSMAxis::~eveSMAxis() {
	try
	{
		if (haveGoto) delete gotoTrans;
		if (havePos) delete posTrans;
		if (haveStop) delete stopTrans;
		if (haveTrigger) delete triggerTrans;
		if (haveStatus) delete statusTrans;
		if (haveDeadband) delete deadbandTrans;
		if (haveUnit) delete unitTrans;
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
			axisStatus = eveAXISIDLE;
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
	posCalc->reset();
	gotoPos(posCalc->getStartPos(), queue);
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
	else {
		targetPosition = newpos;
		axisStatus = eveAXISWRITEPOS;
		if (gotoTrans->writeData(targetPosition, queue)) {
			sendError(ERROR,0,"error writing goto data");
			signalReady();
		}
	}
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
		sendError(ERROR, 0, "Stop Command not operational");
		signalReady();
		return;
	}
	else {
		if (stopTrans->writeData(stopValue, false)) {
			sendError(ERROR,0,"error writing stop command");
			signalReady();
		}
		else
			axisStop = true;
	}
}

/**
 *
 * @return current detector value
 */
eveDataMessage* eveSMAxis::getPositionMessage(){
	eveDataMessage* return_data = curPosition;
	curPosition = NULL;
	return return_data;
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

	scanModule->sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
							errorType,  QString("Axis %1: %2").arg(name).arg(message));
}
