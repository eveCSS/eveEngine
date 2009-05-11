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
#include "eveScanManager.h"

/**
 * \brief constructor connects to EPICS PVs
 * \param pvname EPICS PV name
 * \param wait if true, wait until connected or timeout (5 secs)
 * \param parent parent
 */
eveCaTransport::eveCaTransport(eveCaTransportDef* transdef)
{

	transStatus = eveUNDEFINED;
	currentAction = eveIDLE;
	transDef=transdef;
	needEnums = true;
	enumsInProgress = false;
	chanChid = 0;
	dataPtr = NULL;
	writeDataPtr = NULL;
	name = transDef->getName();
	scanManager = NULL; 	// TODO set this to send errors
	timeOut = 1000;			// TODO get timeout from transdef, keep a low limit of 1 s

	connect (this, SIGNAL(connectChange(int)), this, SLOT(setCnctStatus(int)), Qt::QueuedConnection);
	connect (this, SIGNAL(dataReady(eveDataMessage *)), this, SLOT(readDone(eveDataMessage *)), Qt::QueuedConnection);
	connect (this, SIGNAL(writeReady(int)), this, SLOT(writeDone(int)), Qt::QueuedConnection);
	connect (this, SIGNAL(enumReady(QStringList *)), this, SLOT(enumDone(QStringList *)), Qt::QueuedConnection);

}

eveCaTransport::~eveCaTransport(){

	ca_clear_channel(chanChid);
	caflush();
}

/**
 * connect the transport
 * signals done if ready
 */
