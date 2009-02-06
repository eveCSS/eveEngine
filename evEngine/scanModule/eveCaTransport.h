/*
 * eveCaTransport.h
 *
 *  Created on: 13.01.2009
 *      Author: eden
 */

#ifndef EVECATRANSPORT_H_
#define EVECATRANSPORT_H_

#include <QObject>
#include <QStringList>
#include <QString>

#include "eveBaseTransport.h"
#include "eveDevice.h"
#include "cadef.h"
#include "eveData.h"

enum eveTransActionT {eveCONNECT, eveREAD, eveWRITE, eveIDLE };
enum eveTransStatusT {CONNECTED, NOTCONNECTED, UNDEFINED };

class eveScanManager;

class eveCaTransport: public eveBaseTransport {

	Q_OBJECT

public:
	eveCaTransport(eveCaTransportDef*);
	virtual ~eveCaTransport();
	int readData(bool queue=false);
	int writeData(eveVariant, bool queue=false);
	int connectTrans();
	static int execQueue();

	void sendError(int, int,  QString);

//	bool get(eveData* data, bool flush=true, bool wait=true);	// wait => warten bis Ergebnis oder Timeout
	bool getCB(bool flush=true);
	bool put(eveType, int, void*, bool execute=true);
	bool putCB(eveType, int, void*, bool execute=true);
	bool isConnected();
	chtype getRequestType(){return requestType;};
	int getElemCnt(){return dataCount;};
	static void flush();
	static void eveCaTransportConnectCB(struct connection_handler_args args);
	static void eveCaTransportGetCB(struct event_handler_args arg);
	static void eveCaTransportPutCB(struct event_handler_args arg);
	static void eveCaTransportEnumCB(struct event_handler_args arg);
	static epicsType convertEveToEpicsType(eveType intype){return (epicsType) intype;};
	static int convertEpicsToDBR(epicsType type){return epicsTypeToDBR_XXXX[type];};

public slots:
	void setCnctStatus(int);
	void readDone(eveDataMessage *);
	void writeDone(int);
    void enumDone(QStringList *);

signals:
	void writeReady(int);
    void dataReady(eveDataMessage *);
    void connectChange(int);
    void enumReady(QStringList *);

private:
	void getEnumStrs();
	QString getEnumString(int index);
	QString getName(){return name;};
	eveTransStatusT transStatus;
	eveTransActionT currentAction;
	int dataCount;
	QString name;
	eveCaTransportDef *transDef;
	QStringList *enumStringList;
	eveScanManager* scanManager;
	chid chanChid;
	chtype elementType;
	chtype requestType;
	void *dataPtr;
	void *enumData;
	void *writeDataPtr;
	bool needEnums;
	bool enumsInProgress;
};

#endif /* EVECATRANSPORT_H_ */
