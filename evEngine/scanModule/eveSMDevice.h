/*
 * eveSMDevice.h
 *
 *  Created on: 04.02.2009
 *      Author: eden
 */

#ifndef EVESMDEVICE_H_
#define EVESMDEVICE_H_

#include "eveDevice.h"
#include "eveVariant.h"

class eveSMDevice {
public:
	eveSMDevice(eveDevice*, eveVariant, bool reset=false);
	virtual ~eveSMDevice();

private:
	eveVariant writeVal;
	eveVariant readVal;
	bool setPrevious;
};

#endif /* EVESMDEVICE_H_ */
