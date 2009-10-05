#include "eveNwThread.h"
#include "eveNetObject.h"

eveNwThread::eveNwThread(QWaitCondition* waitRegistration, QMutex *mutex)
{
	channelRegistered = waitRegistration;
	waitMutex = mutex;
}

eveNwThread::~eveNwThread()
{
}

/**
 *  \brief threads run procedure
 */
void eveNwThread::run()
{
	// create an NW-Module
	eveNetObject * netObject = new eveNetObject();
	waitMutex->lock();
	channelRegistered->wakeAll();
	waitMutex->unlock();
	exec();
}
