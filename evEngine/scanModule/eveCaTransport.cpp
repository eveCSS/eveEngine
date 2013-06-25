/*
 * eveCaTransport.cpp
 *
 *  Created on: 13.01.2009
 *      Author: eden
 */

#include <QTimer>
#include "eveMessage.h"
#include "eveCaTransport.h"
#include "eveError.h"
#include "db_access.h"

QHash<struct ca_client_context *, int> eveCaTransport::contextCounter = QHash<struct ca_client_context *, int>();
QReadWriteLock eveCaTransport::contextLock;

//TODO
// improve monitor, with monitor on, monitor off etc.
//TODO
// allow or disallow monitoring and get or put

/**
 * \brief init connection to EPICS PV
 * \param transdef definition of EPICS PV related properties
 * \param parent
 */
eveCaTransport::eveCaTransport(eveSMBaseDevice *parent, QString xmlid, QString name, eveTransportDefinition* transdef) : eveBaseTransport(parent, xmlid, name)
{

	transStatus = eveUNDEFINED;
	currentAction = eveIDLE;
	needEnums = true;
	enumsInProgress = false;
	isMonitorOn = false;
	chanChid = 0;
	dataPtr = NULL;
	writeDataPtr = NULL;
	newData = NULL;
	caThreadContext = NULL;
	pvname = transdef->getName();
	method = transdef->getMethod();
	dataType = transdef->getDataType();
	baseDevice = parent;
	int timeOut = (int)(transdef->getTimeout()*1000.0);

	getTimer = new QTimer(this);
	getTimer->setSingleShot(true);
	getTimer->setInterval(timeOut);
	connect(getTimer, SIGNAL(timeout()), this, SLOT(getTimeout()));
	putTimer = new QTimer(this);
	putTimer->setSingleShot(true);
	putTimer->setInterval(timeOut);
	connect(putTimer, SIGNAL(timeout()), this, SLOT(putTimeout()));

	connect (this, SIGNAL(connectChange(int)), this, SLOT(setCnctStatus(int)), Qt::QueuedConnection);
	connect (this, SIGNAL(dataReady(eveDataMessage *)), this, SLOT(readDone(eveDataMessage *)), Qt::QueuedConnection);
	connect (this, SIGNAL(writeReady(int)), this, SLOT(writeDone(int)), Qt::QueuedConnection);
	connect (this, SIGNAL(enumReady(QStringList *)), this, SLOT(enumDone(QStringList *)), Qt::QueuedConnection);

}

eveCaTransport::~eveCaTransport(){

	QWriteLocker locker(&contextLock);
	if (contextCounter.contains(caThreadContext)){
		int tmp = contextCounter.value(caThreadContext);
		--tmp;
		contextCounter.insert(caThreadContext, tmp);
		if (tmp < 0){
			eveError::log(ERROR, "CA Transport: deleting unused channel ");
		}
		else if (tmp > 0) {
			ca_clear_channel(chanChid);
			caflush();
		}
		else {
			ca_clear_channel(chanChid);
			caflush();
//			eveError::log(DEBUG, QString("all channels done, destroy CA Context %1").arg((int)caThreadContext));
			if (caThreadContext == ca_current_context()){
				ca_context_destroy();
				contextCounter.remove(caThreadContext);
			}
			else
				eveError::log(ERROR, "thread / CA context mismatch");
		}
	}
}

/**
 * \brief connect the transport, signals done if ready
 */
int eveCaTransport::connectTrans(){

	int status;
	int retstat=0;

	if (currentAction == eveCONNECT) {
		return 0;
	}
	else if (transStatus == eveCONNECTED){
		sendError(MINOR, 0, "CA Transport already connected");
		if (currentAction == eveIDLE) emit done(1);
		return 1;
	}
	currentAction = eveCONNECT;

	caThreadContext = ca_current_context();
	contextLock.lockForWrite();
	if (caThreadContext == NULL) {
		sendError(DEBUG, 0, "creating CA Context");
		status = ca_context_create (ca_enable_preemptive_callback);
		caThreadContext = ca_current_context();
		if (status != ECA_NORMAL) sendError(ERROR, 0, "Error creating CA Context");
		contextCounter.insert(caThreadContext, 0);
	}
	int tmp = contextCounter.value(caThreadContext);
	contextCounter.insert(caThreadContext, ++tmp);
	contextLock.unlock();

	// sendError(DEBUG, 0, QString("CaTransport ContextCount: %1 (%2)").arg(tmp).arg((int)caThreadContext));

	status=ca_create_channel(pvname.toAscii().data(), &eveCaTransportConnectCB, (void*) this, 0, &chanChid);
	if(status != ECA_NORMAL){
		sendError(ERROR, 0, "Error creating CA Channel");
		retstat = 1;
	}
	ca_pend_io(0.0);
	int cTimeout=5000;
	QTimer::singleShot(cTimeout, this, SLOT(connectTimeout()));
	return retstat;
}

