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
#include "eveError.h"
#include "eveParameter.h"
#include "eveSMChannel.h"
#include "eveSMDetector.h"
#include "eveSMMotor.h"

#define EVE_XML_VERSION        2
#define EVE_XML_REVISION       3

eveXMLReader::eveXMLReader(eveManager *parentObject){
	parent = parentObject;
	domDocument = new QDomDocument();
	repeatCount=0;

}

eveXMLReader::~eveXMLReader() {
    try
    {
        if (domDocument != NULL) delete domDocument;
        deviceList.clearAll();
        foreach (int key, smIdHash.keys()){
            delete smIdHash.value(key);
        }
        smIdHash.clear();
    }
    catch (std::exception& e)
    {
        sendError(ERROR, 0, QString("C++ Exception: %1 in eveXMLReader destructor").arg(e.what()));
    }

}

/** \brief read XML-Data and create all device definitions and various hashes etc.
 * \param xmldata XML text data
 */
bool eveXMLReader::read(QByteArray xmldata)
{
    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!domDocument->setContent(xmldata, true, &errorStr, &errorLine, &errorColumn)) {
		sendError(ERROR,0,QString("eveXMLReader::read: Parse error at line %1, column %2: Text: %3")
                .arg(errorLine).arg(errorColumn).arg(errorStr));
        return false;
    }

    QDomElement root = domDocument->documentElement();
    eveError::log(DEBUG, QString("ScanModule-File root %1").arg(root.tagName()).toAscii().data());
    if (root.tagName() != "scml") {
		sendError(ERROR,0,QString("eveXMLReader::read: file is not a scml file, it is %1").arg(root.tagName()));
        return false;
    }
    QDomElement version = root.firstChildElement("version");
    if (version.isNull()){
		sendError(ERROR,0,QString("eveXMLReader::read: no version tag found"));
        return false;
    }
    eveParameter::setParameter("xmlversion",version.text());
    QString thisVersion = QString("%1.%2").arg((int)EVE_XML_VERSION).arg((int)EVE_XML_REVISION);
    QStringList versions = version.text().split(".");
    if ((versions.count() > 1) && (versions[0].toInt() != EVE_XML_VERSION)) {
		sendError(ERROR,0,QString("eveXMLReader::read: incompatible xml versions, file: %1, application: %2").arg(version.text()).arg(thisVersion));
        return false;
    }
    if ((versions.count() > 1) && (versions[1].toInt() != EVE_XML_REVISION)) {
		sendError(MINOR,0,QString("eveXMLReader::read: different xml revisions, file: %1, application: %2").arg(version.text()).arg(thisVersion));
    }
    QDomElement locationElem = root.firstChildElement("location");
    if (!locationElem.isNull()){
    	eveParameter::setParameter("location",locationElem.text());
    }

    QDomElement scanElem = root.firstChildElement("scan");
    if (!scanElem.isNull()){
      QDomElement repeatcount = scanElem.firstChildElement("repeatcount");
      if (repeatcount.isNull()){
        sendError(ERROR,0,QString("eveXMLReader::read: no repeatcount tag found"));
        return false;
      }
      bool ok = true;
      repeatCount = repeatcount.text().toInt(&ok);
      if (!ok) repeatCount=0;

      // build indices
      // one Hash with DomElement / chain-id
      // one Hash with DomElement / sm-id for every chain
      // one Hash with previousHash /chain-id
      QDomElement domElem = scanElem.firstChildElement("chain");
      while (!domElem.isNull()) {
        if (domElem.hasAttribute("id")) {
          QString typeString = domElem.attribute("id");
          int chainNo = typeString.toInt();
          if (chainIdList.contains(chainNo)){
            sendError(ERROR,0,QString("duplicate chainId %1").arg(chainNo));
          }
          else {
            chainIdList.append(chainNo);
          }
          chainDomIdHash.insert(chainNo, domElem);
          smIdHash.insert(chainNo, new QHash<int, QDomElement> );
          QDomElement domSM = domElem.firstChildElement("scanmodules");
          domSM = domSM.firstChildElement("scanmodule");
          while (!domSM.isNull()) {
            if (domSM.hasAttribute("id")) {
              unsigned int smNo = QString(domSM.attribute("id")).toUInt();
              QDomElement domParent = domSM.firstChildElement("parent");
              if (!domParent.isNull()) {
                int parentNumber = domParent.text().toInt(&ok);
                // if parent == 0 insert in root-list
                if (ok && (parentNumber == 0)) rootSMHash.insert(chainNo,smNo);
                // ignore all SMs with parent < 0
                if (ok && (parentNumber >= 0)) (smIdHash.value(chainNo))->insert(smNo, domSM);
              }
            }
            domSM = domSM.nextSiblingElement("scanmodule");
          }
          if (!rootSMHash.contains(chainNo)){
            sendError(ERROR,0,QString("no root scanmodule found for chain %1").arg(chainNo));
            return false;
          }

          // check if we have a save plugin with location parameter nosave
          QDomElement domSavPlugin = domElem.firstChildElement("saveplugin");
          if (!domSavPlugin.isNull()) {
            QDomElement pluginParam = domSavPlugin.firstChildElement("parameter");
            while (!pluginParam.isNull()) {
              if (pluginParam.hasAttribute("name")) {
                if (pluginParam.attribute("name") == "location"){
                  if (pluginParam.text().toLower().trimmed() == "nosave") noSaveCidList.append(chainNo);
                }
              }
              pluginParam = pluginParam.nextSiblingElement("parameter");
            }
          }
        }
        domElem = domElem.nextSiblingElement("chain");
      }
      // get the list of ids which should be monitored
      domElem = scanElem.firstChildElement("monitoroptions");
      if (!domElem.isNull()) {
        QDomElement domId = domElem.firstChildElement("id");
//        bool doMonitorList = true;
//        if (domElem.hasAttribute("type") && (domElem.attribute("type") == "none")) doMonitorList = false;
//        while (doMonitorList && !domId.isNull()) {
        while (!domId.isNull()) {
          if (domId.text().length() > 0) monitorList.append(domId.text());
          domId = domId.nextSiblingElement("id");
        }
      }
    }

    QDomElement domElem = root.firstChildElement("detectors");
    QDomNodeList detectorNodeList = domElem.childNodes();
    for (unsigned int i=0; i < detectorNodeList.length(); ++i) createDetectorDefinition(detectorNodeList.item(i));

	domElem = root.firstChildElement("motors");
	QDomNodeList motorNodeList = domElem.childNodes();
	for (unsigned int i=0; i < motorNodeList.length(); ++i) createMotorDefinition(motorNodeList.item(i));

	domElem = root.firstChildElement("devices");
	if (!domElem.isNull()) {
		QDomElement domOption = domElem.firstChildElement("device");
		while (!domOption.isNull()) {
			createDeviceDefinition(domOption);
			domOption = domOption.nextSiblingElement("device");
		}
	}
	return true;
}

