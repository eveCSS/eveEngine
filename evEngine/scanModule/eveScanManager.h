/*
 * eveScanManager.h
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#ifndef EVESCANMANAGER_H_
#define EVESCANMANAGER_H_

#include <QList>
#include <QHash>
#include <QTimer>
#include <QDateTime>
#include <QDomElement>
#include "eveMessageChannel.h"
#include "eveManager.h"
#include "eveXMLReader.h"
#include "eveDeviceList.h"
#include "eveChainStatus.h"

class eveScanModule;
class eveEventProperty;

enum smTypeT {eveSmTypeROOT, eveSmTypeNESTED, eveSmTypeAPPENDED};


/**
 * \brief Manager class for scanModules
 *
 * receive events and call the corresponding SM-method
 *
 */
class eveScanManager : public eveMessageChannel
{
	Q_OBJECT

public:
	eveScanManager(eveManager *, eveXMLReader *, int, int);
	virtual ~eveScanManager();
	virtual void shutdown();
	void sendError(int, int, QString);
	void sendError(int, int, int, QString);
	void sendMessage(eveMessage*);
    void setStatus(int, eveSMStatus &smstatus);
	void handleMessage(eveMessage *);
	void nextPos();
	void registerEvent(int, eveEventProperty*, bool chain=false);
	int sendRequest(int, QString);
	void cancelRequest(int rid);
	int getPositionCount(){return posCounter;};

public slots:
	void smStart();
	void smHalt();
	void smBreak();
	void smStop();
	void smPause();
//	void smRedo();
	void init();
	void smDone();
	void newEvent(eveEventProperty*);
	void sendRemainingTime();

private:
    void sendStatus();
	void sendStartTime();
	void addToHash(QHash<QString, QString>&, QString, eveXMLReader*);
	QHash<int, int> requestHash;
	QHash<QString, QString> chainHash;
	QHash<QString, QString> savePluginHash;
	QList<eveEventProperty*> eventPropList;
	eveChainStatus currentStatus;
	int nextEventId;
	int chainId;
	int storageChannel;
	bool useStorage;
	eveScanModule * rootSM;
	eveManager *manager;
	int posCounter;
        int totalPositions;
	bool doBreak;
        bool isStarted;
	bool shutdownPending;
	QTimer *sendStatusTimer;
        QDateTime startTime;
	QList<eveEventProperty*>* eventList;
};

#endif /* EVESCANMANAGER_H_ */
