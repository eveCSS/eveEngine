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
#include <QDateTime>
#include <QString>
#include "eveTypes.h"
#include "eveBaseTransport.h"
#include "eveDeviceDefinitions.h"
#include "eveTime.h"

class eveTimer: public eveBaseTransport {

	Q_OBJECT

public:
	eveTimer(eveSMBaseDevice *, QString, QString, eveTransportDefinition*);
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
	void setStartTime(QDateTime start){startTime = start;startInitialized =true;};
	QDateTime getStartTime(){return startTime;};
	int execQueue();

public slots:
	void waitDone();


private:
	int getMSecsUntil(QDateTime);
	eveSMBaseDevice* baseDev;
	bool haveMonitor;
	bool startInitialized;
	eveType datatype;
	QTimer timer;
	int timerId;
	QDateTime startTime;
	QString accessname;
	QDateTime targetTime;
	eveTransStatusT transStatus;
	eveTransActionT currentAction;

};

#endif /* EVETIMER_H_ */
