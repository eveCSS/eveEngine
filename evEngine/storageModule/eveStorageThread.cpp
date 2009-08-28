#include "eveStorageThread.h"
#include "eveStorageManager.h"

eveStorageThread::eveStorageThread(QString filename)
{
	fileName = filename;
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
	exec();
}
