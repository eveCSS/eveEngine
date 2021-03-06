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
#include "eveDeviceDefinitions.h"
#include "eveManager.h"
#include "eveDeviceList.h"
#include "eveSMDevice.h"
#include "eveSMAxis.h"
#include "eveSMChannel.h"
#include "eveEventProperty.h"
#include "eveMathConfig.h"

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
	bool read(QByteArray);
	int getRootId(int);
	QList<int> getChainIdList(){return chainIdList;};
	int getNested(int, int);
	int getAppended(int, int);
	int getRepeatCount(){return repeatCount;};
	QList<eveSMDevice*>* getPreScanList(eveScanModule*, int, int);
	QList<eveSMDevice*>* getPostScanList(eveScanModule*, int, int);
	QList<eveSMAxis*>* getAxisList(eveScanModule*, int, int);
	QList<eveSMChannel*>* getChannelList(eveScanModule*, int, int);
	QList<eveEventProperty*>* getSMEventList(int, int);
	QList<eveEventProperty*>* getChainEventList(int);
	QString getChainString(int, QString);
	QString getSMTag(int, int, QString);
	bool getSMTagBool(int, int, QString, bool);
	int getSMTagInteger(int, int, QString, int);
	double getSMTagDouble(int, int, QString, double);
	QHash<QString, QString> getChainPlugin(int, QString);
	QList<QHash<QString, QString>* >* getPositionerPluginList(int, int);
	QList<eveMathConfig*>* getFilteredMathConfigs(int);
	QList<eveDeviceDefinition *>* getMonitorDeviceList();
	bool isNoSave(int id){return noSaveCidList.contains(id);};

private:
	void getMathConfigFromPlot(int, int, QList<eveMathConfig*>*);
	void sendError(int, int,  QString);
	void createDetectorDefinition(QDomNode);
	void createMotorDefinition(QDomNode);
	void createDeviceDefinition(QDomElement);
	int getIntValueOfTag(int, int, QString);
	int repeatCount;
	QList<int> noSaveCidList;
	QList<int> chainIdList;
	QList<eveSMDevice*>* getSMDeviceList(eveScanModule*, int, int, QString);
	eveEventProperty* getEvent(eveEventProperty::actionTypeT, QDomElement);
    QDomDocument *domDocument;
	eveCommandDefinition * createDeviceCommand(QDomNode);
	eveChannelDefinition * createChannelDefinition(QDomNode, eveDetectorDefinition *);
	eveAxisDefinition * createAxisDefinition(QDomNode, eveMotorDefinition *);
	void createOption(QDomNode);
	eveTransportDefinition* createTransportDefinition(QDomElement node);
	void getPluginData(QDomElement, QHash<QString, QString>&);
    bool compareWithSMChannel(QDomElement, QString, QString);
	eveDeviceList deviceList;
	eveManager *parent;
	QHash<int, QDomElement> chainDomIdHash;
	QHash<int, QHash<int, QDomElement>* > smIdHash;
	QHash<int, int> rootSMHash; // has id of the root sm in chain
	QList<eveEventProperty*>* getEventList(QDomElement);
	//eveEventTypeT getEventType(QString);
	QStringList monitorList;

};

#endif /* EVEXMLREADER_H_ */