/** \brief create a Transport from XML
 * \param node the <access> DomNode
 */
eveTransportDefinition * eveXMLReader::createTransportDefinition(QDomElement node)
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
	return new eveTransportDefinition(transport, accesstype, accessMethod, timeout, node.text());

}

/** \brief create an eveDetectorDefinition from XML
 * \param detector the <detector> DomNode
 */
void eveXMLReader::createDetectorDefinition(QDomNode detector){

	QString name;
	QString id;
	eveCommandDefinition *trigger=NULL;
	eveCommandDefinition *unit=NULL;
	eveCommandDefinition *stop=NULL;
	eveChannelDefinition* channel;

	if (detector.isNull()){
		sendError(INFO,0,"eveXMLReader::eveDetectorDefinition: cannot create Null detector, check XML-Syntax");
		return;
	}
	QDomElement domElement = detector.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = detector.firstChildElement("name");
    if (!domElement.isNull()) name = domElement.text();

    domElement = detector.firstChildElement("trigger");
    if (!domElement.isNull()) trigger = createDeviceCommand(domElement);

    domElement = detector.firstChildElement("stop");
    if (!domElement.isNull()) stop = createDeviceCommand(domElement);

    domElement = detector.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
            sendError(DEBUG,0,QString("eveXMLReader::eveDetectorDefinition: found unit with string %1").arg(unitstring.text()));
    		unit = new eveCommandDefinition(NULL, unitstring.text(), eveStringT);
    	}
    }
    eveDetectorDefinition* detectDef = new eveDetectorDefinition(name, id, trigger, unit, stop);

    domElement = detector.firstChildElement("channel");
	while (!domElement.isNull()) {
		channel = createChannelDefinition(domElement, detectDef);
		if (channel != NULL) deviceList.insert(channel->getId(),channel);
	    sendError(INFO,0,QString("eveXMLReader::eveDetectorDefinition: channel-id: %1").arg(channel->getId()));
		domElement = domElement.nextSiblingElement("channel");
	}

	domElement = detector.firstChildElement("option");
	while (!domElement.isNull()) {
		createDeviceDefinition(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}
    sendError(INFO,0,QString("eveXMLReader::eveDetectorDefinition: id: %1, name: %2").arg(id).arg(name));
}


