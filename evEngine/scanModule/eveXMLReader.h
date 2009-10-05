/*
 * eveXMLReader.h
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#ifndef EVEXMLREADER_H_
#define EVEXMLREADER_H_

#include <QDomNode>
#include <QDomElement>
#include <QHash>
#include <QByteArray>
#include "eveDevice.h"
#include "eveManager.h"
#include "eveDeviceList.h"
#include "eveSMDevice.h"
#include "eveSMAxis.h"
#include "eveSMChannel.h"
#include "eveEventProperty.h"

class eveScanModule;
class eveScanManager;

/**
 * \brief reads XML and creates all indices, motors, detectors, devices
 *
 * motor, detector, device -lists must be useable, after destruction
 *
 */
class eveXMLReader {

public:
	eveXMLReader(eveManager*);
	virtual ~eveXMLReader();
	bool read(QByteArray, eveDeviceList *);
	int getRootId(int);
	QHash<int, QDomElement> getChainIdHash(){return chainDomIdHash;};
	int getNested(int, int);
	int getAppended(int, int);
	QList<eveSMDevice*>* getPreScanList(eveScanModule*, int, int);
	QList<eveSMDevice*>* getPostScanList(eveScanModule*, int, int);
	QList<eveSMAxis*>* getAxisList(eveScanModule*, int, int);
	QList<eveSMChannel*>* getChannelList(eveScanModule*, int, int);
	QList<eveEventProperty*>* getEventList(eveScanManager*, int, int);

	QString getChainString(int, QString);

private:
	void sendError(int, int,  QString);
	void createDetectorDefinition(QDomNode);
	void createMotorDefinition(QDomNode);
	void createDeviceDefinition(QDomNode);
	void createEventDefinition(QDomNode);
	int getIntValueOfTag(int, int, QString);
	QList<eveSMDevice*>* getSMDeviceList(eveScanModule*, int, int, QString);
	eveEventProperty* getEvent(eveScanManager*, QDomElement);
    QDomDocument *domDocument;
	eveDeviceCommand * createDeviceCommand(QDomNode);
	eveDetectorChannel * createChannelDefinition(QDomNode, eveDeviceCommand *, eveDeviceCommand *);
	eveMotorAxis * createAxisDefinition(QDomNode, eveDeviceCommand *, eveDeviceCommand *);
	void createOption(QDomNode);
	eveTransportDef* createTransportDefinition(QDomElement node);
	eveDeviceList *deviceList;
	eveManager *parent;
	QHash<int, QDomElement> chainDomIdHash;
	QHash<int, QHash<int, QDomElement>* > smIdHash;
	QHash<int, int> rootSMHash; // has id of the root sm in chain
	eveEventTypeT getEventType(QString);

};

#endif /* EVEXMLREADER_H_ */
