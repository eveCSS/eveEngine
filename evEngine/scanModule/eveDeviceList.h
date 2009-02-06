/*
 * eveDeviceList.h
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#ifndef EVEDEVICELIST_H_
#define EVEDEVICELIST_H_

#include <QHash>
#include "eveDevice.h"

class eveDeviceList {
public:
	eveDeviceList();
	virtual ~eveDeviceList();
	void insert(QString, eveMotorAxis*);
	void insert(QString, eveSimpleDetector*);
	void insert(QString, eveDevice*);
	eveDevice* getDeviceDef(QString name){return deviceDefinitions.value(name, NULL);};
	eveSimpleDetector* getDetectorDef(QString name){return channelDefinitions.value(name, NULL);};
	eveMotorAxis* getAxisDef(QString name){return axisDefinitions.value(name, NULL);};
	void clearAll();

private:
	QHash<QString, eveDevice*> deviceDefinitions;
	QHash<QString, eveSimpleDetector*> channelDefinitions;
	QHash<QString, eveMotorAxis*> axisDefinitions;

};

#endif /* EVEDEVICELIST_H_ */
