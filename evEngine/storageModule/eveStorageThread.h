#ifndef EVESTORAGETHREAD_H_
#define EVESTORAGETHREAD_H_

#include "eveStorageManager.h"

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>

class eveStorageThread : public QThread
{
public:
	eveStorageThread(QString, int, eveXMLReader*, QByteArray*, QWaitCondition*, QMutex *);
	virtual ~eveStorageThread();
	int getChannelId(){return manager->getChannelId();};
    void run();

private:
    eveStorageManager *manager;
	QString fileName;
	QByteArray* xmlData;
	QMutex *waitMutex;
	eveXMLReader* xmlReader;
	int chainId;
	QWaitCondition* storageRegistered;
};

#endif /*EVESTORAGETHREAD_H_*/
