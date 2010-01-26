/*
 * eveDeviceList.cpp
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#include <exception>
#include "eveDeviceList.h"

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
		printf("C++ Exception in eveDeviceList::clearAll %s\n",e.what());
	}

}



