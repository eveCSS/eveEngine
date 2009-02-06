/*
 * eveScanManager.h
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#ifndef EVESCANMANAGER_H_
#define EVESCANMANAGER_H_

#include <QDomElement>
#include "eveMessageChannel.h"
#include "eveManager.h"
#include "eveXMLReader.h"
#include "eveDeviceList.h"

class eveScanModule;

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
	void shutdown();
	void sendError(int, int, QString);
	virtual void sendError(int, int, int, QString);
	void sendStatus(int, chainStatusT);
	engineStatusT getChainStatus(){return chainStatus;};
	//eveDeviceList * getDeviceDefs(){return manager->getDeviceDefs();};

public slots:
	void smStart(int);
	void smHalt();
	void smBreak();
	void smStop();
	void smPause();
	void smRedo(int eventId);
	void init();
	void smDone();

private:
	int chainId;
	eveScanModule * rootSM;
	engineStatusT chainStatus;
	eveManager *manager;
	int posCounter;
	bool doBreak;
};

#endif /* EVESCANMANAGER_H_ */
