/*
 * eveCaTransport.cpp
 *
 *  Created on: 13.01.2009
 *      Author: eden
 */

#include <QTimer>
// TODO remove QThread
#include <QThread>
#include "eveMessage.h"
#include "eveCaTransport.h"
#include "eveError.h"
#include "db_access.h"
//#include "eveScanManager.h"

QHash<struct ca_client_context *, int> eveCaTransport::contextCounter = QHash<struct ca_client_context *, int>();


/**
 * \brief init connection to EPICS PV
 * \param transdef definition of EPICS PV related properties
 * \param parent
 */
eveCaTransport::eveCaTransport(QObject *parent, QString devname, eveCaTransportDef* transdef) : eveBaseTransport(parent)
{

	transStatus = eveUNDEFINED;
	currentAction = eveIDLE;
	transDef=transdef;
	needEnums = true;
	enumsInProgress = false;
	chanChid = 0;
	dataPtr = NULL;
	writeDataPtr = NULL;
	name = devname;
	newData = NULL;
	pvname = transDef->getName();
	//scanManager = NULL; 	// TODO set this to send errors
	timeOut = (int)(transDef->getTimeout()*1000.0);			// TODO get timeout from transdef, keep a low limit of 1 s

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

	ca_clear_channel(chanChid);
	caflush();
	if (contextCounter.contains(caThreadContext)){
		int tmp = contextCounter.value(caThreadContext);
		--tmp;
		sendError(DEBUG, 0, QString("CATransport Destr.: %1 (%2) channels remaining").arg(tmp).arg((int)caThreadContext));
		if (tmp > 0){
			contextCounter.insert(caThreadContext, tmp);
		}
		else {
			sendError(DEBUG, 0, "destroy CA Context");
			//ca_context_destroy();
			contextCounter.remove(caThreadContext);
		}
	}


}

/**
 * \brief connect the transport, signals done if ready
 */
int eveCaTransport::connectTrans(){

	int status;
	int retstat=0;

	caThreadContext = ca_current_context();
	if (caThreadContext == NULL) {
		sendError(INFO, 0, "creating CA Context");
		status = ca_context_create (ca_enable_preemptive_callback);
		caThreadContext = ca_current_context();
		if (status != ECA_NORMAL) sendError(ERROR, 0, "Error creating CA Context");
		contextCounter.insert(caThreadContext, 0);
//		caThreadContext = ca_current_context();
//		printf("Assigned context: %d\n",caThreadContext);
	}
	int tmp = contextCounter.value(caThreadContext);
	contextCounter.insert(caThreadContext, ++tmp);
	sendError(DEBUG, 0, QString("CaTransport ContextCount: %1 (%2)").arg(tmp).arg((int)caThreadContext));

//	printf("Thread context: %d\n",caThreadContext);
//	printf("Thread id: %d\n",QThread::currentThread());
	if (transStatus != eveUNDEFINED) return false;
	currentAction = eveCONNECT;

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
		sendError(INFO, 0, "connected");
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
	if (currentAction != eveIDLE) return false;
	currentAction = eveREAD;

	// TODO remove, should not happen
//	if (caThreadContext != ca_current_context()) {
//		printf("Thread context: %d (getCB)\n",caThreadContext);
//		printf("Current context: %d (getCB)\n",ca_current_context());
//		printf("Thread id: %d\n",QThread::currentThread());
//		int status = ca_attach_context(caThreadContext);
//		if (status == ECA_ISATTACHED) printf("getCB context was already attached\n");
//		else if (status == ECA_NOTTHREADED) printf("getCB context not threaded\n");
//	}

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

	if ((arg.status == ECA_NORMAL) && (arg.count == pv->getElemCnt())
									&& (arg.type == pv->getRequestType())) {
		epicsAlarmCondition status;
		epicsAlarmSeverity severity;		// epics severity: NO_ALARM MINOR_ALARM MAJOR_ALARM INVALID_ALARM
		epicsTimeStamp ets;
		QString id = pv->getName();
		status = (epicsAlarmCondition)((struct dbr_time_short *)arg.dbr)->status;
		severity = (epicsAlarmSeverity)((struct dbr_time_short *)arg.dbr)->severity;
		ets = ((struct dbr_time_short *)arg.dbr)->stamp;
		eveDataStatus dStatus;
		dStatus.condition = (quint8) status;
		dStatus.severity = (quint8) severity;
		dStatus.acqStatus = 1;
		eveDataModType dataMod = DMTunmodified;
		epicsTime etime = ets;
		if (arg.type == DBR_TIME_LONG){
			QVector<int> dataArray(arg.count);
			memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_long *)arg.dbr)->value, sizeof(int)*arg.count);
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, dataArray);
		}
		else if (arg.type == DBR_TIME_SHORT){
			QVector<short> dataArray(arg.count);
			memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_short *)arg.dbr)->value, sizeof(short)*arg.count);
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, dataArray);
		}
		else if (arg.type == DBR_TIME_CHAR){
			QVector<signed char> dataArray(arg.count);
			memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_char *)arg.dbr)->value, sizeof(char)*arg.count);
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, dataArray);
		}
		else if (arg.type == DBR_TIME_FLOAT){
			QVector<float> dataArray(arg.count);
			memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_float *)arg.dbr)->value, sizeof(float)*arg.count);
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, dataArray);
		}
		else if (arg.type == DBR_TIME_DOUBLE){
			QVector<double> dataArray(arg.count);
			memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_double *)arg.dbr)->value, sizeof(double)*arg.count);
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, dataArray);
		}
		else if (arg.type == DBR_TIME_ENUM){
			QStringList qsl;
			QVector<short> dataArray(arg.count);
			memcpy((void *)dataArray.data(), (const void *) &((struct dbr_time_short *)arg.dbr)->value, sizeof(short)*arg.count);
			foreach(short index, dataArray){
				qsl.insert(index, pv->getEnumString(index));
			}
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, qsl);
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
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, qsl);
		}
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
	eveDataMessage *return_data = newData;
	newData = NULL;
	return return_data;
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
	if (currentAction != eveIDLE) return false;
	if (data == NULL) return false;
	if (elemCount > dataCount){
		sendError(ERROR, 0, QString("cannot send array data with arraycount %1, maximum: %2").arg(elemCount).arg(dataCount));
		return false;
	}
	currentAction = eveWRITE;

	int status;
	bool retstatus = true;

	// TODO remove, this should never happen
