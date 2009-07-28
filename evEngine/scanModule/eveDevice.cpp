/*
 * eveDevice.cpp
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#include "eveDevice.h"

eveTransportDef::eveTransportDef(eveType eptype) {
	dataType=eptype;
}
eveTransportDef::~eveTransportDef() {
	// TODO Auto-generated destructor stub
}


eveLocalTransportDef::eveLocalTransportDef(eveType etype, pvMethodT accessMethod ,QString accessname) :
	eveTransportDef::eveTransportDef(etype) {
	accessDescription = accessname;
	method = accessMethod;
	transtype = eveTRANS_LOCAL;
}
eveLocalTransportDef::~eveLocalTransportDef() {
	// TODO Auto-generated destructor stub
}
eveLocalTransportDef* eveLocalTransportDef::clone() {
	return new eveLocalTransportDef(dataType, method, accessDescription );
}

eveCaTransportDef::eveCaTransportDef(eveType etype, pvMethodT caMethod, double timeo, QString pvname) :
	eveTransportDef::eveTransportDef(etype) {
	timeout = timeo;
	pV = pvname;
	method = caMethod;
	transtype = eveTRANS_CA;
}
eveCaTransportDef::~eveCaTransportDef() {
	// TODO Auto-generated destructor stub
}
eveCaTransportDef* eveCaTransportDef::clone() {
	return new eveCaTransportDef(dataType, method, timeout, pV );
}

eveDeviceCommand::eveDeviceCommand(eveTransportDef * trans, QString value, eveType valtype) {
	transDef = trans;
	valueString = value;
	valueType = valtype;
}
eveDeviceCommand::~eveDeviceCommand() {
	if (transDef != NULL) delete transDef;
}
eveDeviceCommand* eveDeviceCommand::clone() {
	eveTransportDef *trans;
	if (transDef != NULL)
		trans = transDef->clone();
	else
		trans = NULL;

	return new eveDeviceCommand(trans, valueString, valueType);
}



eveBaseDevice::eveBaseDevice(QString dName, QString dId) {
	devName = dName;
	devId = dId;
}
eveBaseDevice::~eveBaseDevice() {
	// TODO Auto-generated destructor stub
}


eveDevice::eveDevice(eveDeviceCommand *dUnit, eveDeviceCommand *dPv, QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId) {
	valueCmd = dPv;
	unit = dUnit;
}
eveDevice::~eveDevice() {
	if (unit != NULL) delete unit;
	if (valueCmd != NULL) delete valueCmd;
}

eveDetectorChannel::eveDetectorChannel(eveDeviceCommand *trigger, eveDeviceCommand *unit, eveDeviceCommand *valuePv, QString channelname, QString channelid) :
	eveDevice::eveDevice(unit, valuePv, channelname, channelid)
{
	triggerCmd = trigger;
	// TODO
	stopCmd = NULL;
}
eveDetectorChannel::~eveDetectorChannel() {
	if (triggerCmd != NULL) delete triggerCmd;
}


eveDetector::eveDetector(QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId)  {
	// TODO Auto-generated constructor stub

}

eveDetector::~eveDetector() {
	// TODO Auto-generated destructor stub
}


eveMotorAxis::eveMotorAxis(eveDeviceCommand *triggerCom, eveDeviceCommand *aUnit, eveDeviceCommand *gotoCom, eveDeviceCommand *stopCom, eveDeviceCommand *position, eveDeviceCommand *aStatus, eveDeviceCommand *aDeadbCom, QString aName, QString aId) :
	eveDevice::eveDevice(aUnit, position, aName, aId) {

	// TODO gotoCom may not be NULL
	triggerCmd = triggerCom;
	gotoCmd = gotoCom;
	stopCmd = stopCom;
	axisStatusCmd = aStatus;
	deadbandCmd = aDeadbCom;
}

eveMotorAxis::~eveMotorAxis() {

	if (triggerCmd != NULL) delete triggerCmd;
	if (gotoCmd != NULL) delete gotoCmd;
	if (stopCmd != NULL) delete stopCmd;
	if (axisStatusCmd != NULL) delete axisStatusCmd;
	if (deadbandCmd != NULL) delete deadbandCmd;

}


eveMotor::eveMotor(QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId)  {
	// TODO Auto-generated constructor stub

}

eveMotor::~eveMotor() {
	// TODO Auto-generated destructor stub
}

