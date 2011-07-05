/*
 * eveTimer.cpp
 *
 *  Created on: 13.10.2009
 *      Author: eden
 */

#include "eveTimer.h"
#include "eveError.h"
#include <limits.h>

eveTimer::eveTimer(eveSMBaseDevice *parent, QString xmlid, QString name, eveTransportDef* transdef) : eveBaseTransport(parent, xmlid, name), timer(parent){

	baseDev = parent;
	transStatus = eveUNDEFINED;
	currentAction = eveIDLE;
	haveMonitor = false;
	newData = NULL;
	accessname = transdef->getName();
	datatype = transdef->getDataType();
	//method = transdef->getMethod();
	timer.setSingleShot(true);
	timerId = timer.timerId();
	startInitialized = false;
}

eveTimer::~eveTimer() {
	// TODO Auto-generated destructor stub
}

int eveTimer::connectTrans(){
	emit done(0);
	return 0;
}

int eveTimer::monitorTrans(){
	return 0;
}

/**
 * \brief get the data, may be a pointer to NULL
 * @return eveDataMessage or NULL
 */
eveDataMessage* eveTimer::getData(){
	eveDataMessage *return_data = newData;
	newData = NULL;
	return return_data;
};

/**
 * @param queue true, if request should be queued (needs execQueue() to actually start reading)
 * @return 0 if successful
 *
 * signals done if ready
 * ( we don't do calls to caget(), but use caget_callback() )
 */
int eveTimer::readData(bool queue){

	eveDataStatus status = {0,0,0};
	eveTime currentTime = eveTime::getCurrent();
	// TODO
	// for some reason the first conversion doesn't work (mSecs are always 0)
	//QDateTime now = QDateTime(currentTime.getDateTime());
	//eveTime testTime = eveTime::eveTimeFromDateTime(now);
	// for now we do this
	QDateTime now = QDateTime::currentDateTime();

	if (datatype == eveDateTimeT){
		// absolute time
		newData = new eveDataMessage(xmlId, name, status, DMTunmodified, currentTime, now);
	}
	else if (datatype == eveINT){
		// not absolute time
		QVector<int> iArray;
		if (startInitialized){
			iArray.append(getMSecsUntil(now));
		}
		else {
			iArray.append(0);
		}
		// TODO
		// hdf plugin expects an int array of size 2; change it
		iArray.append(0);
		newData = new eveDataMessage(xmlId, name, status, DMTunmodified, currentTime, iArray);
	}
	else if (datatype == eveDOUBLE){
		// not absolute time
		QVector<double> dArray;
		if (startInitialized){
			dArray.append(((double)getMSecsUntil(now))/1000.0);
		}
		else {
			dArray.append(0.0);
		}
		newData = new eveDataMessage(xmlId, name, status, DMTunmodified, currentTime, dArray);
	}
	else {
		sendError(MINOR, 0, QString("unknown datatype (%1) for Timer").arg((int)datatype));
	}
	emit done(0);
	return 0;
}
/**
 * @param queue true, if request should be queued (needs execQueue() to actually start writing)
 * @return 0 if ok
 *
 * signals done if ready
 */
int eveTimer::writeData(eveVariant writedata, bool queue){

	QDateTime currentDT = QDateTime::currentDateTime();

	if (writedata.getType() == eveDateTimeT){

		QDateTime writeDT = writedata.toDateTime();

		sendError(DEBUG, 0, QString("eveTimer: now: %1 goto: %2").arg(currentDT.time().toString("hh:mm:ss.zzz")).arg(writeDT.time().toString("hh:mm:ss.zzz")));
		if (currentDT >= writeDT){
			waitDone();
		}
		else {
			// check for day wrap
			int msecsOffset = 0 ;
			if (currentDT.date() != writeDT.date())
				msecsOffset = currentDT.secsTo(writeDT) * 1000;
			sendError(DEBUG, 0, QString("eveTimer: load timer with %1 msecs").arg(msecsOffset + currentDT.time().msecsTo(writeDT.time())));
			QTimer::singleShot (msecsOffset + currentDT.time().msecsTo(writeDT.time()), this, SLOT (waitDone()));
		}
	}
	else if (writedata.getType() == eveInt32T){
		bool ok = true;
		int mSecs=writedata.toInt(&ok);
		if (!ok){
			sendError(DEBUG, 0, QString("Error converting data to integer (msecs)"));
		}
		else {
			mSecs -= getMSecsUntil(currentDT);
			if (mSecs < 0 ) mSecs=0;
			sendError(DEBUG, 0, QString("eveTimer: load timer with %1 msecs").arg(mSecs));
			QTimer::singleShot (mSecs, this, SLOT (waitDone()));
		}
	}
	else if (writedata.getType() == eveFloat64T){
		bool ok = true;
		double mSecsD=writedata.toDouble(&ok);
		if (!ok){
			sendError(DEBUG, 0, QString("Error converting data to double (msecs)"));
		}
		else {
			int mSecs = (int)(mSecsD * 1000.0);
			mSecs -= getMSecsUntil(currentDT);
			if (mSecs < 0 ) mSecs=0;
			sendError(DEBUG, 0, QString("eveTimer: load timer with %1 msecs").arg(mSecs));
			QTimer::singleShot (mSecs, this, SLOT (waitDone()));
		}
	}
	else {
		sendError(MINOR, 0, QString("eveTimer: datatype %1 is not DateTime").arg((int)writedata.getType()));
	}


	return 0;
}

void eveTimer::waitDone() {

	emit done(0);
}

void eveTimer::sendError(int severity, int errorType,  QString message){

	baseDev->sendError(severity, EVEMESSAGEFACILITY_LOCALTIMER, errorType,
							QString("Timer %1: %2").arg(name).arg(message));
}

/**
 *
 * @return a QStringList pointer which contains info
 */
QStringList* eveTimer::getInfo(){
	QStringList *sl = new QStringList();
	sl->append(QString("Access:local:%1").arg(accessname));
	return sl;
}

int eveTimer::getMSecsUntil(QDateTime now){
	qint64 delta = startTime.daysTo(now) * 86400000;
	delta += startTime.time().msecsTo(now.time());

	if (delta > INT_MAX){
		sendError(ERROR, 0, QString("integer overflow (%1) for Timer").arg(delta));
		return INT_MAX;
	}
	else if (delta <= INT_MIN){
		sendError(ERROR, 0, QString("integer overflow (%1) for Timer").arg(delta));
		return INT_MIN;
	}

	return (int) delta;

}
