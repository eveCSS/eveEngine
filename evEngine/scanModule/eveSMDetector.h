/*
 * eveSMDetector.h
 *
 *  Created on: 15.06.2012
 *      Author: eden
 */

#ifndef EVESMDETECTOR_H_
#define EVESMDETECTOR_H_

#include <QString>
#include "eveBaseTransport.h"
#include "eveDeviceDefinitions.h"
#include "eveVariant.h"
#include "eveScanModule.h"

class eveSMDetector: public eveSMBaseDevice {
public:
	eveSMDetector(eveScanModule*, eveDetectorDefinition*);
	virtual ~eveSMDetector();
	eveBaseTransport* getTrigTrans(){return triggerTrans;};
	eveBaseTransport* getStopTrans(){return stopTrans;};
	eveBaseTransport* getUnitTrans(){return unitTrans;};
	double getTriggerTimeout() {return triggerTimeout;};
	eveVariant getTrigValue(){return triggerValue;};
	eveVariant getStopValue(){return stopValue;};
	QString getUnitString(){return unitString;};
	void sendError(int, int, int, QString);

private:
	void sendError(int, int, QString);
	eveScanModule* scanModule;
	eveBaseTransport* triggerTrans;
	eveBaseTransport* unitTrans;
	eveBaseTransport* stopTrans;
	QString unitString;
	eveVariant triggerValue;
	eveVariant stopValue;
	double triggerTimeout;
};

#endif /* EVESMDETECTOR_H_ */
