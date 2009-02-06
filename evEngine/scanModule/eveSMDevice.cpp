/*
 * eveSMDevice.cpp
 *
 *  Created on: 04.02.2009
 *      Author: eden
 */

#include "eveSMDevice.h"

eveSMDevice::eveSMDevice(eveDevice* definition, eveVariant writevalue, bool resetPrevious) {

	setPrevious = resetPrevious;
	writeVal=writevalue;
}

eveSMDevice::~eveSMDevice() {
	// TODO Auto-generated destructor stub
}
