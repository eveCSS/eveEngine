#include "eveStorageThread.h"

eveStorageThread::eveStorageThread(QString filename, int cid, eveXMLReader* reader, QByteArray* xmldata, QWaitCondition* waitRegistration, QMutex *mutex)
{
	fileName = filename;
	storageRegistered = waitRegistration;
	waitMutex = mutex;
	xmlData = xmldata;
	chainId = cid;
	xmlReader = reader;
}

eveStorageThread::~eveStorageThread()
{
}

/** \brief threads run procedure
 */
void eveStorageThread::run()
{
	// create a Manager
	manager = new eveStorageManager(fileName, chainId, xmlReader, xmlData);
	xmlData = NULL;
	waitMutex->lock();
	storageRegistered->wakeAll();
	waitMutex->unlock();
	exec();
}
