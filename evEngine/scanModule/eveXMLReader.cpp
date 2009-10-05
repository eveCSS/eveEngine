/*
 * eveXMLReader.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include <exception>
#include <QDomDocument>
#include <QDomNodeList>
#include <QIODevice>
#include <QString>
#include "eveTypes.h"
#include "eveXMLReader.h"
#include "eveScanManager.h"
#include "eveScanModule.h"
#include "evePosCalc.h"

eveXMLReader::eveXMLReader(eveManager *parentObject){
	parent = parentObject;
	domDocument = new QDomDocument();
	// TODO create Hashes
//	idHash = new QHash<QString, QString>;
//	motorHash = new QHash<QString, QDomElement>;
//	detectorHash = new QHash<QString, QDomElement>;
//	deviceHash = new QHash<QString, QDomElement>;

}

eveXMLReader::~eveXMLReader() {
	if (domDocument != NULL) delete domDocument;
}


/** \brief read XML-Data and create all device definitions and various hashes etc.
 * \param xmldata XML text data
 */
bool eveXMLReader::read(QByteArray xmldata, eveDeviceList *devList)
{
    QString errorStr;
    int errorLine;
    int errorColumn;
    deviceList = devList;

    if (!domDocument->setContent(xmldata, true, &errorStr, &errorLine, &errorColumn)) {
		sendError(ERROR,0,QString("eveXMLReader::read: Parse error at line %1, column %2: Text: %3")
                .arg(errorLine).arg(errorColumn).arg(errorStr));
        return false;
    }

    QDomElement root = domDocument->documentElement();
  	printf("%s\n",QString("ScanModule-File root %1").arg(root.tagName()).toAscii().data());
    if (root.tagName() != "scml") {
		sendError(ERROR,0,QString("eveXMLReader::read: file is not a scml file, it is %1").arg(root.tagName()));
        return false;
    }
    QDomElement version = root.firstChildElement("version");
    if (version.text() != "0.2.4") {
		sendError(ERROR,0,QString("eveXMLReader::read: incompatible xml version: %1").arg(version.text()));
        return false;
    }
    // build indices
    // one Hash with DomElement / chain-id
    // one Hash with DomElement / sm-id for every chain
    // one Hash with previousHash /chain-id
    QDomElement domElem = root.firstChildElement("chain");
	while (!domElem.isNull()) {
     	if (domElem.hasAttribute("id")) {
     		QString typeString = domElem.attribute("id");
     		int chainNo = typeString.toInt();
     		chainDomIdHash.insert(chainNo, domElem);
     		smIdHash.insert(chainNo, new QHash<int, QDomElement> );
     		QDomElement domSM = domElem.firstChildElement("scanmodules");
     		domSM = domSM.firstChildElement("scanmodule");
     		while (!domSM.isNull()) {
     		    if (domSM.hasAttribute("id")) {
     		     	unsigned int smNo = QString(domSM.attribute("id")).toUInt();
		     		(smIdHash.value(chainNo))->insert(smNo, domSM);
     		     	QDomElement domParent = domSM.firstChildElement("parent");
     		     	if (!domParent.isNull()) {
     		     		bool ok;
     		     		if (domParent.text().toInt(&ok) == 0)
							if (ok) rootSMHash.insert(chainNo,smNo);
     		     	}
     		    }
     			domSM = domSM.nextSiblingElement("scanmodule");
     		}
     		if (!rootSMHash.contains(chainNo))
     			sendError(ERROR,0,QString("no root scanmodule found for chain %1").arg(chainNo));
     	}
		domElem = domElem.nextSiblingElement("chain");
	}

    domElem = root.firstChildElement("detectors");
    QDomNodeList detectorNodeList = domElem.childNodes();
    for (unsigned int i=0; i < detectorNodeList.length(); ++i) createDetectorDefinition(detectorNodeList.item(i));
    //    sendError(INFO,0,QString("eveXMLReader::read: found %1 detectorChannels").arg(channelDefinitions.size()));

    domElem = root.firstChildElement("motors");
    QDomNodeList motorNodeList = domElem.childNodes();
    for (unsigned int i=0; i < motorNodeList.length(); ++i) createMotorDefinition(motorNodeList.item(i));

    domElem = root.firstChildElement("devices");
    QDomNodeList deviceNodeList = domElem.childNodes();
    for (unsigned int i=0; i < deviceNodeList.length(); ++i) createDeviceDefinition(deviceNodeList.item(i));

    domElem = root.firstChildElement("events");
    QDomNodeList eventNodeList = domElem.childNodes();
    for (unsigned int i=0; i < eventNodeList.length(); ++i) createEventDefinition(eventNodeList.item(i));

    return true;
}