/** \brief connection callback
 * \param arg epics callback structure
 *
 * method is called if connection changes occur; it may be called from a different thread
 */
void eveCaTransport::eveCaTransportConnectCB(struct connection_handler_args arg){

	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);
	if (arg.op == CA_OP_CONN_UP) {
		emit pv->connectChange((int)eveCONNECTED);
	}
	else {
		emit pv->connectChange((int)eveNOTCONNECTED);
	}
}

/** \brief called if connection timeout
 *
 */
void eveCaTransport::connectTimeout(){

	if (currentAction == eveCONNECT) emit connectChange((int)eveTIMEOUT);
}

/** \brief set connection status
 *
 */
void eveCaTransport::setCnctStatus(int status) {

	transStatus = (eveTransStatusT)status;

	if (transStatus == eveNOTCONNECTED){
		sendError(ERROR, 0, "Connection lost");
		if (currentAction == eveCONNECT) currentAction = eveIDLE;
	}
	else if (transStatus == eveTIMEOUT){
		sendError(ERROR, 0, "Timeout while connecting");
		if (currentAction == eveCONNECT) {
			currentAction = eveIDLE;
			emit done(1);
		}
	}
	else {
		sendError(DEBUG, 0, "connected");
		dataCount = ca_element_count(chanChid);
		elementType = ca_field_type(chanChid);
		// as request type we always use DBR_TIME_<PRIMITIVE TYPE>
		requestType = dbf_type_to_DBR_TIME(elementType);
		if ((elementType == DBF_ENUM) && needEnums){
			if (!enumsInProgress) {
				enumsInProgress = true;
				getEnumStrs();
			}
			return;
		}
		else{
			if (currentAction == eveCONNECT) {
				currentAction = eveIDLE;
				disconnect(this, SLOT(connectTimeout()));
				emit done(0);
			}
		}
	}
}

/** \brief return true, if connected
 */
bool eveCaTransport::isConnected() {
	if (transStatus == eveCONNECTED)
		return true;
	else
		return false;
}

/** \brief fetch EPICS ENUM strings
 */
void eveCaTransport::getEnumStrs(){

	enumData = malloc(dbr_size_n(DBR_CTRL_ENUM, ca_element_count(chanChid)));
	if (enumData==NULL) sendError(ERROR, 0, "eveCaTransport EnumString: Unable to allocate memory");
	int status = ca_array_get_callback(DBR_CTRL_ENUM, ca_element_count(chanChid),
							chanChid, eveCaTransportEnumCB, enumData);
	if (status != ECA_NORMAL) {
		sendError(ERROR, 0, "cannot get enum strings");
	}
	caflush();
}

/** \brief read callback
 * \param arg epics callback structure
 *
 * method is called if read operation is ready; it may be called from a different thread
 */
void eveCaTransport::eveCaTransportEnumCB(struct event_handler_args arg){

	QStringList *enumStringList=NULL;
	char charArray[MAX_ENUM_STRING_SIZE+1];
	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);
	if (arg.type == DBR_CTRL_ENUM) {
		enumStringList = new QStringList;
		struct dbr_ctrl_enum *pValue = (struct dbr_ctrl_enum *) arg.dbr;
		for (int i=0; i < pValue->no_str; ++i){
			// make sure we have a trailing zero
			strncpy(charArray,pValue->strs[i], MAX_ENUM_STRING_SIZE);
			charArray[MAX_ENUM_STRING_SIZE] = 0;
			enumStringList->append(QString::fromAscii(charArray));
		}
	}
	emit pv->enumReady(enumStringList);
}

/** \brief is called if enum data has been received
 *
 *
 */
void eveCaTransport::enumDone(QStringList *stringlist) {

	enumStringList = stringlist;
	needEnums = false;
	enumsInProgress = false;
	if (enumData) free(enumData);
	if (currentAction != eveCONNECT){
		sendError(MINOR, 0, QString("enum callback called, but current action was: %1").arg((int)currentAction));
	}
	if (stringlist == NULL){
		sendError(ERROR, 0, "error reading enums!");
	}
	if (currentAction == eveCONNECT) {
		currentAction = eveIDLE;
		disconnect(this, SLOT(connectTimeout()));
		if (stringlist == NULL)
			emit done(1);
		else
			emit done(0);
	}
}

