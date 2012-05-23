/*
 * eveStartTime.h
 *
 *  Created on: 21.05.2012
 *      Author: eden
 */

#ifndef EVESTARTTIME_H_
#define EVESTARTTIME_H_

#include <QDateTime>
#include <QReadWriteLock>

class eveStartTime {
public:
	eveStartTime();
	virtual ~eveStartTime();
	static int getMSecsSinceStart();
	static void setStartTime(QDateTime);
	static QDateTime getStartTime();

	static QDateTime starttime;
	static QReadWriteLock lock;
};

#endif /* EVESTARTTIME_H_ */
