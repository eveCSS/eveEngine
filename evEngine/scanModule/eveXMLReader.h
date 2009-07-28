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

class eveScanModule;

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

private:
	void sendError(int, int,  QString);
	void createDetector(QDomNode);
	void createMotor(QDomNode);
	void createDevice(QDomNode);
	int getIntValueOfTag(int, int, QString);
	QList<eveSMDevice*>* getSMDeviceList(eveScanModule*, int, int, QString);
    QDomDocument *domDocument;
	eveDeviceCommand * createDeviceCommand(QDomNode);
	eveDetectorChannel * createChannel(QDomNode, eveDeviceCommand *, eveDeviceCommand *);
	eveMotorAxis * createAxis(QDomNode, eveDeviceCommand *, eveDeviceCommand *);
	void createOption(QDomNode);
	eveTransportDef* createTransport(QDomElement node);
	eveDeviceList *deviceList;
	eveManager *parent;
	QHash<int, QDomElement> chainDomIdHash;
	QHash<int, QHash<int, QDomElement>* > smIdHash;
	QHash<int, int> rootSMHash; // has id of the root sm in chain
	//QHash<int, eveScanManager*> scanManagerHash;
	// for now we don't care about detectors and motors
	// we are interested in channels and axes only
	//QHash<QString, eveDetector*> detectorDefinitions;
	//QHash<QString, eveMotor*> motorDefinitions;

};

#endif /* EVEXMLREADER_H_ */