/** \brief get data without waiting
 */
bool eveCaTransport::getCB(bool flush)
{

	if (transStatus != eveCONNECTED) return false;
	if (currentAction == eveREAD) return true;
	if (currentAction != eveIDLE) return false;
	currentAction = eveREAD;

	if (dataPtr == NULL) {
		int arraysize = dbr_size_n (requestType, dataCount);
		dataPtr = malloc(arraysize);
		if (!dataPtr) sendError(ERROR, 0, "eveCaTransport Unable to allocate memory");
	}

	int status;
	bool retstatus = true;
	status = ca_array_get_callback(requestType, dataCount, chanChid, &eveCaTransport::eveCaTransportGetCB, dataPtr);
	if (status != ECA_NORMAL) {
		sendError(ERROR, 0, QString("eveCaTransport getCB CA-Message: %1").arg(ca_message(status)));
		retstatus = false;
	}
	if (flush) caflush();

	getTimer->start();
	return retstatus;
}

/** \brief read callback
 * \param arg epics callback structure
 *
 * method is called if read operation is ready; it may be called from a different thread
 */
void eveCaTransport::eveCaTransportGetCB(struct event_handler_args arg){

	//void *newdata = arg.usr;
	eveDataMessage *newdata = NULL;;
	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);

	if ((arg.status == ECA_NORMAL) && (arg.count == pv->getElemCnt()) && (arg.type == pv->getRequestType())) {
		newdata = getDataMessage(arg);
	}
	// newdata may be NULL
	emit pv->dataReady(newdata);
}

/** \brief is called if data has been received
 *
 */
void eveCaTransport::readDone(eveDataMessage *data) {

	if (currentAction == eveREAD){
		getTimer->stop();
		currentAction = eveIDLE;
		if (newData != NULL) delete newData;
		newData=data;
		emit done(0);
	}
	else {
		sendError(MINOR, 0, QString("read callback called, but current action was: %1").arg((int)currentAction));
	}
}

/**
 * \brief get the data, may be a pointer to NULL
 * @return eveDataMessage or NULL
 */
eveDataMessage *eveCaTransport::getData(){

	if (newData == NULL)
		return NULL;
	else
		return newData->clone();
};

void eveCaTransport::getTimeout() {

	if (currentAction == eveREAD){
		sendError(ERROR, 0, "read timeout");
		readDone((eveDataMessage *)NULL);
	}
}
/** \brief write data without waiting
 *
 * TODO
 * the call parameters may change
 */
bool eveCaTransport::putCB(eveType datatype, int elemCount, void* data, bool execute){

	if (transStatus != eveCONNECTED) return false;
	if (currentAction == eveWRITE) return true;
	if (currentAction != eveIDLE) return false;
	if (data == NULL) return false;
	if (elemCount > dataCount){
		sendError(ERROR, 0, QString("cannot send array data with arraycount %1, maximum: %2").arg(elemCount).arg(dataCount));
		return false;
	}
	currentAction = eveWRITE;

	int status;
	bool retstatus = true;

	status = ca_array_put_callback(convertEpicsToDBR(convertEveToEpicsType(datatype)), elemCount,
							chanChid, data, &eveCaTransport::eveCaTransportPutCB, NULL);
	if (status != ECA_NORMAL) {
		sendError(ERROR,0,QString("eveCaTransport putCB CA-Message: %1").arg(ca_message(status)));
		retstatus = false;
	}
	if (execute) caflush();
	putTimer->start();
	return retstatus;
}

/** \brief write callback
 * \param arg epics callback structure
 *
 * method is called if write operation is ready; it may be called from a different thread
 *
 */
void eveCaTransport::eveCaTransportPutCB(struct event_handler_args arg){

	int status=1;
	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);
	if ( arg.status == ECA_NORMAL ) status = 0;
	emit pv->writeReady(status);
}

/** \brief is called by write callback
 *
 * \param status
 */
void eveCaTransport::writeDone(int status) {

	if (currentAction == eveWRITE){
		putTimer->stop();
		emit done(status);
		currentAction = eveIDLE;
	}
	else {
		sendError(MINOR, 0, QString("write callback called, but current action was: %1").arg((int)currentAction));
	}
}

/**
 * slot is called in case of caput timeout
 */
void eveCaTransport::putTimeout() {

	if (currentAction == eveWRITE){
		sendError(ERROR, 0, "write timeout");
		writeDone(1);
	}
}