/** \brief create an eveDetector from XML
 * \param detector the <detector> DomNode
 */
void eveXMLReader::createDetectorDefinition(QDomNode detector){

	QString name;
	QString id;
	eveDeviceCommand *trigger=NULL;
	eveDeviceCommand *unit=NULL;
	eveDetectorChannel* channel;

	if (detector.isNull()){
		sendError(INFO,0,"eveXMLReader::createDetector: cannot create Null detector, check XML-Syntax");
		return;
	}
	QDomElement domElement = detector.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = detector.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;
    //printf("name: %s\n", id.toAscii().data());

    // TODO do we need motors / detectors, or is axis / detector-channel sufficient
    // eveDetector* detect = new eveDetector(name, id);

    domElement = detector.firstChildElement("trigger");
    if (!domElement.isNull()) trigger = createDeviceCommand(domElement);

    domElement = detector.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
            sendError(DEBUG,0,QString("eveXMLReader::createDetector: found unit with string %1").arg(unitstring.text()));
    		unit = new eveDeviceCommand(NULL, unitstring.text(), eveStringT);
    	}
    }


    domElement = detector.firstChildElement("channel");
	while (!domElement.isNull()) {
		channel = createChannelDefinition(domElement, trigger, unit);
		if (channel != NULL) deviceList->insert(channel->getId(),channel);
	    sendError(INFO,0,QString("eveXMLReader::createDetector: channel-id: %1").arg(channel->getId()));
		domElement = domElement.nextSiblingElement("channel");
	}

	domElement = detector.firstChildElement("option");
	while (!domElement.isNull()) {
		createDeviceDefinition(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}
    sendError(INFO,0,QString("eveXMLReader::createDetector: id: %1, name: %2").arg(id).arg(name));
}

/** \brief create a Transport from XML
 * \param node the <access> DomNode
 */
eveTransportDef * eveXMLReader::createTransportDefinition(QDomElement node)
{
	QString typeString, transportString, timeoutString;
	QString methodString;
	double timeout = 5.0;
	eveType accesstype = eveInt8T;
	pvMethodT accessMethod= eveGET;

	if (node.hasAttribute("type")) {
		typeString = node.attribute("type");
		if (typeString == "int") accesstype = eveInt32T;
		else if (typeString == "double") accesstype = eveFloat64T;
		else if (typeString == "string") accesstype = eveStringT;
		else {
			sendError(ERROR,0,QString("eveXMLReader::createTransport: unknown data type: %1").arg(typeString));
		}
	}
	if (node.hasAttribute("method")) {
		methodString = node.attribute("method");
		if (methodString == "PUT") accessMethod = evePUT;
		else if (methodString == "GET") accessMethod = eveGET;
		else if (methodString == "GETPUT") accessMethod = eveGETPUT;
		else if (methodString == "GETCB") accessMethod = eveGETCB;
		else if (methodString == "PUTCB") accessMethod = evePUTCB;
		else if (methodString == "GETPUTCB") accessMethod = eveGETPUTCB;
		else {
			sendError(ERROR,0,QString("eveXMLReader::createTransport: unknown method: %1").arg(methodString));
		}
	}
	if (node.hasAttribute("timeout")) {
		bool ok;
		double tmpdouble;
		tmpdouble = node.attribute("timeout").toDouble(&ok);
		if (ok){
			timeout = tmpdouble;
		}
		else {
			sendError(ERROR,0,QString("eveXMLReader::createTransport: unable to read timeout from xml"));
		}
	}
	if (node.hasAttribute("transport")) {
		transportString = node.attribute("transport");
		if (transportString == "local")
			return new eveLocalTransportDef(accesstype, accessMethod, node.text());
	}
	// return default transport
	return new eveCaTransportDef(accesstype, accessMethod, timeout, node.text());

}

