/*
 * eveSimplePV.h
 *
 *  Created on: 19.10.2011
 *      Author: eden
 */

#ifndef EVESIMPLEPV_H_
#define EVESIMPLEPV_H_

#include <QString>
#include "cadef.h"
#include <QWaitCondition>
#include <QMutex>

enum simpleCaStatusT {SCSSUCCESS, SCSNOTCONNECTED, SCSERROR};


class eveSimplePV {
public:
	eveSimplePV(QString);
	virtual ~eveSimplePV();
	static void eveSimplePVConnectCB(struct connection_handler_args args);
	static void eveSimplePVGetCB(struct event_handler_args arg);
	simpleCaStatusT getStatus(){return lastStatus;};
	QString& getStringValue(){return pvdata;};
	QString& getErrorString(){return errorText;};
	void wakeUp(simpleCaStatusT);
	void setValueString(char* value);

private:
	simpleCaStatusT connectReadPV(QString);
	void disconnectPV();
	void sendError(QString);
	chid chanChid;
	simpleCaStatusT lastStatus;
	QString pvdata;
	QString errorText;
	QWaitCondition waitForCA;
	QMutex waitLock;

};

#endif /* EVESIMPLEPV_H_ */
