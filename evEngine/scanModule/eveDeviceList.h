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
	void insert(QString ident, eveMotorAxis* axis){axisDefinitions.insert(ident, axis);};
	void insert(QString ident, eveDetectorChannel* channel ){channelDefinitions.insert(ident, channel);};
	void insert(QString ident, eveDevice* device){deviceDefinitions.insert(ident, device);};
//	void insert(QString ident, eveEventDefinition* event){eventDefinitions.insert(ident, event);};
	eveDevice* getAnyDef(QString name);
	eveDevice* getDeviceDef(QString name){return deviceDefinitions.value(name, NULL);};
	eveDetectorChannel* getChannelDef(QString name){return channelDefinitions.value(name, NULL);};
	eveMotorAxis* getAxisDef(QString name){return axisDefinitions.value(name, NULL);};
//	eveEventDefinition* getEventDef(QString name){return eventDefinitions.value(name, NULL);};
	void clearAll();

private:
	QHash<QString, eveDevice*> deviceDefinitions;
	QHash<QString, eveDetectorChannel*> channelDefinitions;
	QHash<QString, eveMotorAxis*> axisDefinitions;
//	QHash<QString, eveEventDefinition*> eventDefinitions;

};

#endif /* EVEDEVICELIST_H_ */