/** \brief create a Detector Channel from XML
 * \param node the <channel> DomNode
 * \param defaultTrigger the default trigger if avail. else NULL
 * \param defaultUnit the default unit if avail. else NULL
 */
eveDetectorChannel * eveXMLReader::createChannelDefinition(QDomNode channel, eveDeviceCommand *defaultTrigger, eveDeviceCommand *defaultUnit){

	// name, id, read, unit, trigger
	QString name;
	QString id;
	eveDeviceCommand *trigger=defaultTrigger;
	eveDeviceCommand *unit=defaultUnit;
	eveDeviceCommand *read=NULL;

	QDomElement domElement = channel.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = channel.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;

	domElement = channel.firstChildElement("read");
	if (!domElement.isNull())
		read = createDeviceCommand(domElement);
	else
        sendError(ERROR,0,"eveXMLReader::createChannel: Syntax error in channel tag, no <read>");

    domElement = channel.firstChildElement("trigger");
    if (!domElement.isNull())
    	trigger = createDeviceCommand(domElement);
    else { // every device needs its private copy of default trigger
    	if (trigger != NULL) trigger = trigger->clone();
    }

    domElement = channel.firstChildElement("unit");
    if (!domElement.isNull())
    	unit = createDeviceCommand(domElement);
    else { // every device needs its private copy of default unit
    	if (unit != NULL) unit = unit->clone();
    }
    domElement = channel.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
    		unit = new eveDeviceCommand(NULL, unitstring.text(), eveStringT);
    	}
    }
    else { // every device needs its private copy of default unit
    	// TODO define a copyConstructor which allocates a new eveTransportDef
    	// every device command needs its own eveTransportDef
    	// do not use the default copy constructor or leave clone method
    	//    	if (unit != NULL) unit = new eveDeviceCommand(*unit);
    	if (unit != NULL) unit = unit->clone();
    }


	return new eveDetectorChannel(trigger, unit, read, name, id);

}

void eveXMLReader::sendError(int severity, int errorType,  QString errorString){
	if (parent != NULL) {
		parent->sendError(severity, EVEMESSAGEFACILITY_XMLPARSER, errorType, errorString);
	}
}

/** \brief create an eveMotor from XML
 * \param motor the <motor> DomNode
 */
void eveXMLReader::createMotorDefinition(QDomNode motor){

	QString name;
	QString id;
	eveDeviceCommand *trigger=NULL;
	eveDeviceCommand *unit=NULL;
	eveMotorAxis* axis;

	if (motor.isNull()){
		sendError(INFO,0,"eveXMLReader::createMotor: cannot create Null motor, check XML-Syntax");
		return;
	}
	QDomElement domElement = motor.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = motor.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;
    //printf("name: %s\n", id.toAscii().data());

    // TODO do we need motors / detectors, or is axis / detector-channel sufficient
    // eveMotor* detect = new eveMotor(name, id);

    domElement = motor.firstChildElement("trigger");
    if (!domElement.isNull()) trigger = createDeviceCommand(domElement);

    domElement = motor.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
    		unit = new eveDeviceCommand(NULL, unitstring.text(), eveStringT);
    	}
    }

    domElement = motor.firstChildElement("axis");
	while (!domElement.isNull()) {
		axis = createAxisDefinition(domElement, trigger, unit);
		if (axis != NULL) deviceList->insert(axis->getId(),axis);
	    sendError(INFO,0,QString("eveXMLReader::createMotor: axis-id: %1").arg(axis->getId()));
		domElement = domElement.nextSiblingElement("axis");
	}

	domElement = motor.firstChildElement("option");
	while (!domElement.isNull()) {
		createDeviceDefinition(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}
    sendError(INFO,0,QString("eveXMLReader::createMotor: id: %1, name: %2").arg(id).arg(name));
}


/** \brief create a Detector Axis from XML
 * \param node the <axis> DomNode
 * \param defaultTrigger the default trigger if avail. else NULL
 * \param defaultUnit the default unit if avail. else NULL
 */
