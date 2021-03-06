
#include <iostream>
#include <assert.h>
#include <cmath>
#include <QChar>
#include "eveMessage.h"
#include "eveError.h"
#include "eveStartTime.h"
#include "eveChainStatus.h"
#include "eveParameter.h"

eveMessage::eveMessage()
{
	type = 0;
	priority = EVEMESSAGEPRIO_NORMAL;
	destination = 0;
    facility = 0;
}

/**
 * \brief Constructor
 * \param mtype the messagetype definition
 */
eveMessage::eveMessage(int mtype, int prio, int dest)
{
	priority = prio;
	destination = dest;
	type = mtype;
    facility = 0;
    // TODO remove this assertion
	assert ((type == EVEMESSAGETYPE_START) ||
			(type == EVEMESSAGETYPE_HALT) ||
			(type == EVEMESSAGETYPE_STORAGECONFIG) ||
            (type == EVEMESSAGETYPE_CHAINPROGRESS) ||
			(type == EVEMESSAGETYPE_ENGINESTATUS) ||
			(type == EVEMESSAGETYPE_DATA) ||
			(type == EVEMESSAGETYPE_BASEDATA) ||
			(type == EVEMESSAGETYPE_BREAK) ||
			(type == EVEMESSAGETYPE_STOP) ||
			(type == EVEMESSAGETYPE_PAUSE) ||
			(type == EVEMESSAGETYPE_AUTOPLAY) ||
            (type == EVEMESSAGETYPE_REPEATCOUNT) ||
            (type == EVEMESSAGETYPE_ENDPROGRAM) ||
			(type == EVEMESSAGETYPE_DEVINFO) ||
			(type == EVEMESSAGETYPE_EVENTREGISTER) ||
			(type == EVEMESSAGETYPE_LIVEDESCRIPTION) ||
			(type == EVEMESSAGETYPE_METADATA) ||
			(type == EVEMESSAGETYPE_REMOVEFROMPLAYLIST) ||
			(type == EVEMESSAGETYPE_STORAGEDONE) ||
			(type == EVEMESSAGETYPE_DETECTORREADY) ||
            (type == EVEMESSAGETYPE_CHAINSTATUS) ||
            (type == EVEMESSAGETYPE_VERSION) ||
            (type == EVEMESSAGETYPE_MONITORREGISTER));
}

eveMessage::~eveMessage()
{
}

/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveMessage::compare(eveMessage *message)
{
	if ((message->getType() == type) && (message->getPriority() == priority)
                && (message->getDestinationChannel() == destination)) return true;
	return false;
}
void eveMessage::dump()
{
	std::cout << "eveMessage: destination: " << destination << " type: "
			<< type << " prio: "<< priority << "\n";

}


/**
 * \param mtype the type of message
 * \param text message text
 */
eveMessageText::eveMessageText(int mType, QString text, int prio) : eveMessage(mType, prio), messageText(text)
{
	type = mType;
	assert((type == EVEMESSAGETYPE_DETECTORREADY) || (type == EVEMESSAGETYPE_LIVEDESCRIPTION));
	priority = prio;
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveMessageText::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (messageText != ((eveMessageText*)message)->getText()) return false;
	return true;
}
/**
 * \brief dump content of eveMessageText to stdout
 */
void eveMessageText::dump()
{
	((eveMessage*)this)->dump();
	std::cout << "eveMessageText:  messageText: " << qPrintable(messageText) << "\n";
}



/**
 * \param mtype type of message
 * \param textlist list of messages
 * \param prio message priority
 */
eveMessageTextList::eveMessageTextList(int mType, QStringList& textlist, int prio) : eveMessage(mType, prio), messageTextList(textlist)
{
	type = mType;
	// check the allowed types; for now EVEMESSAGETYPE_METADATA is the only candidate
	assert(type == EVEMESSAGETYPE_METADATA);
	priority = prio;
    chainId = 0;
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveMessageTextList::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	return false;
	// neither implemented nor used
	// return true;
}


/**
 * \brief Constructor of message holding an int
 * \param mtype the type of message
 * \param ival integer value of the message
 */
