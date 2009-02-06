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
	virtual void sendError(int, int, int, QString);
	void shutdown();
	bool loadPlayListEntry();
	bool createSMs(QByteArray);
	bool sendStart();
	//eveDeviceList * getDeviceDefs(){return deviceList;};

signals:
	void finished();
	void startSMs(int);
	void stopSMs();
	void breakSMs();
	void haltSMs();
	void pauseSMs();

private:
	// eveMessageHub * mHub;
	eveDeviceList *deviceList;
	eveManager *manager;
	evePlayListManager *playlist;
	eveManagerStatusTracker *engineStatus;
	QList<QThread * > scanThreadList;
};

#endif /* EVEMANAGER_H_ */
