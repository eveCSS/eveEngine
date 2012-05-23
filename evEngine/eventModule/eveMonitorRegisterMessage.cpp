/*
 * eveMonitorRegisterMessage.cpp
 *
 *  Created on: 16.05.2012
 *      Author: eden
 */

#include "eveMonitorRegisterMessage.h"

eveMonitorRegisterMessage::eveMonitorRegisterMessage(eveDevice * monitordevice, int destination) : eveMessage(EVEMESSAGETYPE_MONITORREGISTER, 0, 0){
	// TODO Auto-generated constructor stub
	if ((monitordevice != NULL) && (monitordevice->getValueCmd() != NULL) && (monitordevice->getValueCmd()->getTrans() != NULL)) {
		transport = new eveTransportDef(*monitordevice->getValueCmd()->getTrans());
		xmlid = monitordevice->getId();
		name = monitordevice->getName();
		storageChannel = destination;
	}
	else {
		transport = NULL;
	}
}

eveMonitorRegisterMessage::~eveMonitorRegisterMessage() {

	if (transport) delete transport;
}

