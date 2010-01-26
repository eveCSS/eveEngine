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
	eveScanManager(eveManager *, eveXMLReader *, int);
	virtual ~eveScanManager();
	// bool setRootSM(eveScanModule *);
	virtual void shutdown();
	void sendError(int, int, QString);
	void sendMessage(eveMessage*);
	virtual void sendError(int, int, int, QString);
	void setStatus(int, smStatusT);
	engineStatusT getChainStatus(){return chainStatus;};
	//eveDeviceList * getDeviceDefs(){return manager->getDeviceDefs();};
	void handleMessage(eveMessage *);
	void nextPos();
	void registerEvent(int, eveEventProperty*, bool chain=false);

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
	QHash<QString, QString> chainHash;
	QHash<QString, QString>* savePluginHash;
	QList<eveEventProperty*> eventPropList;
	int nextEventId;
	int chainId;
	int storageChannel;
	bool useStorage;
	bool sentData;
	eveScanModule * rootSM;
	engineStatusT chainStatus;
	eveManager *manager;
	int posCounter;
	bool doBreak;
	bool waitForMessageBeforeStart;
	bool delayedStart;
	bool shutdownPending;
};

#endif /* EVESCANMANAGER_H_ */