//	if (caThreadContext != ca_current_context()) {
//		printf("putCB CA context mismatch\n");
//		int status = ca_attach_context(caThreadContext);
//		if (status == ECA_ISATTACHED) printf("putCB context was already attached\n");
//		else if (status == ECA_NOTTHREADED) printf("putCB context not threaded\n");
//	}

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

//	if (caThreadContext != ca_current_context()) {
//		int status = ca_attach_context(caThreadContext);
//		if (status == ECA_ISATTACHED) printf("getCB context was already attached\n");
//		else if (status == ECA_NOTTHREADED) printf("getCB context not threaded\n");
//	}

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
	if (index > enumStringList->size())
		return QString();
	else
		return enumStringList->at(index);
}

void eveCaTransport::sendError(int severity, int errorType,  QString message){

	// for now we write output to local console too
	eveError::log(severity, QString("PV %1(%2): %3").arg(name).arg(pvname).arg(message));
	printf("CaTranport: %d, %s\n", severity, qPrintable(QString("PV %1(%2): %3").arg(name).arg(pvname).arg(message)));
//	if (scanManager != NULL)
//		scanManager->sendError(severity, EVEMESSAGEFACILITY_CATRANSPORT, errorType,  QString("PV %1: %2").arg(pvname).arg(message));
}

/**
 * @param queue true, if request should be queued (needs execQueue() to actually start reading)
 *
 * signals done if ready
 * ( we don't do calls to caget(), but use caget_callback() )
 */
int eveCaTransport::readData(bool queue){
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
	if (writedata.getType() != transDef->getDataType()){
		sendError(ERROR, 0, QString("eveCaTransport: datatype mismatch, %1 <-> %2").arg((int)writedata.getType()).arg((int)transDef->getDataType()));
		// TODO try to convert and proceed
		return 1;
	}

	if (transDef->getDataType() == eveINT){
		*((int*)writeDataPtr) = writedata.toInt();
	}
	else if (transDef->getDataType() == eveDOUBLE){
		*((double*)writeDataPtr) = writedata.toDouble();
	}
	else if (transDef->getDataType() == eveSTRING){
		strPtr = (char*) writeDataPtr;
		strncpy(strPtr, writedata.toString().toAscii().data(), MAX_STRING_SIZE);
		strPtr[MAX_STRING_SIZE] = 0;
	}

	// TODO
	// by now, no array data is allowed (elemCount = 1)
	if ((transDef->getMethod() == evePUT) || (transDef->getMethod() == eveGETPUT)){
		retstat = put(transDef->getDataType(), 1, writeDataPtr, !queue);
		// we signal immediately
		if (retstat)emit done(0);
	}
	else
		retstat = putCB(transDef->getDataType(), 1, writeDataPtr, !queue);

	if (retstat)
		return 0;
	else
		return 1;
}

/**
 *
 * @return a QStringHash with PV=Value
 */
QStringList* eveCaTransport::getInfo(){
	QStringList *sl = new QStringList();
	sl->append(QString("PV: %1").arg(pvname));
	return sl;
}
/**
 * start executing previously queued commands
 */
int eveCaTransport::execQueue(){
	eveCaTransport::caflush();
	return 0;
}
