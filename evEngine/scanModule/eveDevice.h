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

/**
 * \brief base class for transports (CA, etc.)
 */
class eveTransport {
public:
	eveTransport(eveType);
	virtual ~eveTransport();
	virtual eveTransport* clone()=0;

protected:
	eveType dataType;
};

/**
 * \brief EPICS CA transport
 */
class eveCaTransport : public eveTransport{
public:
	eveCaTransport(eveType, pvMethodT, QString );
	virtual ~eveCaTransport();
	eveCaTransport* clone();

protected:
	QString pV;
	pvMethodT method;
};

/** \brief trigger or unit command
 *
 */
class eveDeviceCommand {
public:
	eveDeviceCommand(eveTransport *, QString, eveType);
	virtual ~eveDeviceCommand();
	eveDeviceCommand* clone();
	eveType getDeviceType() {return devType;};

private:
	eveTransport *devCmd;
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
	eveDevice(eveDeviceCommand *, eveTransport *, QString, QString);
	virtual ~eveDevice();

protected:
	eveTransport * valueCmd;
	eveDeviceCommand * unit;

};

/**
 * \brief a simple detector or a detectorchannel
 */
class eveSimpleDetector : public eveDevice {
public:
	eveSimpleDetector(eveDeviceCommand *, eveDeviceCommand *, eveTransport *, QString, QString);
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
	eveMotorAxis(eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, eveDeviceCommand *, eveTransport *,eveTransport *, QString, QString);
	virtual ~eveMotorAxis();
	eveType getAxisType(){return gotoCmd->getDeviceType();};
private:
	eveDeviceCommand *triggerCmd;
	eveDeviceCommand *gotoCmd;
	eveDeviceCommand *stopCmd;
	eveTransport *axisStatus;
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
