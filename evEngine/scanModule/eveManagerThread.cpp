#include "eveManagerThread.h"
#include "eveManager.h"

eveManagerThread::eveManagerThread()
{
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
	exec();
}
