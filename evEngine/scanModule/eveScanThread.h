/*
 * eveScanThread.h
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#ifndef EVESCANTHREAD_H_
#define EVESCANTHREAD_H_

#include <QThread>
#include "eveScanManager.h"

class eveScanThread : public QThread {
public:
	eveScanThread(eveScanManager *scanmanager);
	virtual ~eveScanThread();
	void run();
	void shutdown();
	eveScanManager * getScanManager(){return scanManager;};
	void millisleep(unsigned long time) { QThread::msleep(time);};

private:
	eveScanManager * scanManager;
};

#endif /* EVESCANTHREAD_H_ */
