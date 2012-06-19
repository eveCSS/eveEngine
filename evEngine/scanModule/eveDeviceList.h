/*
 * eveDeviceList.h
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#ifndef EVEDEVICELIST_H_
#define EVEDEVICELIST_H_

#include <QHash>
#include "eveDeviceDefinitions.h"

class eveDeviceList {
public:
	eveDeviceList();
	virtual ~eveDeviceList();
	void insert(QString ident, eveAxisDefinition* axis){axisDefinitions.insert(ident, axis);};
	void insert(QString ident, eveChannelDefinition* channel ){channelDefinitions.insert(ident, channel);};
	void insert(QString ident, eveDeviceDefinition* device){deviceDefinitions.insert(ident, device);};
	void insert(QString ident, eveBaseDeviceDefinition* device){baseDeviceDefinitions.insert(ident, device);};
	eveDeviceDefinition* getAnyDef(QString name);
	eveDeviceDefinition* getDeviceDef(QString name){return deviceDefinitions.value(name, NULL);};
	eveChannelDefinition* getChannelDef(QString name){return channelDefinitions.value(name, NULL);};
	eveAxisDefinition* getAxisDef(QString name){return axisDefinitions.value(name, NULL);};
	void clearAll();

private:
	QHash<QString, eveDeviceDefinition*> deviceDefinitions;
	QHash<QString, eveChannelDefinition*> channelDefinitions;
	QHash<QString, eveAxisDefinition*> axisDefinitions;
	QHash<QString, eveBaseDeviceDefinition*> baseDeviceDefinitions;

};

#endif /* EVEDEVICELIST_H_ */
