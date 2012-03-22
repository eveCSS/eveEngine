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
		foreach (eveDevice* device, deviceDefinitions) delete device;
		deviceDefinitions.clear();
		foreach (eveDetectorChannel* channel, channelDefinitions) delete channel;
		channelDefinitions.clear();
		foreach (eveMotorAxis* axis, axisDefinitions) delete axis;
		axisDefinitions.clear();
	}
	catch (std::exception& e)
	{
		eveError::log(DEBUG, QString("C++ Exception in eveDeviceList::clearAll %1").arg(e.what()));
	}

}

eveDevice* eveDeviceList::getAnyDef(QString name){

	if (axisDefinitions.contains(name)) return getAxisDef(name);
	if (channelDefinitions.contains(name)) return getChannelDef(name);
	if (deviceDefinitions.contains(name)) return getDeviceDef(name);
	return NULL;
}