eveMotorAxis * eveXMLReader::createAxisDefinition(QDomNode axis, eveDeviceCommand *defaultTrigger, eveDeviceCommand *defaultUnit){

	QString name;
	QString id;
	eveDeviceCommand *trigger=defaultTrigger;
	eveDeviceCommand *unit=defaultUnit;
	eveDeviceCommand *stopCommand = NULL;
	eveDeviceCommand *gotoCommand=NULL;
	eveDeviceCommand *positionCommand=NULL;
	eveDeviceCommand *statusCommand=NULL;
	eveDeviceCommand *deadbandCommand=NULL;

	QDomElement domElement = axis.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = axis.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;

	domElement = axis.firstChildElement("position");
	if (!domElement.isNull()) positionCommand = createDeviceCommand(domElement);
    if (positionCommand == NULL) {
        sendError(INFO,0,QString("eveXMLReader::createAxis: unable to verify position no <position> tag for %1").arg(name));
    }

	domElement = axis.firstChildElement("status");
	if (!domElement.isNull()) statusCommand = createDeviceCommand(domElement);
    if (statusCommand == NULL) {
        sendError(INFO,0,QString("eveXMLReader::createAxis: unable to check status no <status> tag for %1").arg(name));
    }

    domElement = axis.firstChildElement("goto");
    if (!domElement.isNull())  gotoCommand = createDeviceCommand(domElement);
    if (gotoCommand == NULL) {
        sendError(ERROR,0,QString("eveXMLReader::createAxis: unable to create goto command for %1, check XML").arg(name));
        return NULL;
    }

    domElement = axis.firstChildElement("stop");
    if (!domElement.isNull()) stopCommand = createDeviceCommand(domElement);
    if (stopCommand == NULL) {
        sendError(ERROR,0,QString("eveXMLReader::createAxis: unable to create stop command for %1, check XML").arg(name));
        return NULL;
    }

    domElement = axis.firstChildElement("trigger");
    if (!domElement.isNull()){
    	trigger = createDeviceCommand(domElement);
    }
    else { // every device needs its private copy of default trigger
    	if (trigger != NULL) trigger = trigger->clone();
    }

    domElement = axis.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
            sendError(DEBUG,0,QString("eveXMLReader::createAxis: found unit with string %1").arg(unitstring.text()));
    		unit = new eveDeviceCommand(NULL, unitstring.text(), eveStringT);
    	}
    }
    else { // every device needs its private copy of default unit
// TODO use the copy constructor
//    	if (unit != NULL) unit = new eveDeviceCommand(*unit);
    	if (unit != NULL) unit = unit->clone();
    }

	domElement = axis.firstChildElement("deadband");
	if (!domElement.isNull()) deadbandCommand = createDeviceCommand(domElement);
    if (deadbandCommand == NULL) {
        sendError(INFO,0,QString("eveXMLReader::createAxis: unable to check deadband, no <deadband> tag for %1").arg(name));
    }

	return new eveMotorAxis(trigger, unit, gotoCommand, stopCommand, positionCommand, statusCommand, deadbandCommand, name, id);

}

/** \brief create a device command from XML
 * \param node the corresponding DomNode e.g. <goto>, <unit>, <stop>
 * \return deviceCommand (goto, unit, stop etc.)
 *
 */
eveDeviceCommand * eveXMLReader::createDeviceCommand(QDomNode node){
	eveTransportDef *access=NULL;
	QString valueString="";
	eveType valuetype=eveUnknownT;

	QDomElement domElement = node.firstChildElement("access");
	if (!domElement.isNull())
		access = createTransportDefinition(domElement);
	else
		sendError(ERROR, 0, "eveXMLReader::createDeviceCommand: missing access tag: ");

    domElement = node.firstChildElement("value");
    if (!domElement.isNull()){
     	valueString = domElement.text();
     	if (domElement.hasAttribute("type")) {
     		QString typeString = domElement.attribute("type");
     		if (typeString == "int") valuetype = eveInt32T;
     		else if (typeString == "double") valuetype = eveFloat64T;
     		else if (typeString == "string") valuetype = eveStringT;
     		else if (typeString == "OpenClose") valuetype = eveStringT;
     		else if (typeString == "OnOff") valuetype = eveStringT;
     		else {
     			sendError(ERROR,0,QString("eveXMLReader::createCommand: unknown data type: %1").arg(typeString));
     		}
     	}
     	else {
 			sendError(ERROR,0,"eveXMLReader::createCommand: type attribute required for <value> tag in <access>");
     	}
    }
	return new eveDeviceCommand(access, valueString, valuetype);
}

