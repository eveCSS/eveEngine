/*
 * eveTime.h
 *
 *  Created on: 13.10.2009
 *      Author: eden
 */

#ifndef EVETIME_H_
#define EVETIME_H_

#include <QDateTime>
#include <epicsTime.h>

class eveTime: public epicsTime {

public:
	eveTime();
	eveTime(epicsTime);
	eveTime(struct timespec);
	eveTime(time_t_wrapper);
	static eveTime getCurrent();
	static eveTime eveTimeFromDateTime(QDateTime);
	static eveTime eveTimeFromTime_tNano(quint32, quint32);
	quint32 seconds();
	quint32 nanoSeconds();
	quint64 get64bitTime();
	QDateTime getDateTime();
};
#endif /* EVETIME_H_ */
