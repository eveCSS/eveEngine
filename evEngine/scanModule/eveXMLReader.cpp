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

#define EVE_VERSION        0
#define EVE_REVISION       3
#define EVE_MODIFICATION   8

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
    if (version.isNull()){
		sendError(ERROR,0,QString("eveXMLReader::read: no version tag found %1"));
        return false;
    }
    QString thisVersion = QString("%1.%2.%3").arg((int)EVE_VERSION).arg((int)EVE_REVISION).arg((int)EVE_MODIFICATION);
    QStringList versions = version.text().split(".");
    if (versions[0].toInt() != EVE_VERSION) {
		sendError(ERROR,0,QString("eveXMLReader::read: incompatible xml versions, file: %1, application: %2").arg(version.text()).arg(thisVersion));
        return false;
    }
    if (versions[1].toInt() != EVE_REVISION) {
		sendError(ERROR,0,QString("eveXMLReader::read: incompatible xml revisions file: %1, application: %2").arg(version.text()).arg(thisVersion));
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

//    domElem = root.firstChildElement("events");
//    QDomNodeList eventNodeList = domElem.childNodes();
//    for (unsigned int i=0; i < eventNodeList.length(); ++i) createEventDefinition(eventNodeList.item(i));

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
eveBaseTransportDef * eveXMLReader::createTransportDefinition(QDomElement node)
{
	QString typeString, transportString, timeoutString;
	QString methodString;
	double timeout = 5.0;
	eveType accesstype = eveInt8T;
	eveTransportT transport = eveTRANS_CA;
	transMethodT accessMethod= eveGET;

	if (node.hasAttribute("type")) {
		typeString = node.attribute("type");
		if (typeString == "int") accesstype = eveInt32T;
		else if (typeString == "OnOff") accesstype = eveInt32T;
		else if (typeString == "OpenClose") accesstype = eveInt32T;
		else if (typeString == "double") accesstype = eveFloat64T;
		else if (typeString == "string") accesstype = eveStringT;
		else if (typeString == "datetime") accesstype = eveDateTimeT;
		else {
			sendError(ERROR,0,QString("eveXMLReader::createTransportDef: unknown data type: %1").arg(typeString));
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
		else if (methodString == "monitor") accessMethod = eveMONITOR;
		else {
			sendError(ERROR,0,QString("eveXMLReader::createTransportDef: unknown method: %1").arg(methodString));
		}
	}
	else {
		sendError(ERROR, 0, "eveXMLReader::createTransportDef: method attribute is missing");
	}

	if (node.hasAttribute("timeout")) {
		bool ok;
		double tmpdouble;
		tmpdouble = node.attribute("timeout").toDouble(&ok);
		if (ok){
			timeout = tmpdouble;
		}
		else {
			sendError(ERROR,0,QString("eveXMLReader::createTransportDef: unable to read timeout from xml"));
		}
	}
	if (node.hasAttribute("transport")) {
		transportString = node.attribute("transport");
		if (transportString == "local") transport = eveTRANS_LOCAL;
	}
	// return default transport
	return new eveTransportDef(transport, accesstype, accessMethod, timeout, node.text());

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
    	// TODO define a copyConstructor which allocates a new eveBaseTransportDef
    	// every device command needs its own eveBaseTransportDef
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
	eveBaseTransportDef *access=NULL;
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
     		else if (typeString == "datetime") valuetype = eveDateTimeT;
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
/*
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

	if (eventType != eveEventMONITOR){
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
*/

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
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @param tagname name of XML-Tag
 * @return value of tagname
 */
QString eveXMLReader::getSMTag(int chain, int smid, QString tagname) {

	if (smIdHash.contains(chain)){
		QDomElement domElement = smIdHash.value(chain)->value(smid);
		domElement = domElement.firstChildElement(tagname);
		if (!domElement.isNull()) return domElement.text();
	}
	return QString();
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @param tagname name of XML-Tag
 * @return bool value of tagname or defaultVal if conversion fails
 */
bool eveXMLReader::getSMTagBool(int chain, int smid, QString tagname, bool defaultVal) {

	QString value = getSMTag(chain, smid, tagname);
	if (!value.isEmpty()){
		if ((value.compare("yes", Qt::CaseInsensitive) == 0) || (value.compare("true", Qt::CaseInsensitive) == 0))
			return true;
		else
			return false;
	}
	return defaultVal;
}

/**
 *
 * @param chain chain id
 * @param smid scanmodule id
 * @param tagname name of XML-Tag
 * @return double value of tagname or defaultVal if conversion fails
 */
double eveXMLReader::getSMTagDouble(int chain, int smid, QString tagname, double defaultVal) {

	QString value = getSMTag(chain, smid, tagname);
	if (!value.isEmpty()){
		bool ok=false;
		double dval = value.toDouble(&ok);
		if (ok) return dval;
	}
	return defaultVal;
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

QHash<QString, QString>* eveXMLReader::getChainPlugin(int chain, QString tagname){

	QHash<QString, QString>* pluginHash = new QHash<QString, QString>;
	if (chainDomIdHash.contains(chain)) {
		QDomElement domElement = chainDomIdHash.value(chain);
		domElement = domElement.firstChildElement(tagname);
		if (!domElement.isNull()){
			getPluginData(domElement, pluginHash);
		}
	}
	return pluginHash;
}

QHash<QString, QString>* eveXMLReader::getPositioningPlugin(int chain, int smid, QString tagname){

	QHash<QString, QString>* pluginHash = new QHash<QString, QString>;
	if (smIdHash.contains(chain)){
		QDomElement domSMRoot = smIdHash.value(chain)->value(smid);
		QDomElement	domElement = domSMRoot.firstChildElement("positioning");
		if (!domElement.isNull()){
			QDomElement	domParam = domElement.firstChildElement("axis_id");
			if (!domParam.isNull()) pluginHash->insert("axis_id", domParam.text().trimmed());
			domParam = domElement.firstChildElement("channel_id");
			if (!domParam.isNull()) pluginHash->insert("channel_id", domParam.text().trimmed());
			domParam = domElement.firstChildElement("normalize_id");
			if (!domParam.isNull()) pluginHash->insert("normalize_id", domParam.text().trimmed());
			QDomElement	domPlugin = domElement.firstChildElement("plugin");
			if (!domPlugin.isNull()){
				getPluginData(domPlugin, pluginHash);
			}
		}
	}
	return pluginHash;
}

void eveXMLReader::getPluginData(QDomElement domPlugin, QHash<QString, QString>* pluginHash){

	if (domPlugin.hasAttribute("name")) {
		pluginHash->insert("pluginname", domPlugin.attribute("name").trimmed());
	}
	QDomElement domElement = domPlugin.firstChildElement("parameter");
	while (!domElement.isNull()) {
		if (domElement.hasAttribute("name")) {
			pluginHash->insert(domElement.attribute("name").trimmed(), domElement.text().trimmed());
		}
		domElement = domElement.nextSiblingElement("parameter");
	}

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
	domElement = domElement.firstChildElement("smaxis");
	while (!domElement.isNull()) {
		bool prependElement=true;
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
		QDomElement domPosMode = domElement.firstChildElement("positionmode");
		bool absolute = true;
		if (!domPosMode.isNull()){
			if (domPosMode.text() == "absolute")
				absolute=true;
			else if (domPosMode.text() == "relative")
				absolute = false;
			else
				sendError(MINOR, 0, QString("invalid positionmode string in XML %1").arg(domPosMode.text()));
		}
		evePosCalc *poscalc = new evePosCalc(scanmodule, stepfunction, absolute, axisType);

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
		else if (!domElement.firstChildElement("plugin").isNull()) {
			prependElement=false;
			QHash<QString, QString> paraHash;
			QDomElement domContr = domElement.firstChildElement("plugin");
			getPluginData(domContr, &paraHash);
			if (domContr.attribute("name").trimmed().length() > 1)
				poscalc->setStepPlugin(domContr.attribute("name"), paraHash);
			else
				sendError(ERROR,0,"Step Plugin: invalid name");
		}
		else
			sendError(ERROR,0,"No values found in XML to calculate motor positions");

		// order is important, since we must make sure all axes with step plugins
		// which might use reference axes must be called after their reference axes
		if (prependElement)
			axislist->prepend(new eveSMAxis(scanmodule, axisDefinition, poscalc));
		else
			axislist->append(new eveSMAxis(scanmodule, axisDefinition, poscalc));

		domElement = domElement.nextSiblingElement("smaxis");
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
	domElement = domElement.firstChildElement("smchannel");
	while (!domElement.isNull()) {
		QHash<QString, QString> paraHash;
		QDomElement domId = domElement.firstChildElement("channelid");
		eveDetectorChannel* channelDefinition = deviceList->getChannelDef(domId.text());
		if (channelDefinition == NULL){
			sendError(ERROR,0,QString("no channeldefinition found for %1").arg(domId.text()));
			return channellist;
		}
		QList<eveEventProperty* > *eventList = new QList<eveEventProperty* >;
		domId = domElement.firstChildElement();
		while (!domId.isNull()){
			if (domId.nodeName() == "redoevent"){
				// TODO get event
				eveEventProperty* event = getEvent(eveEventProperty::REDO, domId);
				if (event != NULL ) eventList->append(event);
			}
			else {
				paraHash.insert(domId.nodeName(), domId.text().trimmed());
				domId = domId.nextSiblingElement();
			}
		}

		channellist->append(new eveSMChannel(scanmodule, channelDefinition, paraHash, eventList));
		domElement = domElement.nextSiblingElement("smchannel");
	}
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getChannelList").arg(e.what()));
	}
	return channellist;
}

QList<eveEventProperty*>* eveXMLReader::getSMEventList(int chain, int smid){

	QList<eveEventProperty* > *eventList = new QList<eveEventProperty* >;

	try
	{
		if (!smIdHash.contains(chain)) return eventList;
		QDomElement domElement = smIdHash.value(chain)->value(smid);
		QDomElement domEvent = domElement.firstChildElement("redoevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::REDO, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("redoevent");
		}
		domEvent = domElement.firstChildElement("breakevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::BREAK, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("breakevent");
		}
		domEvent = domElement.firstChildElement("triggerevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::TRIGGER, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("triggerevent");
		}
		domEvent = domElement.firstChildElement("pauseevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::PAUSE, domEvent);
			QDomElement signalOffToo = domElement.firstChildElement("continue_if_false");
			if (!signalOffToo.isNull()){
				bool signalOff = false;
				if (signalOffToo.text() == "true") signalOff = true;
				event->setSignalOff(signalOff);
			}
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("pauseevent");
		}
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getSMEventList").arg(e.what()));
	}
	return eventList;
}

QList<eveEventProperty*>* eveXMLReader::getChainEventList(int chain){

	QList<eveEventProperty* > *eventList = new QList<eveEventProperty* >;

	try
	{
		if (!chainDomIdHash.contains(chain)) return eventList;
		QDomElement domElement = chainDomIdHash.value(chain);
		QDomElement domEvent = domElement.firstChildElement("startevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::START, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("startevent");
		}
		domEvent = domElement.firstChildElement("redoevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::REDO, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("redoevent");
		}
		domEvent = domElement.firstChildElement("breakevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::BREAK, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("breakevent");
		}
		domEvent = domElement.firstChildElement("stopevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::STOP, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("stopevent");
		}
		domEvent = domElement.firstChildElement("pauseevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::PAUSE, domEvent);
			QDomElement signalOffToo = domElement.firstChildElement("continue_if_false");
			if (!signalOffToo.isNull()){
				bool signalOff = false;
				if (signalOffToo.text() == "true") signalOff = true;
				event->setSignalOff(signalOff);
			}
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("pauseevent");
		}
	}
	catch (std::exception& e)
	{
		printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getChainEventList").arg(e.what()));
	}
	return eventList;
}

//eveEventTypeT eveXMLReader::getEventType(QString evtype){
//	if (evtype == "monitor")
//		return eveEventMONITOR;
//	else if (evtype == "detector")
//		return eveEventDetector;
//	else
//		return eveEventSCHEDULE;
//}

eveEventProperty* eveXMLReader::getEvent(eveEventProperty::actionTypeT action, QDomElement domElement){

 	if (!domElement.hasAttribute("type")){
		sendError(ERROR, 0, "getEvent: event tags must have an attribute type");
		return NULL;
 	}

 	if (domElement.attribute("type") == "schedule"){
		eventTypeT eventType = eveEventTypeSCHEDULE;
		incidentTypeT incident = eveIncidentEND;
		int chid=0, smid=0;
		QDomElement domIncident = domElement.firstChildElement("incident");
		if ((!domIncident.isNull()) && (domIncident.text() == "Start")) incident = eveIncidentSTART;
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
		sendError(DEBUG, 0, QString("found schedule event definition, chid %1, smid %2").arg(chid).arg(smid));
		return new eveEventProperty("", "", eveVariant(QVariant(eveVariant::getMangled(chid,smid))), eventType, incident, action, NULL);
	}
 	if (domElement.attribute("type") == "detector"){
		eventTypeT eventType = eveEventTypeDETECTOR;

		QDomElement domId = domElement.firstChildElement("id");
		if (domId.isNull()){
			sendError(ERROR, 0, "getEvent: need <id> tag for detector event");
			return NULL;
		}

		QString eventId = domId.text().trimmed();
		QRegExp regex = QRegExp("^D-\\d+-\\d+-(\\w+)$");
		if (!(eventId.trimmed().contains(regex) && (regex.numCaptures() > 2))){
			sendError(ERROR, 0, QString("get Detector Event: invalid detector id").arg(eventId));
			return NULL;
		}

		int chid=0, smid=0;
		bool cok, sok;
		chid = regex.capturedTexts().at(0).toInt(&cok);
		smid = regex.capturedTexts().at(1).toInt(&sok);
		if (!cok || !sok){
			sendError(ERROR, 0, "getEvent: error extracting chainid or smid from detector event id");
			return NULL;
		}
		QString devname = regex.capturedTexts().at(2);
		eveDevice* deviceDef = deviceList->getDeviceDef(devname);
		if ((deviceDef == NULL) || (deviceDef->getValueCmd() == NULL)){
			sendError(ERROR, 0, QString("get Detector event: no or invalid device definition found for %1").arg(eventId));
			return NULL;
		}

		sendError(DEBUG, 0, QString("found detector event definition for %1").arg(eventId));
		return new eveEventProperty(eventId, devname, eveVariant(QVariant(eveVariant::getMangled(chid,smid))), eventType, eveIncidentNONE, action, NULL);
	}
	else if (domElement.attribute("type") == "monitor"){
	 	eventTypeT eventType = eveEventTypeMONITOR;

	 	QDomElement domId = domElement.firstChildElement("id");
		if (domId.isNull()){
			sendError(ERROR, 0, "getEvent: need <id> tag for monitor event");
			return NULL;
		}

	 	eveDevice* deviceDef = deviceList->getDeviceDef(domId.text());
		if ((deviceDef == NULL) || (deviceDef->getValueCmd() == NULL)){
			sendError(ERROR, 0, QString("getEvent: no or invalid device definition found for %1").arg(domId.text()));
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
		QString name = deviceDef->getName();

		QString comparison = domLimit.attribute("comparison");

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
		if (!ok) sendError(ERROR, 0, QString("getEvent: error converting type attribute of <limit> tag of event with id %1").arg(domId.text()));

		sendError(DEBUG, 0, QString("found monitor event definition for %1").arg(name));
		return new eveEventProperty(name, comparison, limitVal, eventType, eveIncidentNONE, action, deviceDef->getValueCmd()->clone());
	}
	else{
		sendError(ERROR, 0, "getEvent: invalid attribute type");
		return NULL;
	}
}

/**
 * returns a list with all detector/xaxis/normalize_id with corresponding plot id.
 * The list is sorted in tree-hierarchy. List-elements with equal detector/xaxis/normalize_id/plotwindow
 * and preinit == false are packed together into one mathConfig.
 * The calculation and plotting will extend over more than one scanmodule only if all
 * devices are the same in all plots and init == false in all plots except the first;
 * Only on detector/xaxis pairs which are plotted math is applied.
 *
 * @param chid
 * @return
 */
QList<eveMathConfig*>* eveXMLReader::getFilteredMathConfigs(int chid){

	QList<eveMathConfig*> *mathConfigList = new QList<eveMathConfig*>();
	int smid = getRootId(chid);

	getMathConfigFromPlot(chid, smid, mathConfigList);

	int index = 0;
	while (index < mathConfigList->size()){
		eveMathConfig* math = mathConfigList->at(index);
		int next = index +1;
		while (next < mathConfigList->size()){
			eveMathConfig* mathNext = mathConfigList->at(next);
			if (math->getPlotWindowId() == mathNext->getPlotWindowId()){
				if (math->hasEqualDevices(*mathNext) && !mathNext->hasInit()) {
					math->addScanModule(mathNext->getFirstScanModuleId());
					mathConfigList->removeAt(next);
					delete mathNext;
				}
				else {
					break;
				}
			}
			else {
				next++;
			}
		}
		index++;
	}
	return mathConfigList;
}

void eveXMLReader::getMathConfigFromPlot(int chid, int smid, QList<eveMathConfig*> *mathConfigList) {

	if (smIdHash.contains(chid)){
		QDomElement domElement = smIdHash.value(chid)->value(smid);
		QDomElement domPlot = domElement.firstChildElement("plot");
		while (!domPlot.isNull()) {
			bool ok;
			QString xAxisId;
			QString yAxisId;
			QString normalizeId;
			int plotId = domPlot.attribute("id").toInt(&ok);
			bool init = getSMTagBool(chid, smid, "init", true);
			domElement = domPlot.firstChildElement("xaxis");
			if (!domElement.isNull()) domElement = domElement.firstChildElement("id");
			if (!domElement.isNull()) xAxisId = domElement.text();
			QDomElement domYAxis = domPlot.firstChildElement("yaxis");
			ok = (ok && (xAxisId.length() > 0));
			while (ok && !domYAxis.isNull()){
				domElement = domYAxis.firstChildElement("id");
				if (!domElement.isNull()) yAxisId = domElement.text();
				domElement = domYAxis.firstChildElement("normalize_id");
				if (!domElement.isNull()) normalizeId = domElement.text();
				if (yAxisId.length() > 0){
					sendError(DEBUG, 0, QString("MathConfig: %1, %2, %3, Plot: %4, init: %5").arg(xAxisId).arg(yAxisId).arg(normalizeId).arg(plotId).arg(init));
					eveMathConfig *mathConfig = new eveMathConfig(chid, plotId, init, xAxisId);
					mathConfig->addScanModule(smid);
					mathConfig->addYAxis(yAxisId, normalizeId);
					mathConfigList->append(mathConfig);
				}
				domYAxis = domYAxis.nextSiblingElement("yaxis");
			}
			domPlot = domPlot.nextSiblingElement("plot");
		}
	}
	if (getNested(chid, smid) != 0) getMathConfigFromPlot(chid, getNested(chid, smid), mathConfigList);
	if (getAppended(chid, smid) != 0) getMathConfigFromPlot(chid, getAppended(chid, smid), mathConfigList);
}
