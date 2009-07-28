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
#include "eveDevice.h"
#include "eveVariant.h"
#include "eveCaTransport.h"

class eveScanModule;

enum eveDeviceStatusT {eveDEVICEINIT, eveDEVICEIDLE, eveDEVICEWRITE, eveDEVICEREAD};

/**
 * \brief device in a scanModule
 */
class eveSMDevice : public QObject {

	Q_OBJECT

public:
	eveSMDevice(eveScanModule*, eveDevice*, eveVariant, bool reset=false);
	virtual ~eveSMDevice();
	void init();
	void readValue(bool);
	void writeValue(bool);
	bool resetNeeded(){return setPrevious;};
	bool isDone(){return ready;};
	bool isOK(){return deviceOK;};
	QString getName(){return name;};

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
	QString name;
	QList<eveTransportT> transportList;
	eveBaseTransport* valueTrans;
	eveScanModule* scanModule;
	eveDeviceStatusT deviceStatus;
};

#endif /* EVESMDEVICE_H_ */