int eveCaTransport::connectTrans(){

	int status;
	int retstat=0;

	struct ca_client_context *cacontext = ca_current_context();
	if (cacontext == NULL) {
		sendError(INFO, 0, QString("creating CA Context %1").arg(name));
		status = ca_context_create (ca_enable_preemptive_callback);
		if (status != ECA_NORMAL) sendError(ERROR, 0, QString("Error creating CA Context %1").arg(name));
		cacontext = ca_current_context();
	}
	if (transStatus != eveUNDEFINED) return false;
	currentAction = eveCONNECT;

	status=ca_create_channel(name.toAscii().data(), &eveCaTransportConnectCB, (void*) this, 0, &chanChid);
	if(status != ECA_NORMAL){
		sendError(ERROR, 0, QString("Error creating CA Channel %1").arg(name));
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

	printf("called ConnectCB\n");
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
		sendError(ERROR, 0, QString("Connection lost: %1").arg(name));
		if (currentAction == eveCONNECT) currentAction = eveIDLE;
	}
	else if (transStatus == eveTIMEOUT){
		sendError(ERROR, 0, QString("Timeout while connecting to: %1").arg(name));
		if (currentAction == eveCONNECT) {
			currentAction = eveIDLE;
			emit done(1);
		}
	}
	else {
		sendError(INFO, 0, QString("connected to: %1").arg(name));
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
	if (enumData==NULL) sendError(ERROR,0,QString("eveCaTransport EnumString: Unable to allocate memory"));
	int status = ca_array_get_callback(DBR_CTRL_ENUM, ca_element_count(chanChid),
							chanChid, eveCaTransportEnumCB, enumData);
	if (status != ECA_NORMAL) {
		sendError(ERROR,0,"cannot get Enum Strings");
	}
	caflush();
}

/** \brief read callback
 * \param arg epics callback structure
 *
 * method is called if read operation is ready; it may be called from a different thread
 */
void eveCaTransport::eveCaTransportEnumCB(struct event_handler_args arg){

	QStringList *enumStringList=NULL;;
	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);
	if (arg.type == DBR_CTRL_ENUM) {
		enumStringList = new QStringList;
		struct dbr_ctrl_enum *pValue = (struct dbr_ctrl_enum *) arg.dbr;
		for (int i=0; i < pValue->no_str; ++i){
			enumStringList->append(QString::fromAscii(pValue->strs[i],MAX_ENUM_STRING_SIZE));
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
		sendError(MINOR, 0, QString("%1: Enum Callback called, but current action was: %2").arg(name).arg((int)currentAction));
	}
	if (stringlist == NULL){
		sendError(ERROR, 0, QString("%1: Error Reading Enums!").arg(name));
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


/** \brief get data and wait until done (deprecated, use getCB)
*
bool eveCaTransport::get(eveData* data){

	int status;
	// check if connected
	if (transStatus != CONNECTED) return false;
	if (data == NULL) return false;

	ca_get(data->getRequestType(), chanChid, data->getPtr());
	status=ca_pend_io(((double)readTimeout)/1000.0);
	if (status != ECA_NORMAL) {
		sendError(ERROR,0,QString("eveCaTransport get: %1, CA-Message: %2").arg(name).arg(ca_message(status)));
		return false;
	}
	return true;
}
 */

/** \brief get data without waiting
 */
bool eveCaTransport::getCB(bool flush)
{

	if (transStatus != eveCONNECTED) return false;
	if (currentAction != eveIDLE) return false;
	currentAction = eveREAD;

	if (dataPtr == NULL) {
		int arraysize = dbr_size_n (requestType, dataCount);
		dataPtr = malloc(arraysize);
		if (!dataPtr) sendError(ERROR,0,QString("eveCaTransport Unable to allocate memory"));
	}

	int status;
	bool retstatus = true;
	printf("calling getCb (ca_get_callback)\n");
	status = ca_array_get_callback(requestType, dataCount, chanChid, &eveCaTransport::eveCaTransportGetCB, dataPtr);
	if (status != ECA_NORMAL) {
		sendError(ERROR,0,QString("eveCaTransport getCB: %1, CA-Message: %2").arg(name).arg(ca_message(status)));
		retstatus = false;
	}
	if (flush) caflush();
	//ca_pend_io(0.0);

	QTimer::singleShot(timeOut, this, SLOT(getTimeout()));
	return retstatus;
}

/** \brief read callback
 * \param arg epics callback structure
 *
 * method is called if read operation is ready; it may be called from a different thread
 */
void eveCaTransport::eveCaTransportGetCB(struct event_handler_args arg){

	//void *newdata = arg.usr;
	eveDataMessage *newdata;
	eveCaTransport *pv = (eveCaTransport *) ca_puser(arg.chid);

	printf("getCb fired: status: %d\n", arg.status);
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
			QVector<char> dataArray(arg.count);
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
			newdata = new eveDataMessage(id, dStatus, dataMod, etime, dataArray);
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
		else {
			newdata = NULL;
		}
	}
   	else {
		newdata = NULL;
   	}
	emit pv->dataReady(newdata);
}

/** \brief is called if data has been received
 *
 *
 */
void eveCaTransport::readDone(eveDataMessage *data) {

	setData(data);
	if (currentAction == eveREAD){
		currentAction = eveIDLE;
		disconnect(this, SLOT(getTimeout()));
		if (data == NULL)
			emit done(1);
		else
			emit done(0);
	}
	else {
		sendError(MINOR, 0, QString("%1: Read Callback called, but current action was: %2").arg(name).arg((int)currentAction));
	}
}

void eveCaTransport::getTimeout() {

	if (currentAction == eveREAD){
		sendError(ERROR, 0, "Read Timeout");
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
		sendError(ERROR,0,QString("Cannot send array data with arraycount %1, maximum: %2").arg(elemCount).arg(dataCount));
		return false;
	}
	currentAction = eveWRITE;

	int status;
	bool retstatus = true;

	status = ca_array_put_callback(convertEpicsToDBR(convertEveToEpicsType(datatype)), elemCount,
							chanChid, data, &eveCaTransport::eveCaTransportPutCB, NULL);
	if (status != ECA_NORMAL) {
		sendError(ERROR,0,QString("eveCaTransport putCB: %1, CA-Message: %2").arg(name).arg(ca_message(status)));
		retstatus = false;
	}
	if (execute) caflush();
	int timeout = 10000;
	QTimer::singleShot(timeout, this, SLOT(putTimeout()));
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
	printf("leaving PutCB-Callback\n");
}

/** \brief is called by write callback
 *
 *
 */
void eveCaTransport::writeDone(int status) {

	if (currentAction == eveWRITE){
		disconnect(this, SLOT(putTimeout()));
		emit done(status);
		currentAction = eveIDLE;
	}
	else {
		sendError(MINOR, 0, QString("%1: Write Callback called, but current action was: %2").arg(name).arg((int)currentAction));
	}
}

void eveCaTransport::putTimeout() {

	if (currentAction == eveWRITE){
		sendError(ERROR, 0, "Write Timeout");
		writeDone(1);
	}
}


/** \brief write data without Callback
 */
bool eveCaTransport::put(eveType datatype, int elemCount, void* data, bool execute){

	if (transStatus != eveCONNECTED) return false;
	if (data == NULL) return false;
	if (elemCount > dataCount){
		sendError(ERROR,0,QString("Cannot send array data with arraycount %1, maximum: %2").arg(elemCount).arg(dataCount));
		return false;
	}

	int status;
	bool retstatus = true;

	status = ca_array_put(convertEpicsToDBR(convertEveToEpicsType(datatype)), elemCount, chanChid, data);
	if (status != ECA_NORMAL) {
		sendError(ERROR,0,QString("eveCaTransport put: %1, CA-Message: %2").arg(name).arg(ca_message(status)));
		retstatus = false;
	}
	if (execute) caflush();
	return retstatus;
}

void eveCaTransport::caflush(){
	printf("flushing ...\n");
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
	eveError::log(severity, QString("PV %1: %2").arg(name).arg(message));
	if (scanManager != NULL)
		scanManager->sendError(severity, EVEMESSAGEFACILITY_CATRANSPORT, errorType,  QString("PV %1: %2").arg(name).arg(message));
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
		if (!writeDataPtr) sendError(ERROR,0,QString("eveCaTransport::writeData Unable to allocate memory"));
	}
	if (writedata.getType() != transDef->getDataType()){
		sendError(ERROR,0,QString("eveCaTransport::Datatype mismatch, check XML-File"));
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
	if ((transDef->getMethod() == evePUT) || (transDef->getMethod() == eveGETPUT))
		retstat = put(transDef->getDataType(), 1, writeDataPtr, !queue);
	else
		retstat = putCB(transDef->getDataType(), 1, writeDataPtr, !queue);
	if (retstat)
		return 0;
	else
		return 1;
}

/**
 * start executing previous queued commands
 */
int eveCaTransport::execQueue(){
	eveCaTransport::caflush();
	return 0;
}
