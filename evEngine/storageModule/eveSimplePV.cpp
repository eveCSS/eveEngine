/*
 * eveSimplePV.cpp
 *
 *  Created on: 19.10.2011
 *      Author: eden
 */

#include "eveSimplePV.h"
#include <iostream>
#include <QTime>

eveSimplePV::eveSimplePV(QString name) : pvname(name) {

	chanChid = 0;
	connecting = false;
	reading = false;
}

eveSimplePV::~eveSimplePV() {
	// TODO Auto-generated destructor stub
}

simpleCaStatusT eveSimplePV::readPV(){

	int status;

	status = ca_context_create (ca_enable_preemptive_callback);
	if (status != ECA_NORMAL) {
		sendError("eveSimplePV:Error creating CA Context");
		return SCSERROR;
	}

	lastStatus=SCSERROR;
	connecting = true;
	status=ca_create_channel(pvname.toAscii().data(), &eveSimplePVConnectCB, (void*) this, 0, &chanChid);
	if(status != ECA_NORMAL){
		sendError("eveSimplePV: Error creating CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	ca_pend_io(0.0);

	connectLock.lock();
	if(!waitForConnect.wait(&connectLock, 2000)){
		connectLock.unlock();
		sendError("eveSimplePV: Timeout while connecting to CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	connectLock.unlock();
	connecting = false;

	if (lastStatus==SCSERROR){
		disconnectPV();
		return SCSERROR;
	}
	reading = true;
	lastStatus=SCSERROR;
	status = ca_get_callback(DBR_STRING, chanChid, &eveSimplePVGetCB, (void*) this);
	if(status != ECA_NORMAL){
		sendError("eveSimplePV: Error reading from CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	ca_flush_io();

	readLock.lock();
	if(!waitForRead.wait(&readLock, 2000)){
		sendError("eveSimplePV: Timeout while reading from CA Channel");
		disconnectPV();
		return SCSERROR;
	}

	disconnectPV();
	return lastStatus;
}

/** \brief connection callback
 * \param arg epics callback structure
 *
 * method is called if connection changes occur; it may be called from a different thread
 */
void eveSimplePV::eveSimplePVConnectCB(struct connection_handler_args arg){

	eveSimplePV *spv = (eveSimplePV *) ca_puser(arg.chid);
	if (arg.op == CA_OP_CONN_UP) {
		spv->wakeUp(SCSSUCCESS);
	}
	else {
		spv->wakeUp(SCSERROR);
	}
}

void eveSimplePV::wakeUp(simpleCaStatusT state){

	if (connecting){
		lastStatus = state;
		connectLock.lock();
		waitForConnect.wakeAll();
		connectLock.unlock();
	}
}

/** \brief read callback
 * \param arg epics callback structure
 *
 * method is called if read operation is ready; it may be called from a different thread
 */
void eveSimplePV::eveSimplePVGetCB(struct event_handler_args arg){

	eveSimplePV *spv = (eveSimplePV *) ca_puser(arg.chid);
	char* value = NULL;

	if (arg.status == ECA_NORMAL){
	    if ((arg.type == DBR_STRING) && (arg.dbr != NULL)) {
	    	value = (char*)arg.dbr;
	    }
	}
	spv->setValueString(value);
}

void eveSimplePV::setValueString(char* value){

	if (reading) {
		if (value != NULL){
			pvdata = value;
			lastStatus = SCSSUCCESS;
		}
		else {
			lastStatus = SCSERROR;
		}
		readLock.lock();
		waitForRead.wakeAll();
		readLock.unlock();
	}
}

void eveSimplePV::disconnectPV(){

	connecting = false;
	reading = false;

	sendError("destroy channel/context");
	if (chanChid) {
		ca_clear_channel(chanChid);
		ca_pend_io(0.0);
	}
    ca_context_destroy();
}

void eveSimplePV::sendError(QString errorstring) {
	//std::cout << qPrintable(errorstring) << "\n";
	errorText.append(errorstring);
}

