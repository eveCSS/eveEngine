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

enum pvMethodT {evePUT, eveGET, evePUTCB, eveGETCB, eveGETPUT, eveGETPUTCB};
enum eveTransportT {eveTRANS_CA, eveTRANS_LOCAL };

/**
 * \brief base class for transports (CA, etc.)
 */
class eveTransportDef {
public:
	eveTransportDef(eveType);
	virtual ~eveTransportDef();
	virtual eveTransportDef* clone()=0;
	eveType getDataType(){return dataType;};
	eveTransportT getTransType(){return transtype;};
protected:
	eveType dataType;
	eveTransportT transtype;
};

/**
 * \brief EPICS CA transport
 */
class eveCaTransportDef : public eveTransportDef{
public:
	eveCaTransportDef(eveType, pvMethodT, double, QString );
	virtual ~eveCaTransportDef();
	eveCaTransportDef* clone();
	QString getName(){return pV;};
	pvMethodT getMethod(){return method;};
	double getTimeout(){return timeout;};

private:
	QString pV;
	pvMethodT method;
	double timeout;
};

/**
 * \brief Local transport
 */
class eveLocalTransportDef : public eveTransportDef{
public:
	eveLocalTransportDef(eveType, pvMethodT, QString );
	virtual ~eveLocalTransportDef();
	eveLocalTransportDef* clone();
	QString getName(){return accessDescription;};
	pvMethodT getMethod(){return method;};

private:
	QString accessDescription;
	pvMethodT method;
};

/**
 * \brief trigger or unit command
 *
 */
class eveDeviceCommand {
public:
	eveDeviceCommand(eveTransportDef *, QString, eveType);
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
class eveDetectorChannel : public eveDevice {
public:
	eveDetectorChannel(eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, QString, QString);
	virtual ~eveDetectorChannel();
	eveType getChannelType(){return getValueCmd()->getTrans()->getDataType();};
	eveDeviceCommand * getTrigCmd(){return triggerCmd;};
	eveDeviceCommand * getStopCmd(){return stopCmd;};

protected:
	//eveDetector *parent; // the corresponding Detector if any (unused)
	eveDeviceCommand *triggerCmd;
	eveDeviceCommand *stopCmd;

};

/**
 * \brief compound detector
 *
 * detector is not actually used, it keeps a list of channels
 */
class eveDetector : public eveBaseDevice {
public:
	eveDetector(QString, QString);
	virtual ~eveDetector();
	void addChannel(eveDetectorChannel*);

private:
	QList<eveDetectorChannel*> channelList;
};

/**
 * \brief motor axis
 */
class eveMotorAxis : public eveDevice {
public:
	eveMotorAxis(eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *,eveDeviceCommand *, eveDeviceCommand *, QString, QString);
	virtual ~eveMotorAxis();
	eveType getAxisType(){return getGotoCmd()->getTrans()->getDataType();};
	eveDeviceCommand * getTrigCmd(){return triggerCmd;};
	eveDeviceCommand * getGotoCmd(){return gotoCmd;};
	eveDeviceCommand * getStopCmd(){return stopCmd;};
	eveDeviceCommand * getStatusCmd(){return axisStatusCmd;};
	eveDeviceCommand * getPosCmd(){return valueCmd;};
	eveDeviceCommand * getDeadbandCmd(){return deadbandCmd;};

private:
	eveDeviceCommand  *deadbandCmd;
	eveDeviceCommand *triggerCmd;
	eveDeviceCommand  *gotoCmd;
	eveDeviceCommand *stopCmd;
	eveDeviceCommand  *axisStatusCmd;
	//eveMotor *parentMotor;	// the corresponding motor (unused)

};

/**
 * \brief compound motor
 *
 * motor is not actually used, it holds a list of axes
 */
class eveMotor : public eveBaseDevice {
public:
	eveMotor(QString, QString);
	virtual ~eveMotor();
private:
	QList<eveMotorAxis*> axesList;
};


#endif /* EVEDEVICE_H_ */
