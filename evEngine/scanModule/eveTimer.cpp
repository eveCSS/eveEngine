/*
 * eveTimer.cpp
 *
 *  Created on: 13.10.2009
 *      Author: eden
 */

#include "eveTimer.h"
#include "eveError.h"

eveTimer::eveTimer(eveSMBaseDevice *parent, QString devname, eveTransportDef* transdef) : eveBaseTransport(parent), timer(parent){

	baseDev = parent;
	transStatus = eveUNDEFINED;
	currentAction = eveIDLE;
	haveMonitor = false;
	name = devname;
	newData = NULL;
	accessname = transdef->getName();
	//method = transdef->getMethod();
	timer.setSingleShot(true);
	timerId = timer.timerId();
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
	QDateTime dt = QDateTime::currentDateTime();
	newData = new eveDataMessage(name, status, DMTunmodified, eveTime::eveTimeFromDateTime(dt), dt);
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

	if (writedata.getType() != eveDateTimeT){
		sendError(MINOR, 0, QString("eveTimer: datatype %1 is not DateTime").arg((int)writedata.getType()));
	}

	QDateTime currentDT = QDateTime::currentDateTime();
	QDateTime writeDT = writedata.toDateTime();

	if (currentDT >= writeDT)
		waitDone();
	else {
		// check for day wrap
		int msecsOffset = 0 ;
		if (currentDT.date() != writeDT.date())
			msecsOffset = currentDT.secsTo(writeDT) * 1000;
		sendError(DEBUG, 0, QString("eveTimer: load timer with %1 msecs").arg(msecsOffset + currentDT.time().msecsTo(writeDT.time())));
		QTimer::singleShot (msecsOffset + currentDT.time().msecsTo(writeDT.time()), this, SLOT (waitDone()));
	}
	// call clock, signal waitDone

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
	sl->append(QString("Local: %1").arg(accessname));
	return sl;
}
