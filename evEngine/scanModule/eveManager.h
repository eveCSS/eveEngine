/*
 * eveManager.h
 *
 *  Created on: 08.09.2008
 *      Author: eden
 */

#ifndef EVEMANAGER_H_
#define EVEMANAGER_H_

#include <QString>
#include <QList>
#include <QThread>
#include "eveMessageChannel.h"
#include "evePlayListManager.h"
#include "eveStatusTracker.h"
#include "eveDeviceList.h"

/**
 * \brief creates and controls all scanmodules, manages playlist stuff
 */
class eveManager: public eveMessageChannel
{
	Q_OBJECT

public:
	eveManager();
	virtual ~eveManager();
	void handleMessage(eveMessage *);
	void sendError(int, int, QString);
	void sendError(int, int, int, QString);
	void shutdown();
	bool sendStart();

signals:
	void finished();
	void startSMs();
	void stopSMs();
	void breakSMs();
	void haltSMs();
	void pauseSMs();

private:
	bool loadPlayListEntry();
	bool createSMs(QByteArray, bool);
	void startChains();
	// eveMessageHub * mHub;
	evePlayListData* currentPlEntry;
	eveManager *manager;
	evePlayListManager *playlist;
	eveManagerStatusTracker *engineStatus;
	QList<QThread * > scanThreadList;
	bool shutdownPending;

};

#endif /* EVEMANAGER_H_ */
