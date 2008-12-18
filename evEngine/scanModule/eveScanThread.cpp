/*
 * eveScanThread.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include "eveScanThread.h"

eveScanThread::eveScanThread(eveScanManager *scanmanager) {
	scanManager = scanmanager;
}

eveScanThread::~eveScanThread() {
	// TODO Auto-generated destructor stub
}

void eveScanThread::run()
{
	scanManager->init();
    exec();
}

void eveScanThread::shutdown()
{
	delete scanManager;
	// ca_context_destroy();
	quit();
}

