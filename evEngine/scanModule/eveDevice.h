/*
 * eveDevice.h
 *
 *  Created on: 30.09.2008
 *      Author: eden
 */

#ifndef EVEDEVICE_H_
#define EVEDEVICE_H_

#include <QString>
#include <QList>
#include <QStringList>
#include "eveTypes.h"
// #include "eveSMDetector.h"

enum transMethodT {evePUT, eveGET, evePUTCB, eveGETCB, eveGETPUT, eveGETPUTCB, eveMONITOR};
enum eveTransportT {eveTRANS_CA, eveTRANS_LOCAL };
// enum eveEventTypeT {eveEventSCHEDULE, eveEventMONITOR};

class eveDetectorDefinition;
class eveMotorDefinition;
class eveSMDetector;
class eveSMMotor;

/**
 * \brief base class for transports (CA, etc.)
class eveBaseTransportDef {
public:
	eveBaseTransportDef(eveType);
	virtual ~eveBaseTransportDef();
	virtual eveBaseTransportDef* clone()=0;
	eveType getDataType(){return dataType;};
	eveTransportT getTransType(){return transType;};
protected:
	eveType dataType;
	eveTransportT transType;
};
 */

/**
 * \brief Common transport
 */
class eveTransportDef {
public:
	eveTransportDef(eveTransportT, eveType, transMethodT, double, QString );
	virtual ~eveTransportDef();
	eveTransportDef* clone();
	QString getName(){return accessName;};
	eveType getDataType(){return dataType;};
	eveTransportT getTransType(){return transType;};
	transMethodT getMethod(){return method;};
	double getTimeout(){return timeout;};

private:
	eveType dataType;
	eveTransportT transType;
	QString accessName;
	transMethodT method;
	double timeout;
};

///**
// * \brief EPICS CA transport
// */
//class eveCaTransportDef : public eveBaseTransportDef{
//public:
//	eveCaTransportDef(eveType, transMethodT, double, QString );
//	virtual ~eveCaTransportDef();
//	eveCaTransportDef* clone();
//	QString getName(){return pV;};
//	transMethodT getMethod(){return method;};
//	double getTimeout(){return timeout;};
//
//private:
//	QString pV;
//	transMethodT method;
//	double timeout;
//};
//
///**
// * \brief Local transport
// */
//class eveLocalTransportDef : public eveBaseTransportDef{
//public:
//	eveLocalTransportDef(eveType, transMethodT, QString );
//	virtual ~eveLocalTransportDef();
//	eveLocalTransportDef* clone();
//	QString getName(){return accessDescription;};
//	transMethodT getMethod(){return method;};
//
//private:
//	QString accessDescription;
//	transMethodT method;
//};

/**
 * \brief trigger or unit command
 *
 */
class eveDeviceCommand {
public:
	eveDeviceCommand(eveTransportDef *, QString, eveType);
	eveDeviceCommand(const eveDeviceCommand&);
	virtual ~eveDeviceCommand();
	eveDeviceCommand* clone();
	eveType getValueType() {return valueType;};
	QString getValueString() {return valueString;};
	eveTransportDef* getTrans() {return transDef;};

private:
	eveTransportDef *transDef;
	QString	valueString;
	eveType valueType;
};

/**
 * \brief base class for devices, detectors etc.
 */
class eveBaseDevice {
public:
	eveBaseDevice(QString, QString);
	virtual ~eveBaseDevice();
	virtual QString getName(){return devName;};
	virtual QString getId(){return devId;};

protected:
	QString devName;
	QString devId;
};

/**
 * \brief device or option
 */
class eveDevice : public eveBaseDevice {
public:
	eveDevice(eveDeviceCommand *, eveDeviceCommand *, QString, QString);
	virtual ~eveDevice();
	eveDeviceCommand * getValueCmd(){return valueCmd;};
	eveDeviceCommand * getUnitCmd(){return unit;};

protected:
	eveDeviceCommand * valueCmd;
	eveDeviceCommand * unit;

};

/**
 * \brief a simple detector or a detectorchannel
 */
class eveChannelDefinition : public eveDevice {
public:
	eveChannelDefinition(eveDetectorDefinition*, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, QString, QString);
	virtual ~eveChannelDefinition();
	eveType getChannelType(){return getValueCmd()->getTrans()->getDataType();};
	eveDeviceCommand * getTrigCmd(){return triggerCmd;};
	eveDeviceCommand * getStopCmd(){return stopCmd;};
	eveDetectorDefinition* getDetectorDefinition(){return detectorDefinition;};

protected:
	eveDetectorDefinition *parent; // the corresponding Detector if any (unused)
	eveDeviceCommand *triggerCmd;
	eveDeviceCommand *stopCmd;
	eveDetectorDefinition* detectorDefinition;
};

/**
 * \brief compound detector
 *
 */
class eveDetectorDefinition : public eveBaseDevice {
public:
	eveDetectorDefinition(QString, QString, eveDeviceCommand*, eveDeviceCommand*);
	virtual ~eveDetectorDefinition();
//	void addChannel(eveChannelDefinition*);
	eveSMDetector* getDetector() {return detector;};
	void setDetector(eveSMDetector* detec){detector = detec;};
	eveDeviceCommand * getTrigCmd(){return trigger;};
	eveDeviceCommand * getUnitCmd(){return unit;};

private:
//	QList<eveChannelDefinition*> channelList;
	eveDeviceCommand* trigger;
	eveDeviceCommand* unit;
	eveSMDetector* detector;
};

/**
 * \brief motor axis
 */
class eveAxisDefinition : public eveDevice {
public:
	eveAxisDefinition(eveMotorDefinition*, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *,eveDeviceCommand *, eveDeviceCommand *, QString, QString);
	virtual ~eveAxisDefinition();
	eveType getAxisType(){return getGotoCmd()->getTrans()->getDataType();};
	eveDeviceCommand * getTrigCmd(){return triggerCmd;};
	eveDeviceCommand * getGotoCmd(){return gotoCmd;};
	eveDeviceCommand * getStopCmd(){return stopCmd;};
	eveDeviceCommand * getStatusCmd(){return axisStatusCmd;};
	eveDeviceCommand * getPosCmd(){return valueCmd;};
	eveDeviceCommand * getDeadbandCmd(){return deadbandCmd;};
	eveMotorDefinition* getMotorDefinition(){return motorDefinition;};

private:
	eveDeviceCommand  *deadbandCmd;
	eveDeviceCommand *triggerCmd;
	eveDeviceCommand  *gotoCmd;
	eveDeviceCommand *stopCmd;
	eveDeviceCommand  *axisStatusCmd;
	eveMotorDefinition* motorDefinition;

};

/**
 * \brief compound motor
 *
 */
class eveMotorDefinition : public eveBaseDevice {
public:
	eveMotorDefinition(QString, QString, eveDeviceCommand*, eveDeviceCommand*);
	virtual ~eveMotorDefinition();
	eveSMMotor* getMotor() {return motor;};
	void setMotor(eveSMMotor* smmotor){motor = smmotor;};
	eveDeviceCommand * getTrigCmd(){return trigger;};
	eveDeviceCommand * getUnitCmd(){return unit;};

private:
	eveDeviceCommand* trigger;
	eveDeviceCommand* unit;
	eveSMMotor* motor;
};


#endif /* EVEDEVICE_H_ */