/** \brief create an option or a device from XML
 * \param node the <option> or <device> DomNode
 */
void eveXMLReader::createDeviceDefinition(QDomNode device){

	QString name;
	QString id;
	eveDeviceCommand *unit=NULL;
	eveDeviceCommand* valueTrans;

	if (device.isNull()){
		sendError(INFO,0,"eveXMLReader::createDevice: cannot create Null device/option, check XML-Syntax");
		return;
	}
	QDomElement domElement = device.firstChildElement("id");
    if (domElement.isNull()) {
    	sendError(ERROR,0,"eveXMLReader::createDevice: missing <id> Tag");
    	return;
    }
    id = domElement.text();
    if (id.length() < 1) {
    	sendError(ERROR,0,"eveXMLReader::createDevice: empty <id> Tag");
    	return;
    }
    domElement = device.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;

    domElement = device.firstChildElement("unit");
    if (!domElement.isNull()) unit = createDeviceCommand(domElement);

    domElement = device.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
    		unit = new eveDeviceCommand(NULL, unitstring.text(), eveStringT);
    	}
    }

    domElement = device.firstChildElement("value");
	if (!domElement.isNull())
		valueTrans = createDeviceCommand(domElement);
	else {
		sendError(ERROR,0,QString("eveXMLReader::createDevice: need a valid <value> tag for %1").arg(name));
		return;
	}
	deviceList->insert(id, new eveDevice(unit, valueTrans, name, id));
	// TODO remove this
    sendError(INFO,0,QString("eveXMLReader::createDevice: Found id: %1, name: %2").arg(id).arg(name));
}

/** \brief parse XML for event definitions
 * \param node the <event> DomNode
 */
void eveXMLReader::createEventDefinition(QDomNode event){

	QString name;
	QString id;
	eveDeviceCommand* valueTrans;
	eveEventTypeT eventType;

	if (event.isNull()){
		sendError(INFO,0,"eveXMLReader::createEventDefinition: cannot create Null event, check XML-Syntax");
		return;
	}
	QDomElement eventElem = event.toElement();
	if (eventElem.isNull()){
		sendError(INFO,0,"eveXMLReader::createEventDefinition: <event> domnode not an element, check XML-Syntax");
		return;
	}

	QDomElement domElement = event.firstChildElement("id");
    if (domElement.isNull()) {
    	sendError(ERROR,0,"eveXMLReader::createEventDefinition: missing <id> Tag");
    	return;
    }
    id = domElement.text();
    if (id.length() < 1) {
    	sendError(ERROR,0,"eveXMLReader::createEventDefinition: empty <id> Tag");
    	return;
    }

    domElement = event.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;
    if (name.length() < 1) name = id;

	if (!eventElem.hasAttribute("type")) {
		sendError(ERROR,0,QString("eveXMLReader::createEventDefinition: need a type attribute for event %1").arg(name));
    	return;
	}
	eventType = getEventType(eventElem.attribute("type"));

	if (eventType != eveMONITOR){
		sendError(ERROR,0,QString("eveXMLReader::createEventDefinition: only monitor events are supported, please correct %1").arg(name));
		return;
	}

    domElement = event.firstChildElement("value");
	if (!domElement.isNull())
		valueTrans = createDeviceCommand(domElement);
	else {
		sendError(ERROR,0,QString("eveXMLReader::createEventDefinition: need a valid <value> tag for %1").arg(name));
		return;
	}
	deviceList->insert(id, new eveEventDefinition(valueTrans, eventType, name, id));
	// TODO remove this
    sendError(DEBUG,0,QString("eveXMLReader::createEventDefinition: Found id: %1, name: %2").arg(id).arg(name));
}

/**
 *
 * @param chain chainid
 * @param smid scanmodule id
 * @return id of nested scan-module or 0 if none
 *
 */
