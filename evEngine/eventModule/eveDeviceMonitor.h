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
#include "eveMonitorRegisterMessage.h"

class eveEventManager;
/*
 *
 */
class eveDeviceMonitor : public eveSMBaseDevice {

	Q_OBJECT

public:
	eveDeviceMonitor(eveEventManager*, eveEventProperty*);
	eveDeviceMonitor(eveEventManager*, eveMonitorRegisterMessage*);
	virtual ~eveDeviceMonitor();
	int getDestination(){return destination;};
	void sendError(int, int, int, QString);

public slots:
	void valueChange(eveDataMessage*);
	void saveValue(eveDataMessage*);

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
	QString xmlId;
	int destination;
};

#endif /* EVEDEVICEMONITOR_H_ */