eveMessageInt::eveMessageInt(int iType, int ival, int prio, int dest) :
	eveMessage(iType, prio, dest)
{
	value = ival;
	// check the allowed types
    assert ((type == EVEMESSAGETYPE_AUTOPLAY) ||
            (type == EVEMESSAGETYPE_REPEATCOUNT) ||
            (type == EVEMESSAGETYPE_STORAGEDONE) ||
            (type == EVEMESSAGETYPE_REMOVEFROMPLAYLIST));
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveMessageInt::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (value != ((eveMessageInt*)message)->getInt()) return false;
	return true;
}

/*
 * Class eveMessageIntList
 */
/**
 * \param mtype the type of message
 * \param ival integer value of the message
 */
eveMessageIntList::eveMessageIntList(int mtype, int ival1, int ival2, int prio)
{
	priority = prio;
	ivalue1 = ival1;
	ivalue2 = ival2;
	type = mtype;
	// check the allowed types
	assert (type == EVEMESSAGETYPE_REORDERPLAYLIST);
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveMessageIntList::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (ivalue1 != ((eveMessageIntList*)message)->getInt(0)) return false;
	if (ivalue2 != ((eveMessageIntList*)message)->getInt(1)) return false;
	return true;
}
/**
 * \brief get one integer value from array
 * \param index	index of desired value (0 or 1)
 * \return the desired value or 0 if index is out of range
 */
int eveMessageIntList::getInt(int index){
	if (index == 0)	return ivalue1;
	else if (index == 1)	return ivalue2;
	else {
		eveError::log(ERROR,"eveMessageIntList::getInt: index out of range");
		return 0;
	}
}

/**
 * @brief eveVersionMessage::eveVersionMessage holds version, revision, patchlevel of evEngine
 */
eveVersionMessage::eveVersionMessage() :
    eveMessage(EVEMESSAGETYPE_VERSION, 0, 0)
{
    version = 0;
    revision = 0;
    patch = 0;
    QString appversion = eveParameter::getParameter("version");
    QStringList appversionStrings = appversion.split(".");
    if (appversionStrings.count() > 1){
        version = appversionStrings[0].toUInt();
        revision = appversionStrings[1].toUInt();
        if (appversionStrings.count() > 2) patch = appversionStrings[2].toUInt();
    }
}

/**
 * @brief eveVersionMessage::eveVersionMessage holds version, revision, patchlevel of evEngine
 */
eveVersionMessage::eveVersionMessage(quint32 vers, quint32 rev, quint32 plevel) :
    eveMessage(EVEMESSAGETYPE_VERSION, 0, 0)
{
    version = vers;
    revision = rev;
    patch = plevel;
}

/*
 * Class eveAddToPlMessage
 */
/**
 * \param xmlname Name of xml-description (usually the filename)
 * \param xmlauthor Host and Author in the format 'author@host'
 * \param xmldata byte array of xml-data
 */
eveAddToPlMessage::eveAddToPlMessage(QString xmlname, QString xmlauthor, QByteArray xmldata, int prio)
{
	priority = prio;
	type = EVEMESSAGETYPE_ADDTOPLAYLIST;
	// tbd. use try/catch block
	XmlName = QString(xmlname);
	XmlAuthor = QString(xmlauthor);
	XmlData = QByteArray(xmldata);
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveAddToPlMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (XmlName != ((eveAddToPlMessage*)message)->getXmlName()) return false;
	if (XmlAuthor != ((eveAddToPlMessage*)message)->getXmlAuthor()) return false;
	if (XmlData != ((eveAddToPlMessage*)message)->getXmlData()) return false;

	return true;
}
eveAddToPlMessage::~eveAddToPlMessage()
{
}

/*
 * Class eveCurrentXmlMessage
 */
/**
 * \param xmlname Name of xml-description (usually the filename)
 * \param xmlauthor Host and Author in the format author@host
 * \param xmldata xml-formatted description
 */
eveCurrentXmlMessage::eveCurrentXmlMessage(QString xmlname, QString xmlauthor, QByteArray xmldata, int prio)
	:  eveAddToPlMessage(xmlname, xmlauthor, xmldata)
{
	type = EVEMESSAGETYPE_CURRENTXML;
	priority = prio;
}

eveCurrentXmlMessage::~eveCurrentXmlMessage()
{
}

/*
 * Class eveEngineStatusMessage
 */
