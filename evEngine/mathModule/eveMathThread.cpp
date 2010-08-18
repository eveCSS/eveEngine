/*
 * eveMathThread.cpp
 *
 *  Created on: 20.05.2010
 *      Author: eden
 */

#include "eveMathThread.h"

eveMathThread::eveMathThread(int chainId, QList<eveMathConfig*>* mathList) {
	chid = chainId;
	mathConfigList = mathList;
}

eveMathThread::~eveMathThread() {
	// TODO Auto-generated destructor stub
}

void eveMathThread::run()
{
	// create a Manager
	eveMathManager *manager = new eveMathManager(chid, mathConfigList);
	mathConfigList = NULL;
	exec();
}
