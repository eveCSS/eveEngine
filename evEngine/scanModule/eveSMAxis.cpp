/*
 * eveSMAxis.cpp
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#include "eveSMAxis.h"
#include "eveError.h"
#include "eveScanManager.h"
#include <QTimer>

eveSMAxis::eveSMAxis(eveMotorAxis* motorAxisDef, evePosCalc *poscalc) {

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
	scanManager = NULL;  // TODO set this to send errors
	ready = true;

	if (axisDef->getGotoCmd() != NULL){
		if (axisDef->getGotoCmd()->getTrans()->getTransType() == eveTRANS_CA){
			gotoTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getGotoCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (gotoTrans != NULL)
		haveGoto = true;
	else
		sendError(ERROR,0,QString("Unknown GOTO Transport for %1 (%2)").arg(name).arg(id));

	if (axisDef->getPosCmd() != NULL){
		if (axisDef->getPosCmd()->getTrans()->getTransType() == eveTRANS_CA){
			posTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getPosCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (posTrans != NULL)
		havePos = true;
	else
		sendError(ERROR,0,QString("Unknown Position Transport for %1 (%2)").arg(name).arg(id));

	if (axisDef->getTrigCmd() != NULL){
		if (axisDef->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
			triggerTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getTrigCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (triggerTrans != NULL)
		haveTrigger = true;

	if (axisDef->getStopCmd() != NULL){
		if (axisDef->getStopCmd()->getTrans()->getTransType() == eveTRANS_CA){
			stopTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getStopCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (stopTrans != NULL) haveStop = true;

	if (axisDef->getStatusCmd() != NULL){
		if (axisDef->getStatusCmd()->getTrans()->getTransType() == eveTRANS_CA){
			statusTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getStatusCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (statusTrans != NULL) haveStatus = true;

	if (axisDef->getDeadbandCmd() != NULL){
		if (axisDef->getDeadbandCmd()->getTrans()->getTransType() == eveTRANS_CA){
			deadbandTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getDeadbandCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (deadbandTrans != NULL) haveDeadband = true;

	if (axisDef->getUnitCmd() != NULL){
		if (axisDef->getUnitCmd()->getTrans()->getTransType() == eveTRANS_CA){
			unitTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getUnitCmd()->getTrans());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (unitTrans != NULL) haveUnit = true;

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
 *
 */
void eveSMAxis::initReady() {

	axisStatus = eveAXISIDLE;
	if (haveGoto && !gotoTrans->isConnected()) {
		haveGoto=false;
		sendError(ERROR,0,QString("Unable to connect Goto Transport for %1 (%2)").arg(name).arg(id));
	}
	if (havePos && !posTrans->isConnected()) {
		havePos=false;
		sendError(ERROR,0,QString("Unable to connect Position Transport for %1 (%2)").arg(name).arg(id));
	}
	if (haveTrigger && !triggerTrans->isConnected()) {
		haveTrigger=false;
		sendError(ERROR,0,QString("Unable to connect Trigger Transport for %1 (%2)").arg(name).arg(id));
	}
	if (haveDeadband && !deadbandTrans->isConnected()) {
		haveDeadband=false;
		sendError(ERROR,0,QString("Unable to connect Deadband Transport for %1 (%2)").arg(name).arg(id));
	}
	sendError(ERROR,0,QString("Now doing Unit Transport for %1 (%2) bool:%3").arg(name).arg(id).arg(haveUnit));
	if (haveUnit && !unitTrans->isConnected()) {
		haveUnit=false;
		sendError(ERROR,0,QString("Unable to connect Unit Transport for %1 (%2)").arg(name).arg(id));
	}
	if (haveStatus && !statusTrans->isConnected()) {
		haveStatus=false;
		sendError(ERROR,0,QString("Unable to connect Status Transport for %1 (%2)").arg(name).arg(id));
	}
	//TODO check if all are done
	ready = true;
	emit axisDone();
}

/**
 *
 */
void eveSMAxis::transportReady(int status) {

	if (axisStatus == eveAXISINIT){
		if (status != 0) sendError(ERROR,0,"Error while connecting");
		--signalCounter;
		if (signalCounter <= 0) {
			initReady();
		}
	}
	else if (axisStatus == eveAXISWRITEPOS){
		// motor stopped moving, read current position
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
	else if (axisStatus == eveAXISREADPOS){
		// motor stopped moving, read current position
		if (signalCounter > 0){
			--signalCounter;
		}
		else {
			if (posTrans->getData() == NULL)
				sendError(ERROR,0,"unable to read current position");
			else
				currentPosition = posTrans->getData()->toVariant();
			// sendError(INFO,0,QString("current position is %1").arg(currentPosition.toDouble()));
			inDeadband = true;
			if (haveDeadband){
				if (deadbandTrans->getData() == NULL)
					sendError(ERROR,0,"unable to read retry deadband");
				else
					deadbandValue = deadbandTrans->getData()->toVariant();
				// sendError(INFO,0,QString("retry deadband is %1").arg(deadbandValue.toDouble()));
				eveVariant test = targetPosition - currentPosition;
				if (test.abs() > deadbandValue.abs()) {
					inDeadband = false;
					sendError(ERROR,0,"not within retry deadband");
				}
			}
			axisStatus = eveAXISIDLE;
			ready = true;
			emit axisDone();
		}
	}
}

eveSMAxis::~eveSMAxis() {
	// TODO Auto-generated destructor stub
}

/**
 * \brief get start position and move to there, reset internal counter
 * @param queue if true don't send the command, instead put it in the send queue
 */
void eveSMAxis::gotoStartPos(bool queue) {

	posCalc->reset();
	gotoPos(posCalc->getStartPos(), queue);
}

/**
 *
 * @param queue if true don't send the command, instead put it in the send queue
 */
void eveSMAxis::gotoNextPos(bool queue) {

	if (posCalc->isAtEndPos())
		return;
	else
		gotoPos(posCalc->getNextPos(), queue);
}

void eveSMAxis::gotoPos(eveVariant newpos, bool queue) {

	if (!haveGoto) {
		sendError(ERROR, 0, "Axis %1: no goto command available");
		return;
	}
	else {
		targetPosition = newpos;
		ready = false;
		axisStatus = eveAXISWRITEPOS;
		if (gotoTrans->writeData(targetPosition, queue))
			sendError(ERROR,0,"error writing goto data");
		// TODO delete this
		//transportReady(0);
	}
}

/**
 *
 * @brief execute all commands in the send queue
 */
void eveSMAxis::execQueue() {

	if (transportList.contains(eveTRANS_CA)) eveCaTransport::execQueue();
}

void eveSMAxis::sendError(int severity, int errorType,  QString message){

	// for now we write output to local console too
	eveError::log(severity, message);
	if (scanManager != NULL)
		scanManager->sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
							errorType,  QString("Axis %1: %2").arg(name).arg(message));

}