/**
 * \param xmlname Name of currently processing scanmodule
 * \param status status of currently processing scanmodule
 */
eveEngineStatusMessage::eveEngineStatusMessage(unsigned int status, QString xmlname, int prio, int dest) :
	eveMessage(EVEMESSAGETYPE_ENGINESTATUS, prio, dest)
{
    XmlId = xmlname;
	estatus = status;
	timestamp = eveTime::getCurrent();
}
eveEngineStatusMessage::~eveEngineStatusMessage(){
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveEngineStatusMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
    if (estatus != ((eveEngineStatusMessage*)message)->getStatus()) return false;
    if ((XmlId ==  ((eveEngineStatusMessage*)message)->getXmlId()) &&
    		(timestamp == ((eveEngineStatusMessage*)message)->getTime())) return true;

	return false;
}

/*
 * Class eveChainProgressMessage
 */
/**
 * \param chid  chain id
 * \param pc	position counter
 * \param epTime	current time
 * \param remTime	remaining time
 */
eveChainProgressMessage::eveChainProgressMessage(int cid, int pc, eveTime epTime, int remTime, int prio, int dest) :
    eveMessage(EVEMESSAGETYPE_CHAINPROGRESS, prio, dest)
{
	chainId = cid;
	posCounter = pc;
	timestamp = epTime;
	remainingTime =remTime;
}

/*
 * Class eveChainStatusMessage
 */
/**
 * \param status status of currently executing chain
 * \param chid  chain id
 * \param sid	scanmodule id
 * \param pc	position counter
 */
eveChainStatusMessage::eveChainStatusMessage(int cid, eveChainStatus& chstatus) :
    eveMessage(EVEMESSAGETYPE_CHAINSTATUS, 0, 0)
{
    ecstatus = chstatus.getCStatus();
    chainId = cid;
    esmstatus = QHash<int, quint32>(chstatus.getSMStatusHash());
    lastSMsmid = chstatus.getLastSmId();
    lastSMStatus = chstatus.getLastSmStatus();
    timestamp = eveTime::getCurrent();
    lastSMisDone = chstatus.isDoneSM();
    lastSmStarted = chstatus.isSmStarting();
}

/**
 * \param status status of currently executing chain
 * \param chid  chain id
 * \param sid	scanmodule id
 * \param pc	position counter
 */
eveChainStatusMessage::eveChainStatusMessage(int cid, CHStatusT chstat) :
    eveMessage(EVEMESSAGETYPE_CHAINSTATUS, 0, 0)
{
    ecstatus = chstat;
    chainId = cid;
    lastSMsmid = 0;
    esmstatus.clear();
    lastSMStatus = SMStatusNOTSTARTED;
    timestamp = eveTime::getCurrent();
    lastSMisDone = false;
    lastSmStarted = false;
}

eveChainStatusMessage::~eveChainStatusMessage()
{
}
/**
 * \brief compare two messages
 * \param message pointer to the message to compare with
 * \return true if messages are identical else false (always false)
 */
bool eveChainStatusMessage::compare(eveMessage *message)
{
    if (!message) return false;
    if (type != message->getType()) return false;
    if (chainId != ((eveChainStatusMessage*)message)->getChainId()) return false;

    return false;
}

/*
 * Class RequestMessage
 */
/**
 * \param rid request id
 * \param rtype request type
 * \param rtext request text
 */
eveRequestMessage::eveRequestMessage(int rid, int rtype, QString rtext){
	type = EVEMESSAGETYPE_REQUEST;
	requestId = rid;
	requestType = rtype;
        requestString = rtext;
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveRequestMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (requestId != ((eveRequestMessage*)message)->getReqId()) return false;
	if (requestType != ((eveRequestMessage*)message)->getReqType()) return false;
	if (requestString != ((eveRequestMessage*)message)->getReqText()) return false;
	return true;
}

/*
 * Class RequestAnswerMessage
 */
/**
 * \param rid request id
 * \param rtype request type
 * \param answer request answer yes/no
 */
eveRequestAnswerMessage::eveRequestAnswerMessage(int rid, int rtype, bool answer){
	type = EVEMESSAGETYPE_REQUESTANSWER;
	requestId = rid;
	requestType = rtype;
	answerBool = answer;
}
/**
 * \param rid request id
 * \param rtype request type
 * \param answer request answer integer
 */
