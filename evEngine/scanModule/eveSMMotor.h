/*
 * eveSMMotor.h
 *
 *  Created on: 18.06.2012
 *      Author: eden
 */

#ifndef EVESMMOTOR_H_
#define EVESMMOTOR_H_

#include "eveSMBaseDevice.h"
#include "eveBaseTransport.h"
#include "eveDeviceDefinitions.h"
#include "eveVariant.h"
#include "eveScanModule.h"

class eveSMMotor: public eveSMBaseDevice {
public:
	eveSMMotor(eveScanModule*, eveMotorDefinition*);
	virtual ~eveSMMotor();
	eveBaseTransport* getTrigTrans(){return triggerTrans;};
	eveBaseTransport* getUnitTrans(){return unitTrans;};
	eveVariant getTrigValue(){return triggerValue;};
	QString getUnitString(){return unitString;};
	void sendError(int, int, int, QString);

private:
	void sendError(int, int, QString);
	eveScanModule* scanModule;
	eveBaseTransport* triggerTrans;
	eveBaseTransport* unitTrans;
	QString unitString;
	eveVariant triggerValue;
};

#endif /* EVESMMOTOR_H_ */