/** \brief create a Detector Channel from XML
 * \param node the <channel> DomNode
 * \param defaultTrigger the default trigger if avail. else NULL
 * \param defaultUnit the default unit if avail. else NULL
 */
eveChannelDefinition * eveXMLReader::createChannelDefinition(QDomNode channel, eveDetectorDefinition* detectorDef){

	// name, id, read, unit, trigger
	QString name;
	QString id;
	eveCommandDefinition *trigger = NULL;
	eveCommandDefinition *unit = NULL;
	eveCommandDefinition *read = NULL;
	eveCommandDefinition *stop = NULL;

	QDomElement domElement = channel.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = channel.firstChildElement("name");
    if (!domElement.isNull()) name = domElement.text();

	domElement = channel.firstChildElement("read");
	if (!domElement.isNull())
		read = createDeviceCommand(domElement);
	else
        sendError(ERROR,0,"eveXMLReader::createChannel: Syntax error in channel tag, no <read>");

    domElement = channel.firstChildElement("trigger");
    if (!domElement.isNull()) trigger = createDeviceCommand(domElement);

    domElement = channel.firstChildElement("stop");
    if (!domElement.isNull()) stop = createDeviceCommand(domElement);

    domElement = channel.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
    		unit = new eveCommandDefinition(NULL, unitstring.text(), eveStringT);
    	}
    }
    domElement = channel.firstChildElement("option");
	while (!domElement.isNull()) {
		createDeviceDefinition(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}

	return new eveChannelDefinition(detectorDef, trigger, unit, read, stop, name, id);

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
	eveCommandDefinition *trigger=NULL;
	eveCommandDefinition *unit=NULL;
	eveAxisDefinition* axis;

	if (motor.isNull()){
		sendError(INFO,0,"eveXMLReader::createMotorDefinition: cannot create Null motor, check XML-Syntax");
		return;
	}
	QDomElement domElement = motor.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = motor.firstChildElement("name");
    if (!domElement.isNull()) name = domElement.text();

    domElement = motor.firstChildElement("trigger");
    if (!domElement.isNull()) trigger = createDeviceCommand(domElement);

    domElement = motor.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
    		unit = new eveCommandDefinition(NULL, unitstring.text(), eveStringT);
    	}
    }

    eveMotorDefinition* motorDef = new eveMotorDefinition(name, id, trigger, unit);

    domElement = motor.firstChildElement("axis");
	while (!domElement.isNull()) {
		axis = createAxisDefinition(domElement, motorDef);
		if (axis != NULL) deviceList.insert(axis->getId(),axis);
	    sendError(INFO,0,QString("eveXMLReader::createMotorDefinition: axis-id: %1").arg(axis->getId()));
		domElement = domElement.nextSiblingElement("axis");
	}

	domElement = motor.firstChildElement("option");
	while (!domElement.isNull()) {
		createDeviceDefinition(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}
    sendError(INFO,0,QString("eveXMLReader::createMotorDefinition: id: %1, name: %2").arg(id).arg(name));
}


/** \brief create a Detector Axis from XML
 * \param node the <axis> DomNode
 * \param defaultTrigger the default trigger if avail. else NULL
 * \param defaultUnit the default unit if avail. else NULL
 */