eveRequestAnswerMessage::eveRequestAnswerMessage(int rid, int rtype, int answer){
	type = EVEMESSAGETYPE_REQUESTANSWER;
	requestId = rid;
	requestType = rtype;
	answerInt = answer;
}
/**
 * \param rid request id
 * \param rtype request type
 * \param answer request answer float
 */
eveRequestAnswerMessage::eveRequestAnswerMessage(int rid, int rtype, float answer){
	type = EVEMESSAGETYPE_REQUESTANSWER;
	requestId = rid;
	requestType = rtype;
	answerFloat = answer;
}
/**
 * \param rid request id
 * \param rtype request type
 * \param answer request answer string
 */
eveRequestAnswerMessage::eveRequestAnswerMessage(int rid, int rtype, QString answer){
	type = EVEMESSAGETYPE_REQUESTANSWER;
	requestId = rid;
	requestType = rtype;
	answerString = QString(answer);
}
eveRequestAnswerMessage::~eveRequestAnswerMessage(){

}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveRequestAnswerMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;

	// it is an eveRequestAnswerMessage
	if (requestId != ((eveRequestAnswerMessage*)message)->getReqId()) return false;
	if (requestType != ((eveRequestAnswerMessage*)message)->getReqType()) return false;

	return true;
}

/*
 * Class RequestCancelMessage
 */
/**
 * \param rid request id
 */
eveRequestCancelMessage::eveRequestCancelMessage(int rid){
	type = EVEMESSAGETYPE_REQUESTCANCEL;
	requestId = rid;
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveRequestCancelMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (requestId != ((eveRequestCancelMessage*)message)->getReqId()) return false;
	return true;
}

/*
 * Class eveErrorMessage
 */
/**
 * \param errSeverity severity (epics severity if an epics error message)
 * \param errFacility predefined facility (e.g. xml-Parser,..) where an error occured
 * \param errType predefined error types (Timeout, allocation error, etc)
 * \param errString additional error message
 */
eveErrorMessage::eveErrorMessage(int errSeverity, int errFacility, int errType, QString errString, int prio){

	type = EVEMESSAGETYPE_ERROR;
	priority = prio;
	severity = errSeverity;
	facility = errFacility;
	errorType = errType;
	errorString = QString(errString);
	timestamp = eveTime::getCurrent();
}
eveErrorMessage::~eveErrorMessage()
{
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveErrorMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;

	// it is an error message
	if (severity != ((eveErrorMessage*)message)->getSeverity()) return false;
	if (facility != ((eveErrorMessage*)message)->getFacility()) return false;
	if (errorType != ((eveErrorMessage*)message)->getErrorType()) return false;
	if (errorString != (((eveErrorMessage*)message)->getErrorText())) return false;
	return true;
}

/*
 * Class eveBaseDataMessage
 */
eveBaseDataMessage::eveBaseDataMessage(int mtype, int cid, int smid, QString xmlid, QString nam, QString auxinfo, eveDataModType mod, int msecs, int prio, int dest) :
	eveMessage(mtype, prio, dest)
{
	dataModifier = mod;
	auxInfo=auxinfo;
	chainId = cid;
	smId = smid;
	name = QString(nam);
	xmlId = QString(xmlid);
	if (msecs < 0)
		mSecsSinceStart = eveStartTime::getMSecsSinceStart();
	else
		mSecsSinceStart = msecs;
}

eveBaseDataMessage::eveBaseDataMessage(int mtype, int cid, int smid, QString xmlid, QString nam, int prio, int dest) :
	eveMessage(mtype, prio, dest)
{
	dataModifier = DMTunmodified;
	chainId = cid;
	smId = smid;
	name = QString(nam);
	xmlId = QString(xmlid);
	mSecsSinceStart = eveStartTime::getMSecsSinceStart();
}

eveBaseDataMessage::eveBaseDataMessage(int mtype, int prio, int dest) :
	eveMessage(mtype, prio, dest)
{
	mSecsSinceStart = eveStartTime::getMSecsSinceStart();
}

eveBaseDataMessage::~eveBaseDataMessage()
{
}

