#include "eveStorageThread.h"
#include "eveStorageManager.h"

eveStorageThread::eveStorageThread(QString filename, QWaitCondition* waitRegistration, QMutex *mutex)
{
	fileName = filename;
	storageRegistered = waitRegistration;
	waitMutex = mutex;
}

eveStorageThread::~eveStorageThread()
{
}

/** \brief threads run procedure
 */
void eveStorageThread::run()
{
	// create a Manager
	eveStorageManager *manager = new eveStorageManager(fileName);
	waitMutex->lock();
	storageRegistered->wakeAll();
	waitMutex->unlock();
	exec();
}
