/*
 * eveScanThread.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include "eveScanThread.h"
#include "eveScanManager.h"

eveScanThread::eveScanThread(eveScanManager *scanmanager) {
	scanManager = scanmanager;
}

eveScanThread::~eveScanThread() {
	// TODO Auto-generated destructor stub
}

void eveScanThread::run()
{
	//printf("ScanThread starting\n");
	scanManager->init();
    exec();
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility where the error occured
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveScanThread::sendError(int severity, int facility, int errorType,  QString errorString){
	scanManager->sendError(severity, facility, errorType, errorString);
}

void eveScanThread::shutdown()
{
	delete scanManager;
	// ca_context_destroy();
	quit();
}