eveAxisDefinition * eveXMLReader::createAxisDefinition(QDomNode axis, eveMotorDefinition* motorDef){

	QString name;
	QString id;
	eveCommandDefinition *trigger = NULL;
	eveCommandDefinition *unit = NULL;
	eveCommandDefinition *stopCommand = NULL;
	eveCommandDefinition *gotoCommand=NULL;
	eveCommandDefinition *positionCommand=NULL;
	eveCommandDefinition *statusCommand=NULL;
	eveCommandDefinition *deadbandCommand=NULL;

	QDomElement domElement = axis.firstChildElement("id");
    if (!domElement.isNull()) id = domElement.text();

    domElement = axis.firstChildElement("name");
    if (!domElement.isNull()) name = domElement.text();

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

    domElement = axis.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
            sendError(DEBUG,0,QString("eveXMLReader::createAxis: found unit with string %1").arg(unitstring.text()));
    		unit = new eveCommandDefinition(NULL, unitstring.text(), eveStringT);
    	}
    }

	domElement = axis.firstChildElement("deadband");
	if (!domElement.isNull()) deadbandCommand = createDeviceCommand(domElement);
    if (deadbandCommand == NULL) {
        sendError(INFO,0,QString("eveXMLReader::createAxis: unable to check deadband, no <deadband> tag for %1").arg(name));
    }

    domElement = axis.firstChildElement("option");
	while (!domElement.isNull()) {
		createDeviceDefinition(domElement);
 		domElement = domElement.nextSiblingElement("option");
	}

	return new eveAxisDefinition(motorDef, trigger, unit, gotoCommand, stopCommand, positionCommand, statusCommand, deadbandCommand, name, id);

}

/** \brief create a device command from XML
 * \param node the corresponding DomNode e.g. <goto>, <unit>, <stop>
 * \return deviceCommand (goto, unit, stop etc.)
 *
 */
eveCommandDefinition * eveXMLReader::createDeviceCommand(QDomNode node){
	eveTransportDefinition *access=NULL;
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
	return new eveCommandDefinition(access, valueString, valuetype);
}

/** \brief create an option or a device from XML
 * \param node the <option> or <device> DomNode
 */
