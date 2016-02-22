/*
 * eveDeviceDefinition.cpp
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#include "eveDeviceDefinitions.h"

eveTransportDefinition::eveTransportDefinition(eveTransportT ttype, eveType etype, transMethodT transMethod, double timeo, QString accname, QHash<QString, QString> attribs) {
	dataType=etype;
	timeout = timeo;
	accessName = accname;
	method = transMethod;
	transType = ttype;
    attributes = attribs;
}
eveTransportDefinition::eveTransportDefinition(eveTransportT ttype, eveType etype, transMethodT transMethod, double timeo, QString accname) {
    dataType=etype;
    timeout = timeo;
    accessName = accname;
    method = transMethod;
    transType = ttype;
}
eveTransportDefinition::~eveTransportDefinition() {
	// TODO Auto-generated destructor stub
}
eveTransportDefinition* eveTransportDefinition::clone() {
	return new eveTransportDefinition(transType, dataType, method, timeout, accessName );
}

eveCommandDefinition::eveCommandDefinition(eveTransportDefinition * trans, QString value, eveType valtype) {
	transDef = trans;
	valueString = value;
	valueType = valtype;
}
eveCommandDefinition::~eveCommandDefinition() {
	if (transDef != NULL) delete transDef;
}
eveCommandDefinition* eveCommandDefinition::clone() {
	eveTransportDefinition *trans;
	if (transDef != NULL)
		trans = transDef->clone();
	else
		trans = NULL;

	return new eveCommandDefinition(trans, valueString, valueType);
}



eveBaseDeviceDefinition::eveBaseDeviceDefinition(QString dName, QString dId) {
	devName = dName;
	devId = dId;
}
eveBaseDeviceDefinition::~eveBaseDeviceDefinition() {
	// TODO Auto-generated destructor stub
}


eveDeviceDefinition::eveDeviceDefinition(eveCommandDefinition *dUnit, eveCommandDefinition *dPv, QString dName, QString dId) :
  eveBaseDeviceDefinition(dName, dId) {
	valueCmd = dPv;
	unit = dUnit;
}
eveDeviceDefinition::~eveDeviceDefinition() {
	if (unit != NULL) delete unit;
	if (valueCmd != NULL) delete valueCmd;
}

eveChannelDefinition::eveChannelDefinition(eveDetectorDefinition* detectorDef, eveCommandDefinition *triggerdef, eveCommandDefinition *unitdef, eveCommandDefinition *valuedef, eveCommandDefinition *stopdef, QString channelname, QString channelid) :
  eveDeviceDefinition(unitdef, valuedef, channelname, channelid)
{
	detectorDefinition = detectorDef;
	triggerCmd = triggerdef;
	stopCmd = stopdef;
}
eveChannelDefinition::~eveChannelDefinition() {
	if (triggerCmd != NULL) delete triggerCmd;
}


eveDetectorDefinition::eveDetectorDefinition(QString dName, QString dId, eveCommandDefinition* triggerDef, eveCommandDefinition* unitDef, eveCommandDefinition* stopDef) :
  eveBaseDeviceDefinition(dName, dId)  {
	trigger = triggerDef;
	unit = unitDef;
	stop = stopDef;
}

eveDetectorDefinition::~eveDetectorDefinition() {
	if (trigger != NULL) delete trigger;
	if (unit != NULL) delete unit;
}


eveAxisDefinition::eveAxisDefinition(eveMotorDefinition* motorDef, eveCommandDefinition *triggerCom, eveCommandDefinition *aUnit, eveCommandDefinition *gotoCom, eveCommandDefinition *stopCom, eveCommandDefinition *position, eveCommandDefinition *aStatus, eveCommandDefinition *aDeadbCom, QString aName, QString aId) :
  eveDeviceDefinition(aUnit, position, aName, aId) {

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


eveMotorDefinition::eveMotorDefinition(QString dName, QString dId, eveCommandDefinition* triggerDef, eveCommandDefinition* unitDef) :
  eveBaseDeviceDefinition(dName, dId)  {

	trigger = triggerDef;
	unit = unitDef;
}

eveMotorDefinition::~eveMotorDefinition() {

	if (trigger != NULL) delete trigger;
	if (unit != NULL) delete unit;
}

