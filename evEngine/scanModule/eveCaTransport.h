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
#include <QTimer>
#include <QHash>

#include "eveBaseTransport.h"
#include "eveDevice.h"
#include "cadef.h"
#include "eveData.h"

enum eveTransActionT {eveCONNECT, eveREAD, eveWRITE, eveIDLE };
enum eveTransStatusT {eveCONNECTED, eveNOTCONNECTED, eveTIMEOUT, eveUNDEFINED };

//class eveScanManager;
class eveCaTransport: public eveBaseTransport {

	Q_OBJECT

public:
	eveCaTransport(QObject *parent, QString, eveCaTransportDef*);
	virtual ~eveCaTransport();
	int readData(bool queue=false);
	int writeData(eveVariant, bool queue=false);
	int connectTrans();
	bool isConnected();
	bool haveData(){if (newData == NULL) return false; else return true;};
	eveDataMessage *getData();
	QStringList* getInfo();
	static int execQueue();

	void sendError(int, int,  QString);

//	bool get(eveData* data, bool flush=true, bool wait=true);	// wait => warten bis Ergebnis oder Timeout
	bool getCB(bool flush=true);
	bool put(eveType, int, void*, bool execute=true);
	bool putCB(eveType, int, void*, bool execute=true);
	chtype getRequestType(){return requestType;};
	int getElemCnt(){return dataCount;};
	static void caflush();
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
    void connectTimeout();
    void getTimeout();
    void putTimeout();

signals:
	void writeReady(int);
    void dataReady(eveDataMessage *);
    void connectChange(int);
    void enumReady(QStringList *);

private:
    static QHash<struct ca_client_context *, int> contextCounter;
    QTimer *getTimer;
    QTimer *putTimer;
    void getEnumStrs();
	QString getEnumString(int index);
	QString getName(){return name;};
	eveTransStatusT transStatus;
	eveTransActionT currentAction;
	int dataCount;
	int timeOut;
	QString name;
	QString pvname;
	eveCaTransportDef *transDef;
	QStringList *enumStringList;
	//eveScanManager* scanManager;
	chid chanChid;
	chtype elementType;
	chtype requestType;
	void *dataPtr;
	void *enumData;
	void *writeDataPtr;
	bool needEnums;
	bool enumsInProgress;
	struct ca_client_context *caThreadContext;

};

#endif /* EVECATRANSPORT_H_ */
