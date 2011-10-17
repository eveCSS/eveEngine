#include "eveNwThread.h"

eveNwThread::eveNwThread(QWaitCondition* waitRegistration, QMutex *mutex)
{
	channelRegistered = waitRegistration;
	waitMutex = mutex;
}

eveNwThread::~eveNwThread()
{
	delete netObject;
}

/**
 *  \brief threads run procedure
 */
void eveNwThread::run()
{
	// create an NW-Module
	netObject = new eveNetObject();
	waitMutex->lock();
	channelRegistered->wakeAll();
	waitMutex->unlock();
	exec();
}
