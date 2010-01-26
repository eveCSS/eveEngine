/*
 * eveTime.cpp
 *
 *  Created on: 13.10.2009
 *      Author: eden
 */

#include "eveTime.h"

eveTime::eveTime() : epicsTime(){
}

/**
 *
 * @param et time as epicsTime
 * @return converted time from epicsTime
 */
eveTime::eveTime(epicsTime et) : epicsTime(et){
}
/**
 *
 * @param ts time
 * @return converted time from struct timespec
 */
eveTime::eveTime(struct timespec ts ) : epicsTime(ts){
}

/**
 *
 * @param time_t
 * @return converted time from time_t
 */
eveTime::eveTime(time_t_wrapper ttw) : epicsTime(ttw){
}

/**
 *
 * @param dt QDateTime
 * @return converted time from QDateTime
 */
eveTime eveTime::eveTimeFromDateTime(QDateTime dt){
	return eveTimeFromTime_tNano(dt.toTime_t(), dt.time().msec()*1000);
}

/**
 *
 * @param secs seconds since 1.1.1970
 * @param nsecs nanoseconds
 * @return eveTime with seconds and nanoseconds set
 */
eveTime eveTime::eveTimeFromTime_tNano(quint32 secs, quint32 nsecs){

	local_tm_nano_sec ansinano;
	ansinano.ansi_tm = *(localtime((time_t*) &secs));
	ansinano.nSec = nsecs;
	epicsTime et = ansinano;
	return eveTime(et);
}

/**
 *
 * @return current Time
 */
eveTime eveTime::getCurrent(){
	return eveTime(epicsTime::getCurrent());
}

/**
 *
 * @return seconds since 1.1.1970 00:00:00
 */
quint32 eveTime::seconds(){
	time_t_wrapper ttw = (epicsTime) *this;
	return ttw.ts;
}

/**
 *
 * @return nanoseconds (extended nanoseconds)
 */
quint32 eveTime::nanoSeconds(){
	local_tm_nano_sec ansinano;
	ansinano = (epicsTime) *this;
	return ansinano.nSec;
}

/**
 *
 * @return time as 64bit unsigned integer with the first 32 bit holding seconds since
 * 1.1.1970 and second 32 bit holding nanoseconds
 */
quint64 eveTime::get64bitTime(){
	quint64 tmp = seconds();
	return ((tmp << 32) + nanoSeconds());
}

/**
 *
 * @return time as QDateTime
 */
QDateTime eveTime::getDateTime(){
	QDateTime dt = QDateTime::fromTime_t(seconds());
	dt.addMSecs(nanoSeconds()/1000);
	return dt;
}
