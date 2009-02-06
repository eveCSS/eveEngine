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
	printf("deleting devices\n");
	foreach (eveDevice* device, deviceDefinitions) delete device;
	printf("deleting channels\n");
	foreach (eveSimpleDetector* channel, channelDefinitions) delete channel;
	printf("deleting axes\n");
	foreach (eveMotorAxis* axis, axisDefinitions) {
//		printf("deleting first axis %x\n", (int)axis);
		delete axis;
	}
	printf("deleting done\n");
}

void eveDeviceList::insert(QString ident, eveMotorAxis* axis){
	axisDefinitions.insert(ident, axis);
}

void eveDeviceList::insert(QString ident, eveSimpleDetector* channel ){
	channelDefinitions.insert(ident, channel);
}

void eveDeviceList::insert(QString ident, eveDevice* device){
	deviceDefinitions.insert(ident, device);
}