/** \brief write data without Callback
 */
bool eveCaTransport::put(eveType datatype, int elemCount, void* data, bool execute){

	if (transStatus != eveCONNECTED) return false;
	if (data == NULL) return false;
	if (elemCount > dataCount){
		sendError(ERROR, 0, QString("cannot send array data with arraycount %1, maximum: %2").arg(elemCount).arg(dataCount));
		return false;
	}

	int status;
	bool retstatus = true;

	status = ca_array_put(convertEpicsToDBR(convertEveToEpicsType(datatype)), elemCount, chanChid, data);
	if (status != ECA_NORMAL) {
		sendError(ERROR, 0, QString("eveCaTransport put ca-message: %1").arg(ca_message(status)));
		retstatus = false;
	}
	if (execute) caflush();
	return retstatus;
}

void eveCaTransport::caflush(){
	ca_flush_io();
}

/** \brief return EPICS ENUM String or empty string if index is out of bounds
 * \param index number of string to return
*/
QString eveCaTransport::getEnumString(int index) {
	if (index >= enumStringList->size())
		return QString();
	else
		return enumStringList->at(index);
}

void eveCaTransport::sendError(int severity, int errorType,  QString message){

	baseDevice->sendError(severity, EVEMESSAGEFACILITY_CATRANSPORT, errorType,  QString("PV %1: %2").arg(pvname).arg(message));
}

/**
 * @param queue true, if request should be queued (needs execQueue() to actually start reading)
 * @return 0 if successful
 *
 * signals done if ready
 * ( we don't do calls to caget(), but use caget_callback() )
 */
int eveCaTransport::readData(bool queue){

	if ((method == eveGET) || (method == eveGETPUT)){
		sendError(INFO, 0, "eveCaTransport::readData GET not supported, using GETCB");
	}
	else if ((method == evePUT) || (method == evePUTCB)){
		sendError(ERROR, 0, "eveCaTransport::readData unable to read from write-only variable");
		return 1;
	}

	if (getCB(!queue))
		return 0;
	else
		return 1;
}

/**
 * @param queue true, if request should be queued (needs execQueue() to actually start writing)
 * @return 0 if ok
 *
 * signals done if ready
 */
int eveCaTransport::writeData(eveVariant writedata, bool queue){

	bool retstat;
	char *strPtr;

	if (writeDataPtr == NULL) {
		// TODO
		// this can be more specific, enhance for arrays
		writeDataPtr = malloc(MAX_STRING_SIZE+1);
		if (!writeDataPtr) {
			sendError(ERROR,0,QString("eveCaTransport::writeData Unable to allocate memory"));
			return 1;
		}
	}
	if (writedata.getType() != dataType){
		sendError(ERROR, 0, QString("eveCaTransport: datatype mismatch, %1 <-> %2").arg((int)writedata.getType()).arg((int)dataType));
		// TODO try to convert and proceed
		return 1;
	}

	if (dataType == eveINT){
		*((int*)writeDataPtr) = writedata.toInt();
	}
	else if (dataType == eveDOUBLE){
		*((double*)writeDataPtr) = writedata.toDouble();
	}
	else if (dataType == eveSTRING){
		strPtr = (char*) writeDataPtr;
		strncpy(strPtr, writedata.toString().toAscii().data(), MAX_STRING_SIZE);
		strPtr[MAX_STRING_SIZE] = 0;
	}

	// TODO
	// by now, no array data is allowed (elemCount = 1)
	if ((method == evePUT) || (method == eveGETPUT)){
		retstat = put(dataType, 1, writeDataPtr, !queue);
		// we signal immediately
		if (retstat)emit done(0);
	}
	else
		retstat = putCB(dataType, 1, writeDataPtr, !queue);

	if (retstat)
		return 0;
	else
		return 1;
}

int eveCaTransport::monitorTrans(){

	int status=0;
	if (isMonitorOn) return status;
	isMonitorOn = true;
	if (transStatus == eveUNDEFINED) {
		connect (this, SIGNAL(done(int)), this, SLOT(createMonitor(int)));
		status = connectTrans();
	}
	else {
		createMonitor(0);
	}
	return status;
}

