/*
 * eveStorageManager.h
 *
 *  Created on: 28.08.2009
 *      Author: eden
 */

#ifndef EVESTORAGEMANAGER_H_
#define EVESTORAGEMANAGER_H_

#include <QObject>
#include <QHash>
#include <QString>
#include "eveMessageChannel.h"

class eveDataCollector;

class eveStorageManager : public eveMessageChannel
{
	Q_OBJECT

public:
	eveStorageManager(QString);
	virtual ~eveStorageManager();
	void shutdown();
	void handleMessage(eveMessage *);
	void sendError(int, int, QString);

private:
	void sendError(int, int, int, QString);
	bool configStorage(eveStorageMessage*);
	QHash<int, int> chainIdChannelHash;
	QHash<int, eveDataCollector* > chainIdDCHash;
	QString fileName;


};

#endif /* EVESTORAGEMANAGER_H_ */
