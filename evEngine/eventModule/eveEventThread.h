/*
 * eveEventThread.h
 *
 *  Created on: 28.09.2009
 *      Author: eden
 */

#ifndef EVEEVENTTHREAD_H_
#define EVEEVENTTHREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

/*
 *
 */
class eveEventThread : public QThread {
public:
	eveEventThread(QWaitCondition*, QMutex *);
	virtual ~eveEventThread();
    void run();

private:
	QMutex *waitMutex;
	QWaitCondition* channelRegistered;
};

#endif /* EVEEVENTTHREAD_H_ */