/*
 * Class eveDataMessage
 */
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data integer array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QVector<int> data, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = eveDataStatus(stat);
	dataType = eveInt32T;
	dataArrayInt = QVector<int>(data);
	arraySize = dataArrayInt.size();
	timestamp = eveTime(mtime);
	posCount = 0;
	dateTime = QDateTime::currentDateTime();
}
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data short array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QVector<short> data, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = stat;
	dataType = eveInt16T;
	dataArrayShort = QVector<short>(data);
	arraySize = dataArrayShort.size();
	timestamp = eveTime(mtime);
	posCount = 0;
	dateTime = QDateTime::currentDateTime();
}
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data char array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QVector<signed char> data, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = stat;
	dataType = eveInt8T;
	dataArrayChar = QVector<signed char>(data);
	arraySize = dataArrayChar.size();
	timestamp = eveTime(mtime);
	posCount = 0;
	dateTime = QDateTime::currentDateTime();
}
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data float array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QVector<float> data, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = stat;
	dataType = eveFloat32T;
	dataArrayFloat = QVector<float>(data);
	arraySize = dataArrayFloat.size();
	timestamp = eveTime(mtime);
	posCount = 0;
	dateTime = QDateTime::currentDateTime();
}
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data double array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QVector<double> data, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = stat;
	dataType = eveFloat64T;
	dataArrayDouble = QVector<double>(data);
	arraySize = dataArrayDouble.size();
	timestamp = eveTime(mtime);
	posCount = 0;
	dateTime = QDateTime::currentDateTime();
}
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data double array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QStringList data, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = stat;
	dataType = eveStringT;
	dataStrings = QStringList(data);
	arraySize = dataStrings.size();
	timestamp = eveTime(mtime);
	posCount = 0;
	dateTime = QDateTime::currentDateTime();
}
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data double array data
 */
eveDataMessage::eveDataMessage(QString xmlid, QString name, eveDataStatus stat, eveDataModType dmod, eveTime mtime, QDateTime datetime, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DATA, 0, 0, xmlid, name, QString(), dmod, -1, prio, dest)
{
	dataStatus = stat;
	dataType = eveDateTimeT;
	dateTime = datetime;
	arraySize = 1;
	timestamp = eveTime(mtime);
	posCount = 0;
    QChar sign('+');

    // TODO
    // Qt::ISO doesn't do it, we have to build our own ISO format, which is wrong
    // if UTC time diff is not full hours (India?)
    // Fix if using QT5 which has proper support
    QDateTime UTC(QDateTime::currentDateTimeUtc());
    QDateTime local(UTC.toLocalTime());
    int utcdiff = local.time().hour() - UTC.time().hour();
    if (utcdiff < 12) utcdiff +=24;
    if (utcdiff > 12) utcdiff -=24;
    if (utcdiff < 0) sign = QChar('-');

    if (dateTime.isValid())
        dataStrings = QStringList(QString("%1%2%3:00").arg(dateTime.toString("yyyy-MM-ddThh:mm:ss.zzz"))\
                                  .arg(sign).arg(utcdiff,2,10,QLatin1Char('0')));
	else
		dataStrings = QStringList("");

}

eveDataMessage::~eveDataMessage() {
}

/**
 * \brief return a copy of this message object
 */
eveDataMessage* eveDataMessage::clone(){

    eveDataMessage* message=NULL;

	switch (dataType) {
		case eveInt8T:					/* eveInt8 */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dataArrayChar, priority, destination);
		break;
		case eveInt16T:					/* eveInt16 */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dataArrayShort, priority, destination);
		break;
		case eveInt32T:					/* eveInt32 */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dataArrayInt, priority, destination);
		break;
		case eveFloat32T:					/* eveFloat32 */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dataArrayFloat, priority, destination);
		break;
		case eveFloat64T:					/* eveFloat64 */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dataArrayDouble, priority, destination);
		break;
		case eveStringT:					/* eveString */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dataStrings, priority, destination);
		break;
		case eveDateTimeT:					/* eveString */
			message = new eveDataMessage(xmlId, name, dataStatus, dataModifier, timestamp, dateTime, priority, destination);
		break;
		default:
			eveError::log(ERROR,"eveDataMessage unknown data type");
			assert(true);
            return message;
		break;
	}
	message->setChainId(chainId);
	message->setSmId(smId);
	message->setPositionCount(posCount);
	message->setAuxString(auxInfo);
	message->setNormalizeId(normalizeId);
    message->setStorageHint(storageHint);
    message->setMSecsSinceStart(mSecsSinceStart);
	return message;
}
/**
 * \brief compare is not implemented (nor used)
 */
