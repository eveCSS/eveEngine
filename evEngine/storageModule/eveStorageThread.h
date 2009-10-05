#ifndef EVESTORAGETHREAD_H_
#define EVESTORAGETHREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>

class eveStorageThread : public QThread
{
public:
	eveStorageThread(QString, QWaitCondition*, QMutex *);
	virtual ~eveStorageThread();
    void run();

private:
	QString fileName;
	QMutex *waitMutex;
	QWaitCondition* storageRegistered;
};

#endif /*EVESTORAGETHREAD_H_*/