void eveXMLReader::createDeviceDefinition(QDomElement device){

	QString name;
	QString id;
	eveCommandDefinition *unit=NULL;
	eveCommandDefinition* valueTrans;

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
	if (device.hasAttribute("monitor") && (device.attribute("monitor").toLower() == "true")) monitorList.append(id);
    domElement = device.firstChildElement("name");
    if (!domElement.isNull()) name = domElement.text();

    domElement = device.firstChildElement("unit");
    if (!domElement.isNull()) unit = createDeviceCommand(domElement);

    domElement = device.firstChildElement("unit");
    if (!domElement.isNull()){
    	QDomElement unitstring = domElement.firstChildElement("unitstring");
    	if (unitstring.isNull()){
    		unit = createDeviceCommand(domElement);
    	}
    	else {
    		unit = new eveCommandDefinition(NULL, unitstring.text(), eveStringT);
    	}
    }

    domElement = device.firstChildElement("value");
	if (!domElement.isNull())
		valueTrans = createDeviceCommand(domElement);
	else {
		sendError(ERROR,0,QString("eveXMLReader::createDevice: need a valid <value> tag for %1").arg(name));
		return;
	}
	deviceList.insert(id, new eveDeviceDefinition(unit, valueTrans, name, id));

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
 * @return integer value of tagname or defaultVal if conversion fails
 */
int eveXMLReader::getSMTagInteger(int chain, int smid, QString tagname, int defaultVal) {

	QString value = getSMTag(chain, smid, tagname);
	if (!value.isEmpty()){
		bool ok=false;
		int ival = value.toInt(&ok);
		if (ok) return ival;
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

QHash<QString, QString> eveXMLReader::getChainPlugin(int chain, QString tagname){

	QHash<QString, QString> pluginHash;
	if (chainDomIdHash.contains(chain)) {
		QDomElement domElement = chainDomIdHash.value(chain);
		domElement = domElement.firstChildElement(tagname);
		if (!domElement.isNull()){
			getPluginData(domElement, pluginHash);
		}
	}
	return pluginHash;
}

QList<QHash<QString, QString>* >* eveXMLReader::getPositionerPluginList(int chain, int smid){

	QList<QHash<QString, QString>* >* posPluginDataList = new QList<QHash<QString, QString>* >;
	if (smIdHash.contains(chain)){
		QDomElement domSMRoot = smIdHash.value(chain)->value(smid);
		QDomElement	domElement = domSMRoot.firstChildElement("positioning");
		while (!domElement.isNull()){
			QHash<QString, QString>* pluginHash = new QHash<QString, QString>;
			QDomElement	domParam = domElement.firstChildElement("axis_id");
			if (!domParam.isNull()) pluginHash->insert("axis_id", domParam.text().trimmed());
			domParam = domElement.firstChildElement("channel_id");
			if (!domParam.isNull()) pluginHash->insert("channel_id", domParam.text().trimmed());
			domParam = domElement.firstChildElement("normalize_id");
			if (!domParam.isNull()) pluginHash->insert("normalize_id", domParam.text().trimmed());
			QDomElement	domPlugin = domElement.firstChildElement("plugin");
			if (!domPlugin.isNull()){
				getPluginData(domPlugin, *pluginHash);
			}
			domElement = domElement.nextSiblingElement("positioning");
			posPluginDataList->append(pluginHash);
		}
	}
	return posPluginDataList;
}

void eveXMLReader::getPluginData(QDomElement domPlugin, QHash<QString, QString>& pluginHash){

	if (domPlugin.hasAttribute("name")) {
		pluginHash.insert("pluginname", domPlugin.attribute("name").trimmed());
	}
	QDomElement domElement = domPlugin.firstChildElement("parameter");
	while (!domElement.isNull()) {
		if (domElement.hasAttribute("name")) {
			pluginHash.insert(domElement.attribute("name").trimmed(), domElement.text().trimmed());
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
		eveDeviceDefinition* devDef=NULL;

		QDomElement domId = domElement.firstChildElement("id");
		if (!domId.isNull())
			devDef = deviceList.getDeviceDef(domId.text());

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
	QHash<QString, eveSMMotor *> motorHash;

	try
	{
	if (!smIdHash.contains(chain)) return axislist;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	domElement = domElement.firstChildElement("smaxis");
	while (!domElement.isNull()) {
		bool prependElement=true;
		eveAxisDefinition* axisDefinition = NULL;
		eveSMMotor* motor = NULL;
		eveVariant startvalue;
		QString stepfunction = "none";
		QDomElement domId = domElement.firstChildElement("axisid");

		axisDefinition = deviceList.getAxisDef(domId.text());
		if (axisDefinition == NULL){
			sendError(ERROR,0,QString("no axisdefinition found for %1").arg(domId.text()));
                        domElement = domElement.nextSiblingElement("smaxis");
			continue;
		}
		eveMotorDefinition* motorDefinition = axisDefinition->getMotorDefinition();
		if (motorHash.contains(motorDefinition->getId())){
			motor = motorHash.value(motorDefinition->getId());
		}
		else {
			motor = new eveSMMotor(scanmodule, motorDefinition);
			motorHash.insert(motorDefinition->getId(), motor);
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

                if (!domElement.firstChildElement("startstopstep").isNull()) {
                    QDomElement domststst = domElement.firstChildElement("startstopstep");
                        poscalc->setStartPos(domststst.firstChildElement("start").text());
                        poscalc->setEndPos(domststst.firstChildElement("stop").text());
                        poscalc->setStepWidth(domststst.firstChildElement("stepwidth").text());
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
			QDomElement domPlugin = domElement.firstChildElement("plugin");
			getPluginData(domPlugin, paraHash);
			if (domPlugin.attribute("name").trimmed().length() > 1)
				poscalc->setStepPlugin(domPlugin.attribute("name"), paraHash);
			else
				sendError(ERROR,0,"Step Plugin: invalid name");
		}
		else
			sendError(ERROR,0,"No values found in XML to calculate motor positions");

		// order is important, since we must make sure all axes with step plugins
		// which might use reference axes must be called after their reference axes
		if (prependElement)
			axislist->prepend(new eveSMAxis(scanmodule, motor, axisDefinition, poscalc));
		else
			axislist->append(new eveSMAxis(scanmodule, motor, axisDefinition, poscalc));

		domElement = domElement.nextSiblingElement("smaxis");
	}
	}
	catch (std::exception& e)
	{
		// printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getAxisList").arg(e.what()));
	}
	// TODO uncomment
	// foreach (QString key, motorDefHash.keys()) delete axisDefHash.take(key);
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
	QHash<QString, eveSMDetector *> detectorHash;
        QStringList normalizeIdList;

	try
	{
		if (!smIdHash.contains(chain)) return channellist;
                QDomElement domSMElement = smIdHash.value(chain)->value(smid);
                QDomElement domElement = domSMElement.firstChildElement("smchannel");
                while (!domElement.isNull()) {
                    // first loop collects all normalizeIDs
                    QDomElement normId = domElement.firstChildElement("normalize_id");
                    if (!normId.isNull()) normalizeIdList.append(normId.text());
                    domElement = domElement.nextSiblingElement("smchannel");
                }

                domElement = domSMElement.firstChildElement("smchannel");
                while (!domElement.isNull()) {
			QHash<QString, QString> paraHash;
			eveSMChannel* nmChannel = NULL;
			eveSMDetector* detector = NULL;
			eveChannelDefinition* channelDefinition = NULL;
			QDomElement domId = domElement.firstChildElement("channelid");

                        if (normalizeIdList.contains(domId.text())){
                            // skip this channel if it is used as normalized channel elsewhere
                            domElement = domElement.nextSiblingElement("smchannel");
                            continue;
                        }

			channelDefinition = deviceList.getChannelDef(domId.text());
			if (channelDefinition == NULL){
				sendError(ERROR,0,QString("no channeldefinition found for %1").arg(domId.text()));
				domElement = domElement.nextSiblingElement("smchannel");
				continue;
			}

			eveDetectorDefinition* detectorDefinition = channelDefinition->getDetectorDefinition();
			if (detectorHash.contains(detectorDefinition->getId())){
				detector = detectorHash.value(detectorDefinition->getId());
			}
			else {
				detector = new eveSMDetector(scanmodule, detectorDefinition);
				detectorHash.insert(detectorDefinition->getId(), detector);
			}

			QList<eveEventProperty* > *eventList = new QList<eveEventProperty* >;
			domId = domElement.firstChildElement();
			while (!domId.isNull()){
				eveChannelDefinition* normalizeDefinition = NULL;
				eveSMDetector* normalizeDetector = NULL;
				if (domId.nodeName() == "redoevent"){
					eveEventProperty* event = getEvent(eveEventProperty::REDO, domId);
					if (event != NULL ) eventList->append(event);
				}
				else if (domId.nodeName() == "normalize_id"){
					// use a local copy of normalize definition because we modify it for this scanmodule
					normalizeDefinition = deviceList.getChannelDef(domId.text());
					if (normalizeDefinition == NULL) {
						sendError(ERROR,0,QString("no channeldefinition found for normalize channel %1").arg(domId.text()));
						domId = domId.nextSiblingElement();
						continue;
					}
					eveDetectorDefinition* detectorDefinition = normalizeDefinition->getDetectorDefinition();
					if (detectorHash.contains(detectorDefinition->getId())){
						normalizeDetector = detectorHash.value(detectorDefinition->getId());
					}
					else {
						normalizeDetector = new eveSMDetector(scanmodule, detectorDefinition);
						detectorHash.insert(detectorDefinition->getId(), normalizeDetector);
					}
					nmChannel = new eveSMChannel(scanmodule, normalizeDetector, normalizeDefinition, QHash<QString, QString>(), new QList<eveEventProperty* >, NULL);
				}
				else {
					paraHash.insert(domId.nodeName(), domId.text().trimmed());
				}
				domId = domId.nextSiblingElement();
			}

			channellist->append(new eveSMChannel(scanmodule, detector, channelDefinition, paraHash, eventList, nmChannel));
			domElement = domElement.nextSiblingElement("smchannel");
		}
	}
	catch (std::exception& e)
	{
		//printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getChannelList").arg(e.what()));
	}

	// TODO Before we can delete this we need a proper copy constructor for channelDefinition
	// foreach (QString key, channelDefHash.keys()) delete channelDefHash.take(key);
	return channellist;
}

QList<eveEventProperty*>* eveXMLReader::getSMEventList(int chain, int smid){

	if (!smIdHash.contains(chain)) return new QList<eveEventProperty* >;
	QDomElement domElement = smIdHash.value(chain)->value(smid);
	return getEventList(domElement);
}

QList<eveEventProperty*>* eveXMLReader::getChainEventList(int chain){

	if (!chainDomIdHash.contains(chain)) return new QList<eveEventProperty* >;
	QDomElement domElement = chainDomIdHash.value(chain);
	return getEventList(domElement);
}

QList<eveEventProperty*>* eveXMLReader::getEventList(QDomElement domElement){

	QList<eveEventProperty* > *eventList = new QList<eveEventProperty* >;

	try
	{
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
		domEvent = domElement.firstChildElement("stopevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::STOP, domEvent);
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("stopevent");
		}
		domEvent = domElement.firstChildElement("pauseevent");
		while (!domEvent.isNull()) {
			eveEventProperty* event = getEvent(eveEventProperty::PAUSE, domEvent);
			QDomElement direction = domEvent.firstChildElement("action");
			if ((event != NULL ) &&(!direction.isNull())){
				if (direction.text().toLower() == "onoff")
					event->setDirection(eveDirectionONOFF);
				else if (direction.text().toLower() == "on")
					event->setDirection(eveDirectionON);
				else if (direction.text().toLower() == "off")
					event->setDirection(eveDirectionOFF);
				else
					eveError::log(ERROR, QString("eveXMLReader::getSMEventList: pauseEvent invalid action: %1 ").arg(direction.text()));
				eveError::log(DEBUG, QString("eveXMLReader::getSMEventList: pauseEvent action: %1 ").arg(direction.text()));
			}
			if (event != NULL ) eventList->append(event);
			domEvent = domEvent.nextSiblingElement("pauseevent");
		}
	}
	catch (std::exception& e)
	{
		//printf("C++ Exception %s\n",e.what());
		sendError(FATAL,0,QString("C++ Exception %1 in eveXMLReader::getEventList").arg(e.what()));
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
 	else if (domElement.attribute("type") == "detector"){
		eventTypeT eventType = eveEventTypeDETECTOR;

		QDomElement domId = domElement.firstChildElement("id");
		if (domId.isNull()){
			sendError(ERROR, 0, "getEvent: need <id> tag for detector event");
			return NULL;
		}

		QString eventId = domId.text().trimmed();
		QRegExp regex = QRegExp("^D-(\\d+)-(\\d+)-([a-zA-Z0-9_:.;%-]+)$");
		if (!(eventId.contains(regex) && (regex.numCaptures() == 3))){
			sendError(ERROR, 0, QString("get Detector Event: invalid detector id: %1").arg(eventId));
			return NULL;
		}

		int chid=0, smid=0;
		bool cok, sok;
		sendError(ERROR, 0, QString("get Detector Event: all: %1 chid %2, smid: %3 det: %4").arg(regex.capturedTexts().at(0)).arg(regex.capturedTexts().at(1)).arg(regex.capturedTexts().at(2)).arg(regex.capturedTexts().at(3)));
		chid = regex.capturedTexts().at(1).toInt(&cok);
		smid = regex.capturedTexts().at(2).toInt(&sok);
		if (!cok || !sok){
			sendError(ERROR, 0, "getEvent: error extracting chainid or smid from detector event id");
			return NULL;
		}
		QString devname = regex.capturedTexts().at(3);
		eveDeviceDefinition* deviceDef = deviceList.getAnyDef(devname);
		if ((deviceDef == NULL) || (deviceDef->getValueCmd() == NULL)){
			sendError(ERROR, 0, QString("get Detector event: no or invalid device definition found for %1").arg(devname));
			return NULL;
		}
		// rebuild eventId to remove possible leading zeros
		eventId = QString("D-%1-%2-%3").arg(chid).arg(smid).arg(devname);
		sendError(DEBUG, 0, QString("found detector event definition for %1, id:%2, smid: %3, chid: %4").arg(devname).arg(eventId).arg(smid).arg(chid));
		return new eveEventProperty(eventId, devname, eveVariant(QVariant(eveVariant::getMangled(chid,smid))), eventType, eveIncidentNONE, action, NULL);
	}
	else if (domElement.attribute("type") == "monitor"){
	 	eventTypeT eventType = eveEventTypeMONITOR;

	 	QDomElement domId = domElement.firstChildElement("id");
		if (domId.isNull()){
			sendError(ERROR, 0, "getEvent: need <id> tag for monitor event");
			return NULL;
		}

	 	eveDeviceDefinition* deviceDef = deviceList.getAnyDef(domId.text());
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

QList<eveDeviceDefinition *>* eveXMLReader::getMonitorDeviceList(){

	QList<eveDeviceDefinition *>* monitors = new QList<eveDeviceDefinition *>();

	foreach(QString xmlid, monitorList){
		eveDeviceDefinition* deviceDef = deviceList.getAnyDef(xmlid);
		if ((deviceDef == NULL) || (deviceDef->getValueCmd() == NULL)){
            sendError(ERROR, 0, QString("MonitorList: no or invalid device definition found for %1").arg(xmlid));
		}
		else{
			monitors->append(deviceDef);
		}
	}
	return monitors;
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
                    if (math->getNormalizeExternal() != mathNext->getNormalizeExternal())
                        sendError(MINOR, 0, QString("SM id %1: yaxis/normalize_id/external inconsistency found, might show a slightly inaccurate plot %2").arg(mathNext->getFirstScanModuleId()).arg(math->getDetector()));
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
			bool init = true;
			QString xAxisId;
			QString yAxisId;
			QString normalizeId;
			int plotId = domPlot.attribute("id").toInt(&ok);
			domElement = domPlot.firstChildElement("init");
			if (!domElement.isNull()) {
				if ((domElement.text().compare("no", Qt::CaseInsensitive) == 0) || (domElement.text().compare("false", Qt::CaseInsensitive) == 0))
					init = false;
			}

			domElement = domPlot.firstChildElement("xaxis");
			if (!domElement.isNull()) domElement = domElement.firstChildElement("id");
			if (!domElement.isNull()) xAxisId = domElement.text();
			QDomElement domYAxis = domPlot.firstChildElement("yaxis");
			ok = (ok && (xAxisId.length() > 0));
			while (ok && !domYAxis.isNull()){
				normalizeId.clear();
				yAxisId.clear();
				domElement = domYAxis.firstChildElement("id");
				if (!domElement.isNull()) yAxisId = domElement.text();
				domElement = domYAxis.firstChildElement("normalize_id");
				if (!domElement.isNull()) normalizeId = domElement.text();
				if (yAxisId.length() > 0){
					sendError(DEBUG, 0, QString("MathConfig: %1, %2, %3, Plot: %4, init: %5").arg(xAxisId).arg(yAxisId).arg(normalizeId).arg(plotId).arg(init));
					eveMathConfig *mathConfig = new eveMathConfig(plotId, init, xAxisId);
					mathConfig->addScanModule(smid);
					mathConfig->addYAxis(yAxisId, normalizeId);
                    if ((normalizeId.length() > 0 ) && compareWithSMChannel(domElement, yAxisId, normalizeId)) mathConfig->setNormalizeExternal(true);
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

bool eveXMLReader:: compareWithSMChannel(QDomElement elem, QString yaxis, QString normalize){

    QDomElement smchannelElement = elem.firstChildElement("smchannel");
    while (!smchannelElement.isNull()) {
        QDomElement chidElem = smchannelElement.firstChildElement("channelid");
        QDomElement normidElem = smchannelElement.firstChildElement("normalize_id");

        if (!chidElem.isNull() && !normidElem.isNull() && (chidElem.text() == yaxis) &&
                    (normidElem.text() == normalize)) return true;
        smchannelElement = smchannelElement.nextSiblingElement("smchannel");
    }
    return false;
}