bool eveDataMessage::compare(eveMessage* with){
	return false;
}

eveVariant eveDataMessage::toVariant(){
	eveVariant result;

	switch (dataType) {
		case eveInt8T:					/* eveInt8 */
			result.setType(eveINT);
			if (dataArrayChar.size() > 0) result.setValue(dataArrayChar[0]);
		break;
		case eveInt16T:					/* eveInt16 */
			result.setType(eveINT);
			if (dataArrayShort.size() > 0) result.setValue(dataArrayShort[0]);
		break;
		case eveInt32T:					/* eveInt32 */
			result.setType(eveINT);
			if (dataArrayInt.size() > 0) result.setValue(dataArrayInt[0]);
		break;
		case eveFloat32T:					/* eveFloat32 */
			result.setType(eveDOUBLE);
			if (dataArrayFloat.size() > 0) result.setValue(dataArrayFloat[0]);
		break;
		case eveFloat64T:					/* eveFloat64 */
			result.setType(eveDOUBLE);
			if (dataArrayDouble.size() > 0) result.setValue(dataArrayDouble[0]);
		break;
		case eveStringT:					/* eveString */
			result.setType(eveSTRING);
			if (dataStrings.size() > 0) result.setValue(dataStrings.at(0));
		break;
		case eveDateTimeT:					/* eveString */
			result.setType(eveDateTimeT);
			result.setValue(dateTime);
		break;
		default:
			eveError::log(ERROR,"eveDataMessage unknown data type");
		break;
	}
	return result;
}

bool eveDataMessage::isFloat(){
	return ((dataType == eveFloat32T) || (dataType == eveFloat64T));
}

bool eveDataMessage::isInteger(){
	return ((dataType == eveInt8T) || (dataType == eveInt16T) || (dataType == eveInt32T));
}

void eveDataMessage::invalidate() {

    switch (dataType) {
    case eveInt8T:					/* eveInt8 */
        foreach (signed char val, dataArrayChar) val = 0xff;
        break;
    case eveInt16T:					/* eveInt16 */
        foreach (short val, dataArrayShort) val = 0xffff;
        break;
    case eveInt32T:					/* eveInt32 */
        foreach (int val, dataArrayInt) val = 0xffffffff;
        break;
    case eveFloat32T:					/* eveFloat32 */
        foreach (float val, dataArrayFloat) val = NAN;
        break;
    case eveFloat64T:					/* eveFloat64 */
        foreach (double val, dataArrayDouble) val = NAN;
        break;
    case eveStringT:					/* eveString */
        break;
    case eveDateTimeT:					/* eveString */
        break;
    default:
        eveError::log(ERROR,"eveDataMessage unknown data type");
        break;
    }


}

// -----------------------

eveDevInfoMessage::eveDevInfoMessage(int cid, int sid, QString xmlid, QString name, eveType dtype, bool arr, eveDataModType dataMod, QString auxinfo, QStringList* info, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DEVINFO, cid, sid, xmlid, name, auxinfo, dataMod, prio, dest)
{
	dataType = dtype;
	isarray = arr;
	infoList = info;
	dataModifier = DMTunmodified;
}
eveDevInfoMessage::eveDevInfoMessage(QString xmlid, QString name, QStringList* info, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DEVINFO, 0, 0, xmlid, name, prio, dest)
{
	infoList = info;
	dataModifier = DMTunmodified;
}
eveDevInfoMessage::eveDevInfoMessage(QString xmlid, QString name, QStringList* info, eveDataModType dataMod, QString auxinfo, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DEVINFO, 0, 0, xmlid, name, auxinfo, dataMod, prio, dest)
{
	infoList = info;
}
eveDevInfoMessage::eveDevInfoMessage(QStringList* info, int prio, int dest) :
	eveBaseDataMessage(EVEMESSAGETYPE_DEVINFO, 0, 0, QString(), QString(), prio, dest)
{
	infoList = info;
	dataModifier = DMTunmodified;
}

