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


eveCaTransportDef::eveCaTransportDef(eveType etype, pvMethodT caMethod ,QString pvname) :
	eveTransportDef::eveTransportDef(etype) {
	pV = pvname;
	method = caMethod;
	transtype = eveTRANS_CA;
}
eveCaTransportDef::~eveCaTransportDef() {
	// TODO Auto-generated destructor stub
}
eveCaTransportDef* eveCaTransportDef::clone() {
	return new eveCaTransportDef(dataType, method, pV );
}


eveDeviceCommand::eveDeviceCommand(eveTransportDef * trans, QString value, eveType etype) {
	devCmd = trans;
	devString = value;
	devType = etype;
}
eveDeviceCommand::~eveDeviceCommand() {
	if (devCmd != NULL) delete devCmd;
}
eveDeviceCommand* eveDeviceCommand::clone() {
	eveTransportDef *catrans;
	if (devCmd != NULL)
		catrans = devCmd->clone();
	else
		catrans = NULL;

	return new eveDeviceCommand(catrans, devString, devType);
}



eveBaseDevice::eveBaseDevice(QString dName, QString dId) {
	devName = dName;
	devId = dId;
}
eveBaseDevice::~eveBaseDevice() {
	// TODO Auto-generated destructor stub
}


eveDevice::eveDevice(eveDeviceCommand *dUnit, eveTransportDef *dPv, QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId) {
	valueCmd = dPv;
	unit = dUnit;
}
eveDevice::~eveDevice() {
	if (unit != NULL) delete unit;
	if (valueCmd != NULL) delete valueCmd;
}

eveSimpleDetector::eveSimpleDetector(eveDeviceCommand *trigger, eveDeviceCommand *unit, eveTransportDef *valuePv, QString channelname, QString channelid) :
	eveDevice::eveDevice(unit, valuePv, channelname, channelid)
{
	triggerCmd = trigger;
}
eveSimpleDetector::~eveSimpleDetector() {
	if (triggerCmd != NULL) delete triggerCmd;
}


eveDetector::eveDetector(QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId)  {
	// TODO Auto-generated constructor stub

}

eveDetector::~eveDetector() {
	// TODO Auto-generated destructor stub
}


eveMotorAxis::eveMotorAxis(eveDeviceCommand *triggerCom, eveDeviceCommand *aUnit, eveTransportDef *gotoCom, eveDeviceCommand *stopCom, eveTransportDef *position, eveTransportDef *aStatus, eveTransportDef *aDeadbCom, QString aName, QString aId) :
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

