/*
 * eveSMDevice.h
 *
 *  Created on: 04.02.2009
 *      Author: eden
 */

#ifndef EVESMDEVICE_H_
#define EVESMDEVICE_H_

#include <QObject>
#include <QList>
#include "eveSMBaseDevice.h"
#include "eveDeviceDefinitions.h"
#include "eveVariant.h"
#include "eveCaTransport.h"

class eveScanModule;

enum eveDeviceStatusT {eveDEVICEINIT, eveDEVICEIDLE, eveDEVICEWRITE, eveDEVICEREAD};

/**
 * \brief device in a scanModule
 */
class eveSMDevice : public eveSMBaseDevice {

	Q_OBJECT

public:
	eveSMDevice(eveScanModule*, eveDeviceDefinition*, eveVariant, bool reset=false);
	virtual ~eveSMDevice();
	void init();
	void readValue(bool);
	void writeValue(bool);
	bool resetNeeded(){return setPrevious;};
	bool isDone(){return ready;};
	bool isOK(){return deviceOK;};
	void sendError(int, int, int, QString);


public slots:
	void transportReady(int status);

signals:
	void deviceDone();

private:
	void sendError(int, int, QString);
	eveVariant value;
	bool setPrevious;
	bool haveValue;
	bool haveResetValue;
	bool ready;
	bool deviceOK;
	int signalCounter;
	QList<eveTransportT> transportList;
	eveBaseTransport* valueTrans;
	eveScanModule* scanModule;
	eveDeviceStatusT deviceStatus;
};

#endif /* EVESMDEVICE_H_ */