eveDevInfoMessage::~eveDevInfoMessage() {
	if (infoList != NULL) {
		infoList->clear();
		delete infoList;
	}
}

eveDevInfoMessage* eveDevInfoMessage::clone() {
	eveDevInfoMessage* message = new eveDevInfoMessage(chainId, smId, xmlId, name, dataType, isarray, dataModifier, auxInfo, infoList, priority, destination);
    message->setNormalizeId(normalizeId);
    message->setStorageHint(storageHint);
    message->setMSecsSinceStart(mSecsSinceStart);
	return message;
}

/**
 * \brief compare is not implemented (nor used)
 */
bool eveDevInfoMessage::compare(eveMessage* with){
	return false;
}

/*
 * Class evePlayListMessage
 */
/**
 * \param playlist current playlist
 */
evePlayListMessage::evePlayListMessage(const QList<evePlayListEntry> playlist, int prio)
{
	type = EVEMESSAGETYPE_PLAYLIST;
	priority = prio;
	plList = QList<evePlayListEntry>(playlist);
}
evePlayListMessage::~evePlayListMessage() {
}

/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool evePlayListMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	// compare might be expensive, we always return false
	//if (*plList == *(((evePlayListMessage*)message)->getListPtr())) return true;
	return false;
}
/**
 * \brief clone a messages
 * \return identical evePlayListMessage Pointer
 */
//evePlayListMessage* evePlayListMessage::clone()
//{
//	return new evePlayListMessage(plList, priority);
//}
/**
 * \brief copy constructor
 */
evePlayListMessage::evePlayListMessage(evePlayListMessage& ref) : eveMessage(ref)
{
	plList = QList<evePlayListEntry>(ref.plList);
}
/**
 * \brief get playlist entry at specified index
 * \param index index
 * \return entry at index, returns an entry with pid=-1, if index is out of range
 */
evePlayListEntry evePlayListMessage::getEntry(int index){

	evePlayListEntry entry;
	entry.pid=-1;
	if ((index >= 0) && (index < plList.size()))
		entry = plList.at(index);
	return entry;
}

/**
 * \brief dump the content of this class
 */
void evePlayListMessage::dump(){

	((eveMessage*)this)->dump();
	foreach(evePlayListEntry entry, plList) {
		std::cout << "evePlayListMessage:  Author: " << qPrintable(entry.author) << " Name: "
				<< qPrintable(entry.name) << " pid: "<< entry.pid << "\n";
	}
}

/*
 * Class eveStorageMessage
 */
/**
 *
 * @param chid	chainId of sending chain
 * @param channel of sending chain
 * @param parameter Hash with all save parameters (filename, pluginname ...)h
 * @param prio priority
 * @param dest destination channel
 */
eveStorageMessage::eveStorageMessage(int chid, int channel, QHash<QString, QString>& parameter, int prio, int dest) :
	eveMessage(EVEMESSAGETYPE_STORAGECONFIG, prio, dest)
{
	chainId = chid;
	channelId = channel;
	paraHash = QHash<QString, QString>(parameter);
	filename = paraHash.value("savefilename", QString());
}
eveStorageMessage::~eveStorageMessage() {
}

QHash<QString, QString>&  eveStorageMessage::getHash(){
	return paraHash;
}

/**
 * \brief compare of eveStorageMessage always returns false
 * \param message a pointer to the message to compare with
 * \return always false,
 */
bool eveStorageMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	return false;
}

/**
 * \brief clone a messages
 * \return pointer to a copy of this eveStorageMessage
 */
eveStorageMessage* eveStorageMessage::clone()
{
	return new eveStorageMessage(chainId, channelId, paraHash, priority, destination);
}
/**
 * \brief print content to stdout
 */
void eveStorageMessage::dump()
{
	((eveMessage*)this)->dump();
	std::cout << "eveStorageMessage: chainId: " << chainId << " channel: " << channelId
				<< " filename: " << qPrintable(filename) << "\n";
	foreach(QString key, paraHash.keys()) {
		std::cout << "evePlayListMessage:  key: " << qPrintable(key) << " Value: "
				<< qPrintable(paraHash.value(key)) << "\n";
	}
}
