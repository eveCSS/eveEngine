/*
 * eveEventThread.cpp
 *
 *  Created on: 28.09.2009
 *      Author: eden
 */

#include "eveEventThread.h"
#include "eveEventManager.h"

eveEventThread::eveEventThread(QWaitCondition* waitRegistration, QMutex *mutex)
{
	channelRegistered = waitRegistration;
	waitMutex = mutex;
}

eveEventThread::~eveEventThread() {
	// TODO Auto-generated destructor stub
}

/** \brief threads run procedure
 */
void eveEventThread::run()
{
	// create a Manager
	eveEventManager *manager = new eveEventManager();
	waitMutex->lock();
	channelRegistered->wakeAll();
	waitMutex->unlock();
	exec();
}
