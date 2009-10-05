/*
 * eveDeviceList.cpp
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#include "eveDeviceList.h"

eveDeviceList::eveDeviceList() {

}

eveDeviceList::~eveDeviceList() {
	clearAll();
}

void eveDeviceList::clearAll() {

	foreach (eveDevice* device, deviceDefinitions) delete device;
	foreach (eveDetectorChannel* channel, channelDefinitions) delete channel;
	foreach (eveMotorAxis* axis, axisDefinitions) delete axis;
	foreach (eveEventDefinition* event, eventDefinitions) delete event;
}



