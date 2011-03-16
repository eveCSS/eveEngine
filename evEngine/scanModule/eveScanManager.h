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
#include <QDomElement>
#include "eveMessageChannel.h"
#include "eveManager.h"
#include "eveXMLReader.h"
#include "eveDeviceList.h"

class eveScanModule;
class eveEventProperty;

enum smStatusT {eveSmNOTSTARTED, eveSmINITIALIZING, eveSmEXECUTING, eveSmPAUSED, eveSmTRIGGERWAIT, eveSmAPPEND, eveSmDONE} ;

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
	void sendMessage(eveMessage*);
	virtual void sendError(int, int, int, QString);
	void setStatus(int, smStatusT);
	void handleMessage(eveMessage *);
	void nextPos();
	void registerEvent(int, eveEventProperty*, bool chain=false);
	int sendRequest(int, QString);
	void cancelRequest(int smid, int rid);

public slots:
	void smStart();
	void smHalt();
	void smBreak();
	void smStop();
	void smPause();
	void smRedo();
	void init();
	void smDone();
	void newEvent(eveEventProperty*);

private:
	void sendStatus(int, chainStatusT);
	void addToHash(QHash<QString, QString>*, QString, eveXMLReader*);
	QHash<int, int> requestHash;
	QHash<QString, QString> chainHash;
	QHash<QString, QString>* savePluginHash;
	QList<eveEventProperty*> eventPropList;
	int nextEventId;
	int chainId;
	int storageChannel;
	bool useStorage;
	bool sentData;
	eveScanModule * rootSM;
	eveManager *manager;
	int posCounter;
	bool doBreak;
	bool shutdownPending;
};

#endif /* EVESCANMANAGER_H_ */
