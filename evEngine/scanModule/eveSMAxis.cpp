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

	if (axisDef->getGotoCmd() != NULL){
		if (axisDef->getGotoCmd()->getTransType() == eveTRANS_CA){
			gotoTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getGotoCmd());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (gotoTrans != NULL)
		haveGoto = true;
	else
		sendError(ERROR,0,QString("Unknown GOTO Transport for %1 (%2)").arg(name).arg(id));

	if (axisDef->getPosCmd() != NULL){
		if (axisDef->getPosCmd()->getTransType() == eveTRANS_CA){
			posTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getPosCmd());
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
		if (axisDef->getStatusCmd()->getTransType() == eveTRANS_CA){
			statusTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getStatusCmd());
			if (!transportList.contains(eveTRANS_CA)) transportList.append(eveTRANS_CA);
		}
	}
	if (statusTrans != NULL) haveStatus = true;

	if (axisDef->getDeadbandCmd() != NULL){
		if (axisDef->getDeadbandCmd()->getTransType() == eveTRANS_CA){
			deadbandTrans = new eveCaTransport((eveCaTransportDef*)axisDef->getDeadbandCmd());
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
	if (haveGoto){
		connect (gotoTrans, SIGNAL(done(int)), this, SLOT(transportReady(int)), Qt::QueuedConnection);
		++signalCounter;
		gotoTrans->connectTrans();
	}

	// put TimeoutCounter in Tranport!
	//int initTimeout=5000;
	//QTimer::singleShot(initTimeout, this, SLOT(transportTimeout()));

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
	if (haveUnit && !unitTrans->isConnected()) {
		haveUnit=false;
		sendError(ERROR,0,QString("Unable to connect Unit Transport for %1 (%2)").arg(name).arg(id));
	}
	if (haveStatus && !statusTrans->isConnected()) {
		haveStatus=false;
		sendError(ERROR,0,QString("Unable to connect Status Transport for %1 (%2)").arg(name).arg(id));
	}
	//TODO check if all are done
	emit axisDone();
}

/**
 *
 */
void eveSMAxis::transportReady(int status) {

	if (status != 0) sendError(ERROR,0,"Error in TransportReady");
	if (axisStatus == eveAXISINIT){
		--signalCounter;
		if (signalCounter <= 0) {
			initReady();
		}
	}
}

void eveSMAxis::transportTimeout() {

	sendError(ERROR,0,"Transport Timeout");
	transportReady(1);
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
	if (!haveGoto) return;
	gotoTrans->writeData(posCalc->getStartPos(), queue);
}

/**
 *
 * @param queue if true don't send the command, instead put it in the send queue
 */
void eveSMAxis::gotoNextPos(bool queue) {

	if (posCalc->isAtEndPos()) return;
	if (!haveGoto) return;
	int status = gotoTrans->writeData(posCalc->getNextPos(), queue);
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
		scanManager->sendError(severity, EVEMESSAGEFACILITY_SMDEVICE, errorType,  message);

}
