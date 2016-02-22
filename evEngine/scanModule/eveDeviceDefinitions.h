/*
 * eveDeviceDefinition.h
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#ifndef EVEDEVICE_H_
#define EVEDEVICE_H_

#include <QString>
#include <QHash>
#include <QList>
#include <QStringList>
#include "eveTypes.h"

enum transMethodT {evePUT, eveGET, evePUTCB, eveGETCB, eveGETPUT, eveGETPUTCB, eveMONITOR};
enum eveTransportT {eveTRANS_CA, eveTRANS_LOCAL };

class eveDetectorDefinition;
class eveMotorDefinition;

/**
 * \brief Common transport definition
 */
class eveTransportDefinition {
public:
    eveTransportDefinition(eveTransportT, eveType, transMethodT, double, QString);
    eveTransportDefinition(eveTransportT, eveType, transMethodT, double, QString, QHash<QString, QString>);
    virtual ~eveTransportDefinition();
	eveTransportDefinition* clone();
	QString getName(){return accessName;};
	eveType getDataType(){return dataType;};
	eveTransportT getTransType(){return transType;};
	transMethodT getMethod(){return method;};
    QHash<QString, QString> getAttributes(){return attributes;};
	double getTimeout(){return timeout;};

private:
	eveType dataType;
	eveTransportT transType;
	QString accessName;
	transMethodT method;
    QHash<QString, QString> attributes;
	double timeout;
};

/**
 * \brief trigger or unit command
 *
 */
class eveCommandDefinition {
public:
	eveCommandDefinition(eveTransportDefinition *, QString, eveType);
	eveCommandDefinition(const eveCommandDefinition&);
	virtual ~eveCommandDefinition();
	eveCommandDefinition* clone();
	eveType getValueType() {return valueType;};
	QString getValueString() {return valueString;};
	eveTransportDefinition* getTrans() {return transDef;};

private:
	eveTransportDefinition *transDef;
	QString	valueString;
	eveType valueType;
};

/**
 * \brief base class for all devices
 */
class eveBaseDeviceDefinition {
public:
	eveBaseDeviceDefinition(QString, QString);
	virtual ~eveBaseDeviceDefinition();
	virtual QString getName(){return devName;};
	virtual QString getId(){return devId;};

protected:
	QString devName;
	QString devId;
};

/**
 * \brief devices or options (xml) use this
 */
class eveDeviceDefinition : public eveBaseDeviceDefinition {
public:
	eveDeviceDefinition(eveCommandDefinition *, eveCommandDefinition *, QString, QString);
	virtual ~eveDeviceDefinition();
	eveCommandDefinition * getValueCmd(){return valueCmd;};
	eveCommandDefinition * getUnitCmd(){return unit;};

protected:
	eveCommandDefinition * valueCmd;
	eveCommandDefinition * unit;

};

/**
 * \brief detectorchannel definition
 */
class eveChannelDefinition : public eveDeviceDefinition {
public:
	eveChannelDefinition(eveDetectorDefinition*, eveCommandDefinition *, eveCommandDefinition *, eveCommandDefinition *, eveCommandDefinition *, QString, QString);
	virtual ~eveChannelDefinition();
	eveType getChannelType(){return getValueCmd()->getTrans()->getDataType();};
	eveCommandDefinition * getTrigCmd(){return triggerCmd;};
	eveCommandDefinition * getStopCmd(){return stopCmd;};
	eveDetectorDefinition* getDetectorDefinition(){return detectorDefinition;};

protected:
	eveDetectorDefinition *parent; // the corresponding Detector if any (unused)
	eveCommandDefinition *triggerCmd;
	eveCommandDefinition *stopCmd;
	eveDetectorDefinition* detectorDefinition;
};

/**
 * \brief detector definition
 *
 */
class eveDetectorDefinition : public eveBaseDeviceDefinition {
public:
	eveDetectorDefinition(QString, QString, eveCommandDefinition*, eveCommandDefinition*, eveCommandDefinition*);
	virtual ~eveDetectorDefinition();
	eveCommandDefinition * getTrigCmd(){return trigger;};
	eveCommandDefinition * getUnitCmd(){return unit;};
	eveCommandDefinition * getStopCmd(){return stop;};

private:
	eveCommandDefinition* trigger;
	eveCommandDefinition* unit;
	eveCommandDefinition* stop;
};

/**
 * \brief motoraxis definition
 */
class eveAxisDefinition : public eveDeviceDefinition {
public:
	eveAxisDefinition(eveMotorDefinition*, eveCommandDefinition *, eveCommandDefinition *, eveCommandDefinition *, eveCommandDefinition *, eveCommandDefinition *,eveCommandDefinition *, eveCommandDefinition *, QString, QString);
	virtual ~eveAxisDefinition();
	eveType getAxisType(){return getGotoCmd()->getTrans()->getDataType();};
	eveCommandDefinition * getTrigCmd(){return triggerCmd;};
	eveCommandDefinition * getGotoCmd(){return gotoCmd;};
	eveCommandDefinition * getStopCmd(){return stopCmd;};
	eveCommandDefinition * getStatusCmd(){return axisStatusCmd;};
	eveCommandDefinition * getPosCmd(){return valueCmd;};
	eveCommandDefinition * getDeadbandCmd(){return deadbandCmd;};
	eveMotorDefinition* getMotorDefinition(){return motorDefinition;};
	void setMotorDefinition(eveMotorDefinition* motorDef){motorDefinition = motorDef;};

private:
	eveCommandDefinition  *deadbandCmd;
	eveCommandDefinition *triggerCmd;
	eveCommandDefinition  *gotoCmd;
	eveCommandDefinition *stopCmd;
	eveCommandDefinition  *axisStatusCmd;
	eveMotorDefinition* motorDefinition;

};

/**
 * \brief motor definition
 *
 */
class eveMotorDefinition : public eveBaseDeviceDefinition {
public:
	eveMotorDefinition(QString, QString, eveCommandDefinition*, eveCommandDefinition*);
	virtual ~eveMotorDefinition();
	eveCommandDefinition * getTrigCmd(){return trigger;};
	eveCommandDefinition * getUnitCmd(){return unit;};

private:
	eveCommandDefinition* trigger;
	eveCommandDefinition* unit;
};


#endif /* EVEDEVICE_H_ */
