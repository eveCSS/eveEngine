#include "eveStorageThread.h"

eveStorageThread::eveStorageThread(QString filename, QByteArray* xmldata, QWaitCondition* waitRegistration, QMutex *mutex)
{
	fileName = filename;
	storageRegistered = waitRegistration;
	waitMutex = mutex;
	xmlData = xmldata;
}

eveStorageThread::~eveStorageThread()
{
}

/** \brief threads run procedure
 */
void eveStorageThread::run()
{
	// create a Manager
	manager = new eveStorageManager(fileName, xmlData);
	xmlData = NULL;
	waitMutex->lock();
	storageRegistered->wakeAll();
	waitMutex->unlock();
	exec();
}
