/*
 * eveDevice.cpp
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#include "eveDevice.h"

//eveBaseTransportDef::eveBaseTransportDef(eveType eptype) {
//	dataType=eptype;
//}
//eveBaseTransportDef::~eveBaseTransportDef() {
//	// TODO Auto-generated destructor stub
//}

//eveLocalTransportDef::eveLocalTransportDef(eveType etype, transMethodT accessMethod ,QString accessname) :
//	eveBaseTransportDef::eveBaseTransportDef(etype) {
//	accessDescription = accessname;
//	method = accessMethod;
//	transType = eveTRANS_LOCAL;
//}
//eveLocalTransportDef::~eveLocalTransportDef() {
//	// TODO Auto-generated destructor stub
//}
//eveLocalTransportDef* eveLocalTransportDef::clone() {
//	return new eveLocalTransportDef(dataType, method, accessDescription );
//}
//
//eveCaTransportDef::eveCaTransportDef(eveType etype, transMethodT caMethod, double timeo, QString pvname) :
//	eveBaseTransportDef::eveBaseTransportDef(etype) {
//	timeout = timeo;
//	pV = pvname;
//	method = caMethod;
//	transType = eveTRANS_CA;
//}
//eveCaTransportDef::~eveCaTransportDef() {
//	// TODO Auto-generated destructor stub
//}
//eveCaTransportDef* eveCaTransportDef::clone() {
//	return new eveCaTransportDef(dataType, method, timeout, pV );
//}

eveTransportDef::eveTransportDef(eveTransportT ttype, eveType etype, transMethodT transMethod, double timeo, QString accname) {
	dataType=etype;
	timeout = timeo;
	accessName = accname;
	method = transMethod;
	transType = ttype;
}
eveTransportDef::~eveTransportDef() {
	// TODO Auto-generated destructor stub
}
eveTransportDef* eveTransportDef::clone() {
	return new eveTransportDef(transType, dataType, method, timeout, accessName );
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

eveChannelDefinition::eveChannelDefinition(eveDetectorDefinition* detectorDef, eveDeviceCommand *trigger, eveDeviceCommand *unit, eveDeviceCommand *valuePv, QString channelname, QString channelid) :
	eveDevice::eveDevice(unit, valuePv, channelname, channelid)
{
	detectorDefinition = detectorDef;
	triggerCmd = trigger;
	// TODO
	stopCmd = NULL;
}
eveChannelDefinition::~eveChannelDefinition() {
	if (triggerCmd != NULL) delete triggerCmd;
}


eveDetectorDefinition::eveDetectorDefinition(QString dName, QString dId, eveDeviceCommand* triggerDef, eveDeviceCommand* unitDef) :
	eveBaseDevice::eveBaseDevice(dName, dId)  {
	trigger = triggerDef;
	unit = unitDef;
	detector = NULL;
}

eveDetectorDefinition::~eveDetectorDefinition() {
	if (trigger != NULL) delete trigger;
	if (unit != NULL) delete unit;
}


eveAxisDefinition::eveAxisDefinition(eveMotorDefinition* motorDef, eveDeviceCommand *triggerCom, eveDeviceCommand *aUnit, eveDeviceCommand *gotoCom, eveDeviceCommand *stopCom, eveDeviceCommand *position, eveDeviceCommand *aStatus, eveDeviceCommand *aDeadbCom, QString aName, QString aId) :
	eveDevice::eveDevice(aUnit, position, aName, aId) {

	motorDefinition = motorDef;
	triggerCmd = triggerCom;
	gotoCmd = gotoCom;
	stopCmd = stopCom;
	axisStatusCmd = aStatus;
	deadbandCmd = aDeadbCom;
}

eveAxisDefinition::~eveAxisDefinition() {

	if (triggerCmd != NULL) delete triggerCmd;
	if (gotoCmd != NULL) delete gotoCmd;
	if (stopCmd != NULL) delete stopCmd;
	if (axisStatusCmd != NULL) delete axisStatusCmd;
	if (deadbandCmd != NULL) delete deadbandCmd;

}


eveMotorDefinition::eveMotorDefinition(QString dName, QString dId, eveDeviceCommand* triggerDef, eveDeviceCommand* unitDef) :
	eveBaseDevice::eveBaseDevice(dName, dId)  {

	trigger = triggerDef;
	unit = unitDef;
	motor = NULL;
}

eveMotorDefinition::~eveMotorDefinition() {

	if (trigger != NULL) delete trigger;
	if (unit != NULL) delete unit;
}

