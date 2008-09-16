#include "eveNwThread.h"
#include "eveNetObject.h"

eveNwThread::eveNwThread()
{
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
	exec();
}
