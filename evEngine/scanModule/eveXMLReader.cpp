/*
 * eveXMLReader.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include <QDomDocument>
#include <QDomNodeList>
#include <QIODevice>
#include <QString>
#include "eveTypes.h"
#include "eveXMLReader.h"

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


/** \brief read XML-Data and create all devices, hashes etc.
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
    if (version.text() != "0.1.5") {
		sendError(ERROR,0,QString("eveXMLReader::read: incompatible xml version: %1").arg(version.text()));
        return false;
    }
    // build indices
    // one Hash with DomElement / chain-id

    QDomElement domElem = root.firstChildElement("chain");
	while (!domElem.isNull()) {
     	if (domElem.hasAttribute("id")) {
     		QString typeString = domElem.attribute("id");
     		int number = typeString.toInt();
     		if (number < 1) {
     			sendError(ERROR,0,"eveXMLReader::read: chain id must be > 0 %1");
     		}
     		else {
     			chainDomIdHash.insert(number, domElem);
     		}
     	}
		domElem = domElem.nextSiblingElement("chain");
	}

    domElem = root.firstChildElement("detectors");
    QDomNodeList detectorList = domElem.childNodes();
    for (unsigned int i=0; i < detectorList.length(); ++i) createDetector(detectorList.item(i));
    //    sendError(INFO,0,QString("eveXMLReader::read: found %1 detectorChannels").arg(channelDefinitions.size()));

    domElem = root.firstChildElement("motors");
    QDomNodeList motorList = domElem.childNodes();
    for (unsigned int i=0; i < motorList.length(); ++i) createMotor(motorList.item(i));

    domElem = root.firstChildElement("devices");
    QDomNodeList deviceList = domElem.childNodes();
    for (unsigned int i=0; i < deviceList.length(); ++i) createDevice(deviceList.item(i));

    return true;
}

/** \brief create an eveDetector from XML
 * \param detector the <detector> DomNode
 */
