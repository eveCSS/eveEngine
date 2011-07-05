/*
 * eveCounter.cpp
 *
 *  Created on: 19.10.2009
 *      Author: eden
 */
#include <stdio.h>
#include "eveCounter.h"
#include <QDateTime>
#include "eveError.h"

eveCounter::eveCounter(eveSMBaseDevice *parent, QString xmlid, QString name, eveTransportDef* transdef) : eveBaseTransport(parent, xmlid, name) {

	transStatus = eveUNDEFINED;
	currentAction = eveIDLE;
	haveMonitor = false;
	newData = NULL;
	accessname = transdef->getName();
	currentCount=0;
}

eveCounter::~eveCounter() {
	// TODO Auto-generated destructor stub
}

int eveCounter::connectTrans(){
	emit done(0);
	return 0;
}

// TODO not yet implemented
int eveCounter::monitorTrans(){
	return 0;
}

/**
 * \brief get the data, may be a pointer to NULL
 * @return eveDataMessage or NULL
 */
eveDataMessage* eveCounter::getData(){
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
int eveCounter::readData(bool queue){

	eveDataStatus status = {0,0,0};
	newData = new eveDataMessage(xmlId, name, status, DMTunmodified, eveTime::getCurrent(), QVector<int>(1,currentCount));
	emit done(0);
	return 0;
}
/**
 * @param queue true, if request should be queued (needs execQueue() to actually start writing)
 * @return 0 if ok
 *
 * signals done if ready
 */
int eveCounter::writeData(eveVariant writedata, bool queue){

	if (writedata.getType() != eveINT){
		sendError(MINOR, 0, QString("eveCounter: datatype %1 is not int").arg((int)writedata.getType()));
	}
	bool ok = true;
	currentCount = writedata.toInt(&ok);
	if (!ok) sendError(ERROR, 0, "eveCounter: error converting to int");
	emit done(0);

	return 0;
}

void eveCounter::sendError(int severity, int errorType,  QString message){

	printf("Counter: %d, %s\n", severity, qPrintable(QString("PV %1(%2): %3").arg(name).arg(accessname).arg(message)));
}

/**
 *
 * @return a QStringList pointer which contains info
 */
QStringList* eveCounter::getInfo(){
	QStringList *sl = new QStringList();
	sl->append(QString("Access:local:%1").arg(accessname));
	return sl;
}
