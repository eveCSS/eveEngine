/*
 * eveTimer.h
 *
 *  Created on: 13.10.2009
 *      Author: eden
 */

#ifndef EVETIMER_H_
#define EVETIMER_H_

/*
 *
 */
#include <QTimer>
#include <QString>
#include "eveBaseTransport.h"
#include "eveDevice.h"
#include "eveTime.h"

class eveTimer: public eveBaseTransport {

	Q_OBJECT

public:
	eveTimer(eveSMBaseDevice *, QString, QString, eveTransportDef*);
	virtual ~eveTimer();
	int readData(bool);
	int writeData(eveVariant, bool);
	int connectTrans();
	int monitorTrans();
	bool isConnected(){return true;};
	bool haveData(){return true;};
	eveDataMessage *getData();
	QStringList* getInfo();
	void sendError(int, int,  QString);
	int execQueue();

public slots:
	void waitDone();


private:
	eveSMBaseDevice* baseDev;
	bool haveMonitor;
	eveType datatype;
	QTimer timer;
	int timerId;
	QString accessname;
	QDateTime targetTime;
	eveTransStatusT transStatus;
	eveTransActionT currentAction;

};

#endif /* EVETIMER_H_ */
