/*
 * eveDeviceList.cpp
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#include <exception>
#include "eveDeviceList.h"
#include "eveMessage.h"
#include "eveError.h"

eveDeviceList::eveDeviceList() {

}

eveDeviceList::~eveDeviceList() {
	clearAll();
}

void eveDeviceList::clearAll() {

	try
	{
		foreach (eveDeviceDefinition* device, deviceDefinitions) delete device;
		deviceDefinitions.clear();
		foreach (eveChannelDefinition* channel, channelDefinitions) delete channel;
		channelDefinitions.clear();
		foreach (eveAxisDefinition* axis, axisDefinitions) delete axis;
		axisDefinitions.clear();
		foreach (eveBaseDeviceDefinition* basedevice, baseDeviceDefinitions) delete basedevice;
		baseDeviceDefinitions.clear();
	}
	catch (std::exception& e)
	{
		eveError::log(DEBUG, QString("C++ Exception in eveDeviceList::clearAll %1").arg(e.what()));
	}

}

eveDeviceDefinition* eveDeviceList::getAnyDef(QString name){

	if (axisDefinitions.contains(name)) return getAxisDef(name);
	if (channelDefinitions.contains(name)) return getChannelDef(name);
	if (deviceDefinitions.contains(name)) return getDeviceDef(name);
	return NULL;
}

