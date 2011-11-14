/*
 * eveSimplePV.cpp
 *
 *  Created on: 19.10.2011
 *      Author: eden
 */

#include "eveSimplePV.h"
#include <iostream>
#include <QTime>

eveSimplePV::eveSimplePV(QString pvname) {

	chanChid = 0;
	lastStatus = connectReadPV(pvname);
}

eveSimplePV::~eveSimplePV() {
	// TODO Auto-generated destructor stub
}

simpleCaStatusT eveSimplePV::connectReadPV(QString pvname){

	int status;

	status = ca_context_create (ca_enable_preemptive_callback);
	if (status != ECA_NORMAL) {
		sendError("eveSimplePV:Error creating CA Context");
		return SCSERROR;
	}

	lastStatus=SCSERROR;
	status=ca_create_channel(pvname.toAscii().data(), &eveSimplePVConnectCB, (void*) this, 0, &chanChid);
	if(status != ECA_NORMAL){
		sendError("eveSimplePV: Error creating CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	ca_pend_io(0.0);

	waitLock.lock();
	if(!waitForCA.wait(&waitLock, 2000)){
		waitLock.unlock();
		sendError("eveSimplePV: Timeout while connecting to CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	waitLock.unlock();

	lastStatus=SCSERROR;
	status = ca_get_callback(DBR_STRING, chanChid, &eveSimplePVGetCB, (void*) this);
	if(status != ECA_NORMAL){
		sendError("eveSimplePV: Error reading from CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	ca_flush_io();

	waitLock.lock();
	if(!waitForCA.wait(&waitLock, 2000)){
		sendError("eveSimplePV: Timeout while reading from CA Channel");
		disconnectPV();
		return SCSERROR;
	}
	disconnectPV();
	return SCSSUCCESS;
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

	lastStatus = state;
	if (state){
		sendError("connected");
	}
	else {
		sendError("connection unsuccessful");
	}
	waitLock.lock();
	waitForCA.wakeAll();
    waitLock.unlock();
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

	if (value != NULL){
		pvdata = value;
		sendError(QString("data is %1").arg(pvdata));
		lastStatus = SCSSUCCESS;
	}
	else {
		sendError("error reading value");
		lastStatus = SCSERROR;
	}
	waitLock.lock();
	waitForCA.wakeAll();
    waitLock.unlock();
}

void eveSimplePV::disconnectPV(){

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