void eveCaTransport::createMonitor(int cstatus){

	int status = 0;
	disconnect (this, SIGNAL(done(int)), this, SLOT(createMonitor(int)));
	if ((cstatus != 0) || (transStatus != eveCONNECTED)){
		sendError(ERROR, 0, "eveCaTransport::createMonitor Monitor not connected");
		return;
	}

	if (monitorDataPtr == NULL) {
		int arraysize = dbr_size_n(requestType, dataCount);
		monitorDataPtr = malloc(arraysize);
		if (!monitorDataPtr) {
			sendError(ERROR, 0, "eveCaTransport::createMonitor Unable to allocate memory");
			return;
		}
	}

	status = ca_create_subscription(requestType, dataCount, chanChid, DBE_VALUE, &eveCaTransport::eveCaTransportMonitorCB, monitorDataPtr, NULL );

	if (status != ECA_NORMAL)
		sendError(ERROR, 0, QString("eveCaTransport createMonitor CA-Message: %1").arg(ca_message(status)));

	caflush();
}

void eveCaTransport::eveCaTransportMonitorCB(struct event_handler_args arg){

	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);
	epicsAlarmSeverity severity = (epicsAlarmSeverity)((struct dbr_time_short *)arg.dbr)->severity;

	if ((arg.count == pv->getElemCnt()) && (arg.type == pv->getRequestType())) {
		emit pv->valueChanged(getDataMessage(arg));
	}
}

eveDataMessage* eveCaTransport::getDataMessage(struct event_handler_args arg){

	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);
	epicsAlarmCondition status = (epicsAlarmCondition)((struct dbr_time_short *)arg.dbr)->status;
	epicsAlarmSeverity severity = (epicsAlarmSeverity)((struct dbr_time_short *)arg.dbr)->severity;
	QString name = pv->getName();
	QString xmlId = pv->getXmlId();
	epicsTimeStamp ets = ((struct dbr_time_short *)arg.dbr)->stamp;
	eveDataStatus dStatus((quint8) severity, (quint8) status, (quint16) 0);
	eveDataModType dataMod = DMTunmodified;
	epicsTime etime = ets;
	eveDataMessage *newdata = NULL;

	if (arg.type == DBR_TIME_LONG){
		QVector<int> dataArray(arg.count);
		memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_long *)arg.dbr)->value, sizeof(int) * arg.count);
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, dataArray);
	}
	else if (arg.type == DBR_TIME_SHORT){
		QVector<short> dataArray(arg.count);
		memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_short *)arg.dbr)->value, sizeof(short) * arg.count);
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, dataArray);
	}
	else if (arg.type == DBR_TIME_CHAR){
		QVector<signed char> dataArray(arg.count);
		memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_char *)arg.dbr)->value, sizeof(char) * arg.count);
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, dataArray);
	}
	else if (arg.type == DBR_TIME_FLOAT){
		QVector<float> dataArray(arg.count);
		memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_float *)arg.dbr)->value, sizeof(float) * arg.count);
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, dataArray);
	}
	else if (arg.type == DBR_TIME_DOUBLE){
		QVector<double> dataArray(arg.count);
		memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_double *)arg.dbr)->value, sizeof(double) * arg.count);
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, dataArray);
	}
	else if (arg.type == DBR_TIME_ENUM){
		QStringList qsl;
		QVector<short> dataArray(arg.count);
		memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_short *)arg.dbr)->value, sizeof(short) * arg.count);
		foreach(short index, dataArray){
			qsl.insert(index, pv->getEnumString(index));
		}
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, qsl);
	}
	else if (arg.type == DBR_TIME_STRING){
		char buffer[MAX_STRING_SIZE+1];
		QStringList qsl;
		char *mystr = ((struct dbr_time_string *)arg.dbr)->value;
		buffer[MAX_STRING_SIZE]=0;
		for (int i=0; i<arg.count; ++i){
			strncpy(buffer, mystr, MAX_STRING_SIZE);
			qsl.insert(i, QString(buffer));
			mystr += MAX_STRING_SIZE;
		}
		newdata = new eveDataMessage(xmlId, name, dStatus, dataMod, etime, qsl);
	}
	return newdata;
}

/**
 *
 * @return a QStringList pointer which contains the pv name "PV: <value>"
 */
QStringList* eveCaTransport::getInfo(){
	QStringList *sl = new QStringList();
	sl->append(QString("Access:ca:%1").arg(pvname));
	return sl;
}
/**
 * start executing previously queued commands
 */
int eveCaTransport::execQueue(){

	QWriteLocker locker(&contextLock);
	if (contextCounter.contains(ca_current_context())){
		eveError::log(DEBUG, "CaTransport: context found, flushing CA");
		eveCaTransport::caflush();
	}
	else {
		eveError::log(DEBUG, "CaTransport: no context found, not flushing CA");
	}

	return 0;
}
