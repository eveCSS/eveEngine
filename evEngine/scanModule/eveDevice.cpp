/*
 * eveDevice.cpp
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#include "eveDevice.h"

eveTransport::eveTransport(eveType eptype) {
	dataType=eptype;
}
eveTransport::~eveTransport() {
	// TODO Auto-generated destructor stub
}


eveCaTransport::eveCaTransport(eveType etype, pvMethodT caMethod ,QString pvname) :
	eveTransport::eveTransport(etype) {
	pV = pvname;
	method = caMethod;
}
eveCaTransport::~eveCaTransport() {
	// TODO Auto-generated destructor stub
}
eveCaTransport* eveCaTransport::clone() {
	return new eveCaTransport(dataType, method, pV );
}


eveDeviceCommand::eveDeviceCommand(eveTransport * trans, QString value, eveType etype) {
	devCmd = trans;
	devString = value;
	devType = etype;
}
eveDeviceCommand::~eveDeviceCommand() {
	if (devCmd != NULL) delete devCmd;
}
eveDeviceCommand* eveDeviceCommand::clone() {
	eveTransport *catrans;
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


eveDevice::eveDevice(eveDeviceCommand *dUnit, eveTransport *dPv, QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId) {
	valueCmd = dPv;
	unit = dUnit;
}
eveDevice::~eveDevice() {
	if (unit != NULL) delete unit;
	if (valueCmd != NULL) delete valueCmd;
}

eveSimpleDetector::eveSimpleDetector(eveDeviceCommand *trigger, eveDeviceCommand *unit, eveTransport *valuePv, QString channelname, QString channelid) :
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


eveMotorAxis::eveMotorAxis(eveDeviceCommand *triggerCom, eveDeviceCommand *aUnit, eveDeviceCommand *gotoCom, eveDeviceCommand *stopCom, eveTransport *position, eveTransport *aStatus, QString aName, QString aId) :
	eveDevice::eveDevice(aUnit, position, aName, aId) {

	// TODO gotoCom may not be NULL
	triggerCmd = triggerCom;
	gotoCmd = gotoCom;
	stopCmd = stopCom;
	axisStatus = aStatus;

}

eveMotorAxis::~eveMotorAxis() {

	if (triggerCmd != NULL) delete triggerCmd;
	if (gotoCmd != NULL) delete gotoCmd;
	if (stopCmd != NULL) delete stopCmd;
	if (axisStatus != NULL) delete axisStatus;

}


eveMotor::eveMotor(QString dName, QString dId) :
	eveBaseDevice::eveBaseDevice(dName, dId)  {
	// TODO Auto-generated constructor stub

}

eveMotor::~eveMotor() {
	// TODO Auto-generated destructor stub
}

