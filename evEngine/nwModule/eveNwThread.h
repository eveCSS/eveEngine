#ifndef EVENWTHREAD_H_
#define EVENWTHREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "eveNetObject.h"

class eveNwThread : public QThread
{
public:
	eveNwThread(QWaitCondition*, QMutex *);
	virtual ~eveNwThread();
    void run();

private:
    eveNetObject *netObject;
	QMutex *waitMutex;
	QWaitCondition* channelRegistered;
};

#endif /*EVENWTHREAD_H_*/
