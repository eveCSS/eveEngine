/*
 * eveDeviceMonitor.h
 *
 *  Created on: 07.10.2009
 *      Author: eden
 */

#ifndef EVEDEVICEMONITOR_H_
#define EVEDEVICEMONITOR_H_

#include <QObject>
#include "eveEventProperty.h"
#include "eveBaseTransport.h"
#include "eveSMBaseDevice.h"

class eveEventManager;
/*
 *
 */
class eveDeviceMonitor : public eveSMBaseDevice {

	Q_OBJECT

public:
	eveDeviceMonitor(eveEventManager*, eveEventProperty*);
	virtual ~eveDeviceMonitor();
	void sendError(int, int, int, QString);

public slots:
	void valueChange(eveVariant*);

private:
	bool operatorEQ(eveVariant value){return (value == limit);};
	bool operatorNE(eveVariant value){return (value != limit);};
	bool operatorGT(eveVariant value){return (value > limit);};
	bool operatorLT(eveVariant value){return (value < limit);};
	eveBaseTransport* monitorTrans;
	eveEventProperty* event;
	eveEventManager* manager;
	eveVariant limit;
	bool(eveDeviceMonitor::* compare)(eveVariant);
};

#endif /* EVEDEVICEMONITOR_H_ */