int eveXMLReader::getNested(int chain, int smid){
	return getIntValueOfTag(chain, smid, "nested");
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @return id of appended scanmodule or 0 if none
 *
 */
int eveXMLReader::getAppended(int chain, int smid){
	return getIntValueOfTag(chain, smid, "appended");
}

/**
 *
 * @param chain	chainid
 * @return smid of root scanmodule for the given chain
 */
int eveXMLReader::getRootId(int chain){
	if (rootSMHash.contains(chain))
		return rootSMHash.value(chain);
	else
		return 0;
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @param tagname name of XML-Tag
 * @return integer value of tagname
 *
 */
int eveXMLReader::getIntValueOfTag(int chain, int smid, QString tagname){

	bool ok;
	if (!smIdHash.contains(chain)) return 0;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	domElement = domElement.firstChildElement(tagname);
	if (!domElement.isNull()){
		int value = domElement.text().toInt(&ok);
		if (ok) return value;
	}
	return 0;
}

/**
 * \brief get string values from chain
 * @param chain chain id
 * @param tagname name of XML-Tag
 * @return string value of tagname
 *
 */
QString eveXMLReader::getChainString(int chain, QString tagname){

	QString empty;
	if (!chainDomIdHash.contains(chain)) return empty;
	QDomElement domElement = chainDomIdHash.value(chain);
	domElement = domElement.firstChildElement(tagname);
	if (!domElement.isNull()){
		return domElement.text();
	}
	return empty;
}

/**
 *
 * @param chain chain id
 * @param smid chain id
 * @return the list of prescan-smdevices
 */
QList<eveSMDevice*>* eveXMLReader::getPreScanList(eveScanModule* scanmodule, int chain, int smid){
	return getSMDeviceList(scanmodule, chain, smid, "prescan");
}

/**
 *
 * @param chain chain id
 * @param smid chain id
 * @return the list of postscan-smdevices
 */
QList<eveSMDevice*>* eveXMLReader::getPostScanList(eveScanModule* scanmodule, int chain, int smid){
	return getSMDeviceList(scanmodule, chain, smid, "postscan");
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @param tagname may be "prescan" or "postscan"
 * @return
 */
QList<eveSMDevice*>* eveXMLReader::getSMDeviceList(eveScanModule* scanmodule, int chain, int smid, QString tagname){

	QList<eveSMDevice *> *devicelist = new QList<eveSMDevice *>;
	if (!smIdHash.contains(chain)) return devicelist;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	domElement = domElement.firstChildElement(tagname);
	while (!domElement.isNull()) {
		bool ok = false;
		eveVariant varValue;
		bool reset = false;
		eveDevice* devDef=NULL;

		QDomElement domId = domElement.firstChildElement("id");
		if (!domId.isNull())
			devDef = deviceList->getDeviceDef(domId.text());

		QDomElement domReset = domElement.firstChildElement("reset_originalvalue");
		if (!domReset.isNull()){
			if (domReset.text() == "true") reset = true;
		}

		QDomElement domValue = domElement.firstChildElement("value");
		if (!domValue.isNull()){
			QString value = domValue.text();
			if (domValue.hasAttribute("type")) {
				QString typeString = domValue.attribute("type");
				if (typeString == "int"){
					varValue.setType(eveINT);
					varValue.setValue(domValue.text().toInt(&ok));
				}
				else if (typeString == "double"){
					varValue.setType(eveDOUBLE);
					varValue.setValue(domValue.text().toDouble(&ok));
				}
				else {
					ok = true;
					varValue.setType(eveSTRING);
					varValue.setValue(domValue.text());
				}
				if (!ok) sendError(ERROR,0,QString("Error converting %1 type").arg(tagname));
			}
			else {
				sendError(ERROR,0,QString("value tag needs type attribute (%1)").arg(tagname));
			}
		}
		if (devDef != NULL)
			devicelist->append(new eveSMDevice(scanmodule, devDef, varValue, reset));
		else
			sendError(ERROR, 0, QString("Error reading device %1 (XML-Syntax Error)").arg(tagname));

		domElement = domElement.nextSiblingElement(tagname);
	}
	return devicelist;
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @return
 */
QList<eveSMAxis*>* eveXMLReader::getAxisList(eveScanModule* scanmodule, int chain, int smid){

	QList<eveSMAxis *> *axislist = new QList<eveSMAxis *>;

	try
	{
	if (!smIdHash.contains(chain)) return axislist;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	domElement = domElement.firstChildElement("smmotor");
	while (!domElement.isNull()) {
		eveVariant startvalue;
		QString stepfunction = "none";
		QDomElement domId = domElement.firstChildElement("axisid");
		eveMotorAxis* axisDefinition = deviceList->getAxisDef(domId.text());
		if (axisDefinition == NULL){
			sendError(ERROR,0,QString("no axisdefinition found for %1").arg(domId.text()));
			return axislist;
		}
		eveType axisType=axisDefinition->getAxisType();
		QDomElement domstepf = domElement.firstChildElement("stepfunction");
		if (!domstepf.isNull()) stepfunction = domstepf.text();
//		if (!scanManagerHash.contains(chain))
//			sendError(ERROR,0,"Could not find scanmanager for chain");
		evePosCalc *poscalc = new evePosCalc(scanmodule, stepfunction, axisType);

		// TODO relative or absolute position values
		// TODO stepamount,ismainaxis

		if (!domElement.firstChildElement("start").isNull()) {
			poscalc->setStartPos(domElement.firstChildElement("start").text());
			poscalc->setEndPos(domElement.firstChildElement("stop").text());
			poscalc->setStepWidth(domElement.firstChildElement("stepwidth").text());
		}
		else if (!domElement.firstChildElement("stepfilename").isNull()) {
			poscalc->setStepFile(domElement.firstChildElement("stepfilename").text());
		}
		else if (!domElement.firstChildElement("positionlist").isNull()) {
			poscalc->setPositionList(domElement.firstChildElement("positionlist").text());
		}
		else if (!domElement.firstChildElement("controller").isNull()) {
			QDomElement domContr = domElement.firstChildElement("controller");
			poscalc->setStepPlugin(domContr.attribute("plugin"));
			QDomElement domParam = domContr.firstChildElement("parameter");
			while (!domParam.isNull()){
				QDomNamedNodeMap attribMap = domParam.attributes();
				for (int i= 0; i < attribMap.count(); ++i)
					poscalc->setStepPara(attribMap.item(i).nodeValue(), domParam.text());
				domParam = domParam.nextSiblingElement("parameter");
			}
		}
		else
			sendError(ERROR,0,"No values found in XML to calculate motor positions");

		axislist->append(new eveSMAxis(scanmodule, axisDefinition, poscalc));
		domElement = domElement.nextSiblingElement("smmotor");
	}
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getAxisList").arg(e.what()));
	}
	return axislist;
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @return
 */
QList<eveSMChannel*>* eveXMLReader::getChannelList(eveScanModule* scanmodule, int chain, int smid){

	QList<eveSMChannel *> *channellist = new QList<eveSMChannel *>;

	try
	{
	if (!smIdHash.contains(chain)) return channellist;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	domElement = domElement.firstChildElement("smdetector");
	while (!domElement.isNull()) {
		QHash<QString, QString> paraHash;
		QDomElement domId = domElement.firstChildElement("channelid");
		eveDetectorChannel* channelDefinition = deviceList->getChannelDef(domId.text());
		if (channelDefinition == NULL){
			sendError(ERROR,0,QString("no channeldefinition found for %1").arg(domId.text()));
			return channellist;
		}
		domId = domElement.firstChildElement();
		while (!domId.isNull()){
			if (domId.nodeName() == "redoevent"){
				// TODO get event
			}
			else {
				paraHash.insert(domId.nodeName(), domId.text());
				domId = domId.nextSiblingElement();
			}
		}

		channellist->append(new eveSMChannel(scanmodule, channelDefinition, paraHash));
		domElement = domElement.nextSiblingElement("smdetector");
	}
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getChannelList").arg(e.what()));
	}
	return channellist;
}

QList<eveEventProperty*>* eveXMLReader::getEventList(eveScanManager* manager, int chain, int smid){

	QList<eveEventProperty* > *eventList = new QList<eveEventProperty* >;

	try
	{
	if (!smIdHash.contains(chain)) return eventList;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	domElement = domElement.firstChildElement("startevent");
	while (!domElement.isNull()) {
		eveEventProperty* event = getEvent(manager, domElement);
		if (event != NULL ) eventList->append(event);
		domElement = domElement.nextSiblingElement("startevent");
	}
	domElement = domElement.firstChildElement("redoevent");
	while (!domElement.isNull()) {
		eveEventProperty* event = getEvent(manager, domElement);
		if (event != NULL ) eventList->append(event);
		domElement = domElement.nextSiblingElement("redoevent");
	}
	domElement = domElement.firstChildElement("breakevent");
	while (!domElement.isNull()) {
		eveEventProperty* event = getEvent(manager, domElement);
		if (event != NULL ) eventList->append(event);
		domElement = domElement.nextSiblingElement("breakevent");
	}
	domElement = domElement.firstChildElement("stopevent");
	while (!domElement.isNull()) {
		eveEventProperty* event = getEvent(manager, domElement);
		if (event != NULL ) eventList->append(event);
		domElement = domElement.nextSiblingElement("stopevent");
	}
	domElement = domElement.firstChildElement("pauseevent");
	while (!domElement.isNull()) {
		eveEventProperty* event = getEvent(manager, domElement);
		if (event != NULL ) eventList->append(event);
		domElement = domElement.nextSiblingElement("pauseevent");
	}
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getEventList").arg(e.what()));
	}
	return eventList;
}

eveEventTypeT eveXMLReader::getEventType(QString evtype){
	if (evtype == "monitor")
		return eveMONITOR;
	else
		return eveSCHEDULE;
}

eveEventProperty* eveXMLReader::getEvent(eveScanManager* manager, QDomElement domElement){

 	if (!domElement.hasAttribute("type")){
		sendError(ERROR, 0, "getEvent: event tags must have an attribute type");
		return NULL;
 	}

 	if (domElement.attribute("type") == "schedule"){
		eventTypeT eventType = eveEventTypeSCHEDULE;
		incidentTypeT incident = eveIncidentTypeEND;
		int chid=0, smid=0;
		QDomElement domIncident = domElement.firstChildElement("incident");
		if ((!domIncident.isNull()) && (domIncident.text() == "Start")) incident = eveIncidentTypeSTART;
		QDomElement domChainid = domElement.firstChildElement("chainid");
		QDomElement domSmid = domElement.firstChildElement("smid");
		if (domChainid.isNull() || domSmid.isNull()){
			sendError(ERROR, 0, "getEvent: <event type=schedule> must have valid <chainid> and <smid> tags");
			return NULL;
		}
		bool cok, sok;
		chid = domChainid.text().toInt(&cok);
		smid = domSmid.text().toInt(&sok);
		if (!cok || !sok){
			sendError(ERROR, 0, "getEvent: error converting chainid or smid");
			return NULL;
		}
		return new eveEventProperty(manager, eveVariant(QVariant(eveVariant::getMangled(chid,smid))), eventType);
	}
	else if (domElement.attribute("type") == "monitor"){
	 	eventTypeT eventType = eveEventTypeMONITOR;

	 	QDomElement domId = domElement.firstChildElement("id");
		if (domId.isNull()){
			sendError(ERROR, 0, "getEvent: need <id> tag for monitor event");
			return NULL;
		}

	 	eveEventDefinition* eventDefinition = deviceList->getEventDef(domId.text());
		if (eventDefinition == NULL){
			sendError(ERROR,0,QString("no event definition found for %1").arg(domId.text()));
			return NULL;
		}

	 	QDomElement domLimit = domElement.firstChildElement("limit");
		if (domLimit.isNull()){
			sendError(ERROR, 0, QString("getEvent: need <limit> tag for monitor event with id %1").arg(domId.text()));
			return NULL;
		}
		if (!domLimit.hasAttribute("comparison") || !domLimit.hasAttribute("type")){
			sendError(ERROR, 0, QString("getEvent: need valid type and comparison attributes in <limit> tag of event with id %1").arg(domId.text()));
			return NULL;
		}

		eveVariant limitVal;
		bool ok = true;
		QString typeString = domLimit.attribute("type");
		if (typeString == "int"){
			limitVal.setType(eveINT);
			limitVal.setValue(domLimit.text().toInt(&ok));
		}
		else if (typeString == "double"){
			limitVal.setType(eveDOUBLE);
			limitVal.setValue(domLimit.text().toDouble(&ok));
		}
		else {
			limitVal.setType(eveSTRING);
			limitVal.setValue(domLimit.text());
		}
		if (!ok) sendError(ERROR, 0, QString("getEvent: error converting <limit> tag of event with id %1").arg(domId.text()));

		return new eveEventProperty(manager, limitVal, eventType);
	}
	else{
		sendError(ERROR, 0, "getEvent: invalid attribute type");
		return NULL;
	}
}
