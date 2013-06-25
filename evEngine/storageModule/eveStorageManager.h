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
#include "eveXMLReader.h"

class eveDataCollector;

class eveStorageManager : public eveMessageChannel
{
	Q_OBJECT

public:
	eveStorageManager(QString, int, eveXMLReader*, QByteArray* );
	virtual ~eveStorageManager();
	void shutdown();
	void handleMessage(eveMessage *);
	void sendError(int, int, QString);
	void sendError(int, int, int, QString);
	int getChannelId(){return channelId;};

private:
	void initShutdown();
  eveChainStatusMessage* delayedStatus;
	bool configStorage(eveStorageMessage*);
	void addToHash(QHash<QString, QString>&, int, QString, eveXMLReader*);
        int confirmSaveRid;
        QHash<int, int> chainIdChannelHash;
        eveDataCollector* dc;
	QString fileName;
	//QByteArray* xmlData;
	bool shutdownPending;


};

#endif /* EVESTORAGEMANAGER_H_ */
