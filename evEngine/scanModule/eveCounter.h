/*
 * eveCounter.h
 *
 *  Created on: 19.10.2009
 *      Author: eden
 */

#ifndef EVECOUNTER_H_
#define EVECOUNTER_H_

/*
 *
 */
#include <QString>
#include "eveBaseTransport.h"
#include "eveDevice.h"

class eveCounter: public eveBaseTransport {

	Q_OBJECT

public:
	eveCounter(eveSMBaseDevice *, QString, eveTransportDef*);
	virtual ~eveCounter();
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

private:
	bool haveMonitor;
	int currentCount;
	QString name;
	QString accessname;
	eveTransStatusT transStatus;
	eveTransActionT currentAction;

};

#endif /* EVECOUNTER_H_ */