void eveXMLReader::createDetector(QDomNode detector){

	QString name;
	QString id;
	eveDeviceCommand *trigger=NULL;
	eveDeviceCommand *unit=NULL;
	eveSimpleDetector* channel;

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
    if (!domElement.isNull()) unit = createDeviceCommand(domElement);

    domElement = detector.firstChildElement("channel");
	while (!domElement.isNull()) {
		channel = createChannel(domElement, trigger, unit);
		if (channel != NULL) deviceList->insert(channel->getId(),channel);
	    sendError(INFO,0,QString("eveXMLReader::createDetector: channel-id: %1").arg(channel->getId()));
		domElement = domElement.nextSiblingElement("channel");
	}

	domElement = detector.firstChildElement("option");
	while (!domElement.isNull()) {
		createDevice(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}
    sendError(INFO,0,QString("eveXMLReader::createDetector: id: %1, name: %2").arg(id).arg(name));
}

/** \brief create a CaTransport from XML
 * \param node the <pv> DomNode
 */
eveCaTransport * eveXMLReader::createCaTransport(QDomElement node)
{
	QString typeString;
	QString methodString;
	eveType pvtype = eveInt8T;
	pvMethodT pvMethod= eveGET;

	if (node.hasAttribute("type")) {
		typeString = node.attribute("type");
		if (typeString == "int") pvtype = eveInt32T;
		else if (typeString == "double") pvtype = eveFloat64T;
		else if (typeString == "string") pvtype = eveStringT;
		else {
			sendError(ERROR,0,QString("eveXMLReader::createCaTransport: unknown data type: %1").arg(typeString));
		}
	}
	if (node.hasAttribute("method")) {
		methodString = node.attribute("method");
		if (methodString == "PUT") pvMethod = evePUT;
		else if (methodString == "GET") pvMethod = eveGET;
		else if (methodString == "GETPUT") pvMethod = eveGETPUT;
		else if (methodString == "GETCB") pvMethod = eveGETCB;
		else if (methodString == "PUTCB") pvMethod = evePUTCB;
		else if (methodString == "GETPUTCB") pvMethod = eveGETPUTCB;
		else {
			sendError(ERROR,0,QString("eveXMLReader::createCaTransport: unknown method: %1").arg(methodString));
		}
	}
	return new eveCaTransport(pvtype, pvMethod, node.text());

}

/** \brief create a Detector Channel from XML
 * \param node the <channel> DomNode
 * \param defaultTrigger the default trigger if avail. else NULL
 * \param defaultUnit the default unit if avail. else NULL
 */
eveSimpleDetector * eveXMLReader::createChannel(QDomNode channel, eveDeviceCommand *defaultTrigger, eveDeviceCommand *defaultUnit){

	// name, id, readpv, unit, trigger
	QString name;
	QString id;
	eveDeviceCommand *trigger=defaultTrigger;
	eveDeviceCommand *unit=defaultUnit;
	eveCaTransport * pv=NULL;

	QDomElement domElement = channel.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = channel.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;

	domElement = channel.firstChildElement("readpv");
	if (!domElement.isNull()) pv = createCaTransport(domElement);
    if (pv == NULL) {
        sendError(ERROR,0,"eveXMLReader::createChannel: Syntax error in channel tag, no <readpv>");
        return NULL;
    }

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

	return new eveSimpleDetector(trigger, unit, pv, name, id);

}

void eveXMLReader::sendError(int severity, int errorType,  QString errorString){
	if (parent != NULL) {
		parent->sendError(severity, EVEMESSAGEFACILITY_XMLPARSER, errorType, errorString);
	}
}

/** \brief create an eveMotor from XML
 * \param motor the <motor> DomNode
 */
void eveXMLReader::createMotor(QDomNode motor){

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
    if (!domElement.isNull()) unit = createDeviceCommand(domElement);

    domElement = motor.firstChildElement("axis");
	while (!domElement.isNull()) {
		axis = createAxis(domElement, trigger, unit);
		if (axis != NULL) deviceList->insert(axis->getId(),axis);
	    sendError(INFO,0,QString("eveXMLReader::createMotor: axis-id: %1").arg(axis->getId()));
		domElement = domElement.nextSiblingElement("axis");
	}

	domElement = motor.firstChildElement("option");
	while (!domElement.isNull()) {
		createDevice(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}
    sendError(INFO,0,QString("eveXMLReader::createMotor: id: %1, name: %2").arg(id).arg(name));
}


/** \brief create a Detector Axis from XML
 * \param node the <axis> DomNode
 * \param defaultTrigger the default trigger if avail. else NULL
 * \param defaultUnit the default unit if avail. else NULL
 */
eveMotorAxis * eveXMLReader::createAxis(QDomNode axis, eveDeviceCommand *defaultTrigger, eveDeviceCommand *defaultUnit){

	QString name;
	QString id;
	eveDeviceCommand *trigger=defaultTrigger;
	eveDeviceCommand *unit=defaultUnit;
	eveDeviceCommand *gotoCommand = NULL;
	eveDeviceCommand *stopCommand = NULL;
	eveCaTransport * positionPv=NULL;
	eveCaTransport * statusPv=NULL;

	QDomElement domElement = axis.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = axis.firstChildElement("name");
    if (!domElement.isNull())
    	name = domElement.text();
    else
    	name = id;

	domElement = axis.firstChildElement("positionpv");
	if (!domElement.isNull()) positionPv = createCaTransport(domElement);
    if (positionPv == NULL) {
        sendError(INFO,0,QString("eveXMLReader::createAxis: unable to verify position no <positionpv> tag for %1").arg(name));
    }

	domElement = axis.firstChildElement("statuspv");
	if (!domElement.isNull()) statusPv = createCaTransport(domElement);
    if (statusPv == NULL) {
        sendError(INFO,0,QString("eveXMLReader::createAxis: unable to check status no <statuspv> tag for %1").arg(name));
    }

    domElement = axis.firstChildElement("goto");
    if (!domElement.isNull()) gotoCommand = createDeviceCommand(domElement);
    if (gotoCommand == NULL) {
        sendError(ERROR,0,QString("eveXMLReader::createAxis: unable to create goto command for %1, check XML").arg(name));
        return NULL;
    }

    domElement = axis.firstChildElement("stop");
    if (!domElement.isNull()) stopCommand = createDeviceCommand(domElement);
    if (gotoCommand == NULL) {
        sendError(ERROR,0,QString("eveXMLReader::createAxis: unable to create stop command for %1, check XML").arg(name));
        return NULL;
    }

    domElement = axis.firstChildElement("trigger");
    if (!domElement.isNull())
    	trigger = createDeviceCommand(domElement);
    else { // every device needs its private copy of default trigger
    	if (trigger != NULL) trigger = trigger->clone();
    }

    domElement = axis.firstChildElement("unit");
    if (!domElement.isNull())
    	unit = createDeviceCommand(domElement);
    else { // every device needs its private copy of default unit
    	if (unit != NULL) unit = unit->clone();
    }

	return new eveMotorAxis(trigger, unit, gotoCommand, stopCommand, positionPv, statusPv, name, id);

}

/** \brief create a device command from XML
 * \param node the corresponding DomNode e.g. <goto>, <unit>, <stop>
 * \return deviceCommand (goto, unit, stop etc.)
 *
 */
eveDeviceCommand * eveXMLReader::createDeviceCommand(QDomNode node){
	eveCaTransport * pv=NULL;
	QString valueString=NULL;
	eveType pvtype=eveUnknownT;

	QDomElement domElement = node.firstChildElement("pv");
	if (!domElement.isNull()) pv = createCaTransport(domElement);

    domElement = node.firstChildElement("value");
    if (!domElement.isNull()){
     	valueString = domElement.text();
     	if (domElement.hasAttribute("type")) {
     		QString typeString = domElement.attribute("type");
     		if (typeString == "int") pvtype = eveInt32T;
     		else if (typeString == "double") pvtype = eveFloat64T;
     		else if (typeString == "string") pvtype = eveStringT;
     		else {
     			sendError(ERROR,0,QString("eveXMLReader::createCommand: unknown data type: %1").arg(typeString));
     		}
     	}
    }
	return new eveDeviceCommand(pv, valueString, pvtype);
}

/** \brief create an option or a device from XML
 * \param node the <option> or <device> DomNode
 */
void eveXMLReader::createDevice(QDomNode device){

	QString name;
	QString id;
	eveDeviceCommand *unit=NULL;
	eveTransport* valuePv;

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

    domElement = device.firstChildElement("pv");
	if (!domElement.isNull()) valuePv = createCaTransport(domElement);
    if (valuePv == NULL) {
        sendError(ERROR,0,QString("eveXMLReader::createDevice: need a valid <pv> tag for %1").arg(name));
        return;
    }
    deviceList->insert(id, new eveDevice(unit, valuePv, name, id));
	// TODO remove this
    sendError(INFO,0,QString("eveXMLReader::createDevice: Found id: %1, name: %2").arg(id).arg(name));
}


