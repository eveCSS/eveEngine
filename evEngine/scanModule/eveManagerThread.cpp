#include "eveManagerThread.h"
#include "eveManager.h"

eveManagerThread::eveManagerThread(QWaitCondition* waitRegistration, QMutex *mutex)
{
    channelRegistered = waitRegistration;
    waitMutex = mutex;
}

eveManagerThread::~eveManagerThread()
{
}

/** \brief threads run procedure
 */
void eveManagerThread::run()
{
    // create a Manager
    eveManager *manager = new eveManager();
    waitMutex->lock();
    channelRegistered->wakeAll();
    waitMutex->unlock();
    exec();
    delete manager;
}
