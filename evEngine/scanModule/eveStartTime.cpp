/*
 * eveStartTime.cpp
 *
 *  Created on: 21.05.2012
 *      Author: eden
 */

#include "eveStartTime.h"

QDateTime eveStartTime::starttime=QDateTime(QDate(0,0,0), QTime(0,0,0,0));
QReadWriteLock eveStartTime::lock;

eveStartTime::eveStartTime() {
	// TODO Auto-generated constructor stub

}

eveStartTime::~eveStartTime() {
	// TODO Auto-generated destructor stub
}

QDateTime eveStartTime::getStartTime() {

	QReadLocker locker(&lock);
	return starttime;
}

void eveStartTime::setStartTime(QDateTime newTime) {

	QWriteLocker locker(&lock);
	starttime = newTime;
}

int eveStartTime::getMSecsSinceStart() {

	QReadLocker locker(&lock);
	QDateTime now = QDateTime::currentDateTime();
	int msecs = 86400 * starttime.daysTo(now);
	msecs += starttime.time().msecsTo(now.time());
	return msecs;

}
