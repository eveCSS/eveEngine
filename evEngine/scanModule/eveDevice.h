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
	eveCaTransportDef(eveType, pvMethodT, QString );
	virtual ~eveCaTransportDef();
	eveCaTransportDef* clone();
	QString getName(){return pV;};
	pvMethodT getMethod(){return method;};

private:
	QString pV;
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
	eveType getDeviceType() {return devType;};
	eveTransportDef* getTrans() {return devCmd;};

private:
	eveTransportDef *devCmd;
	QString	devString;
	eveType devType;
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
	eveDevice(eveDeviceCommand *, eveTransportDef *, QString, QString);
	virtual ~eveDevice();
	eveDeviceCommand * getUnitCmd(){return unit;};

protected:
	eveTransportDef * valueCmd;
	eveDeviceCommand * unit;

};

/**
 * \brief a simple detector or a detectorchannel
 */
class eveSimpleDetector : public eveDevice {
public:
	eveSimpleDetector(eveDeviceCommand *, eveDeviceCommand *, eveTransportDef *, QString, QString);
	virtual ~eveSimpleDetector();

protected:
	//eveDetector *parent; // the corresponding Detector if any (unused)
	eveDeviceCommand * triggerCmd;

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
	void addChannel(eveSimpleDetector*);

private:
	QList<eveSimpleDetector*> channelList;
};

/**
 * \brief motor axis
 */
class eveMotorAxis : public eveDevice {
public:
	eveMotorAxis(eveDeviceCommand *, eveDeviceCommand *, eveTransportDef *, eveDeviceCommand *, eveTransportDef *,eveTransportDef *, eveTransportDef *, QString, QString);
	virtual ~eveMotorAxis();
	eveType getAxisType(){return getGotoCmd()->getDataType();};
	eveDeviceCommand * getTrigCmd(){return triggerCmd;};
	eveTransportDef * getGotoCmd(){return gotoCmd;};
	eveDeviceCommand * getStopCmd(){return stopCmd;};
	eveTransportDef * getStatusCmd(){return axisStatusCmd;};
	eveTransportDef * getPosCmd(){return valueCmd;};
	eveTransportDef * getDeadbandCmd(){return deadbandCmd;};

private:
	eveTransportDef  *deadbandCmd;
	eveDeviceCommand *triggerCmd;
	eveTransportDef  *gotoCmd;
	eveDeviceCommand *stopCmd;
	eveTransportDef  *axisStatusCmd;
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
