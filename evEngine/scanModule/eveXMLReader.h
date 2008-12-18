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

public:
//	double getDVal(QString, int, int, int);
//	int getIVal(QString, int, int, int);
//	epqMotor* getMotor(int, int, int);
//	epqBaseMotSched* getMotSched(QDomElement);
//	epqDetector* getDetector(int, int, int);
//	epqCommand* getPreScan(int, int, int);
//	QString getString(QString, int, int, int);
//	QHash<QString, QString> * getIdTable();
	QHash<int, QDomElement> getChainIdHash(){return chainDomIdHash;};

private:
	void sendError(int, int,  QString);
	void createDetector(QDomNode);
	void createMotor(QDomNode);
	void createDevice(QDomNode);
    QDomDocument *domDocument;
	eveDeviceCommand * createDeviceCommand(QDomNode);
	eveSimpleDetector * createChannel(QDomNode, eveDeviceCommand *, eveDeviceCommand *);
	eveMotorAxis * createAxis(QDomNode, eveDeviceCommand *, eveDeviceCommand *);
	void createOption(QDomNode);
	eveCaTransport* createCaTransport(QDomElement node);
	eveDeviceList *deviceList;
	eveManager *parent;
	QHash<int, QDomElement> chainDomIdHash;
	// for now we don't care about detectors and motors
	// we are interested in channels and axes only
	//QHash<QString, eveDetector*> detectorDefinitions;
	//QHash<QString, eveMotor*> motorDefinitions;

};

#endif /* EVEXMLREADER_H_ */
