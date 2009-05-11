
#include <assert.h>
#include "eveMessage.h"
#include "eveError.h"

eveMessage::eveMessage()
{
	type = 0;
	priority = EVEMESSAGEPRIO_NORMAL;
}

/**
 * \brief Constructor
 * \param mtype the messagetype definition
 */
eveMessage::eveMessage(int mtype, int prio)
{
	priority = prio;
	type = mtype;
	assert ((type == EVEMESSAGETYPE_START) ||
			(type == EVEMESSAGETYPE_HALT) ||
			(type == EVEMESSAGETYPE_BREAK) ||
			(type == EVEMESSAGETYPE_STOP) ||
			(type == EVEMESSAGETYPE_PAUSE) ||
			(type == EVEMESSAGETYPE_ENDPROGRAM));
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
	if (!message) return false;
	if (type != message->getType()) return false;
	return true;
}


/**
 * \param mtype the type of message
 * \param text message text
 */
eveMessageText::eveMessageText(int mType, QString text, int prio)
{
	type = mType;
	// check the allowed types; for now EVEMESSAGETYPE_LIVEDESCRIPTION is the only candidate
	assert(type == EVEMESSAGETYPE_LIVEDESCRIPTION);
	messageText = text;
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

/*
 * Class eveMessageList
eveMessageTextList::eveMessageTextList(int mType, QStringList mList)
{
	type = mType;
	// check the allowed types; for now EVEMESSAGETYPE_PLAYLIST is the only candidate
	assert(type != EVEMESSAGETYPE_PLAYLIST);
	messageList = mList;
}
bool eveMessageTextList::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (messageList != ((eveMessageTextList*)message)->getList()) return false;
	return true;
}
 */

/**
 * \param mtype the type of message
 * \param ival integer value of the message
 */
eveMessageInt::eveMessageInt(int iType, int ival, int prio)
{
	priority = prio;
	value = ival;
	type = iType;
	// check the allowed types
	assert ((type == EVEMESSAGETYPE_AUTOPLAY) || (type == EVEMESSAGETYPE_REMOVEFROMPLAYLIST));
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
		eveError::log(4,"eveMessageIntList::getInt: index out of range");
		return 0;
	}
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
	XmlName = new QString(xmlname);
	XmlAuthor = new QString(xmlauthor);
	XmlData = new QByteArray(xmldata);
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
	if (*XmlName != *(((eveAddToPlMessage*)message)->getXmlName())) return false;
	if (*XmlAuthor != *(((eveAddToPlMessage*)message)->getXmlAuthor())) return false;
	if (*XmlData != *(((eveAddToPlMessage*)message)->getXmlData())) return false;

	return true;
}
eveAddToPlMessage::~eveAddToPlMessage()
{
	delete XmlName;
	delete XmlAuthor;
	delete XmlData;
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
eveEngineStatusMessage::eveEngineStatusMessage(int status, QString xmlname, int prio)
{
	type = EVEMESSAGETYPE_ENGINESTATUS;
	priority = prio;
	XmlId = new QString(xmlname);
	estatus = status;
	timestamp = epicsTime::getCurrent();
}
eveEngineStatusMessage::~eveEngineStatusMessage()
{
	delete XmlId;
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

	// it is an eveEngineStatusMessage
	// tbd
	//if (estatus != ((eveEngineStatusMessage*)message)->getestatus()) return false;
	eveError::log(4,"eveEngineStatusMessage::compare: not yet implemented");

	return false;
	// return true;
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
eveChainStatusMessage::eveChainStatusMessage(int status, int cid, int sid, int pc)
{
	type = EVEMESSAGETYPE_CHAINSTATUS;
	cstatus = status;
	chainId = cid;
	smId = sid;
	posCounter = pc;
	timestamp = epicsTime::getCurrent();
}
/**
 * \param status status of currently executing chain
 * \param chid  chain id
 * \param sid	scanmodule id
 * \param pc	position counter
 * \param epTime	epicsTime
 */
eveChainStatusMessage::eveChainStatusMessage(int status, int cid, int sid, int pc, epicsTime epTime, int remTime, int prio)
{
	type = EVEMESSAGETYPE_CHAINSTATUS;
	priority = prio;
	cstatus = status;
	chainId = cid;
	smId = sid;
	posCounter = pc;
	timestamp = epTime;
	remainingTime =remTime;
}

eveChainStatusMessage::~eveChainStatusMessage()
{
}
/**
 * \brief compare two messages
 * \param message a pointer to the message to compare with
 * \return true if messages are identical else false
 */
bool eveChainStatusMessage::compare(eveMessage *message)
{
	if (!message) return false;
	if (type != message->getType()) return false;
	if (cstatus == ((eveChainStatusMessage*)message)->getStatus()) return true;

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
	answerString = new QString(answer);
}
eveRequestAnswerMessage::~eveRequestAnswerMessage(){

	if ((requestType == EVEREQUESTTYPE_TEXT) || (requestType == EVEREQUESTTYPE_ERRORTEXT)) delete answerString;
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
	errorString = new QString(errString);
}
eveErrorMessage::~eveErrorMessage()
{
	delete errorString;
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
	if (*errorString != *(((eveErrorMessage*)message)->getErrorText())) return false;
	return true;
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
eveDataMessage::eveDataMessage(QString Id, eveDataStatus stat, eveDataModType dmod, epicsTime mtime, QVector<int> data)
{
	type = EVEMESSAGETYPE_DATA;
	ident = Id;
	dataStatus = stat;
	dataModifier = dmod;
	dataType = epicsInt32T;
	dataArrayInt = QVector<int>(data);
	arraySize = dataArrayInt.size();
	timestamp = mtime;
};
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data short array data
 */
eveDataMessage::eveDataMessage(QString Id, eveDataStatus stat, eveDataModType dmod, epicsTime mtime, QVector<short> data){
	type = EVEMESSAGETYPE_DATA;
	ident = Id;
	dataStatus = stat;
	dataModifier = dmod;
	dataType = epicsInt16T;
	dataArrayShort = QVector<short>(data);
	arraySize = dataArrayShort.size();
	timestamp = mtime;
};
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data char array data
 */
eveDataMessage::eveDataMessage(QString Id, eveDataStatus stat, eveDataModType dmod, epicsTime mtime, QVector<char> data){
	type = EVEMESSAGETYPE_DATA;
	ident = Id;
	dataStatus = stat;
	dataModifier = dmod;
	dataType = epicsInt8T;
	dataArrayChar = QVector<char>(data);
	arraySize = dataArrayChar.size();
	timestamp = mtime;
};
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data float array data
 */
eveDataMessage::eveDataMessage(QString Id, eveDataStatus stat, eveDataModType dmod, epicsTime mtime, QVector<float> data){
	type = EVEMESSAGETYPE_DATA;
	ident = Id;
	dataStatus = stat;
	dataModifier = dmod;
	dataType = epicsFloat32T;
	dataArrayFloat = QVector<float>(data);
	arraySize = dataArrayFloat.size();
	timestamp = mtime;
};
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data double array data
 */
eveDataMessage::eveDataMessage(QString Id, eveDataStatus stat, eveDataModType dmod, epicsTime mtime, QVector<double> data){
	type = EVEMESSAGETYPE_DATA;
	ident = Id;
	dataStatus = stat;
	dataModifier = dmod;
	dataType = epicsFloat64T;
	dataArrayDouble = QVector<double>(data);
	arraySize = dataArrayDouble.size();
	timestamp = mtime;
};
/**
 * \param Id id (name) of Device which sent the data
 * \param stat status of the data
 * \param dmod kind of data (raw, mean, edge, center)
 * \param mtime time of data acquisition
 * \param data double array data
 */
eveDataMessage::eveDataMessage(QString Id, eveDataStatus stat, eveDataModType dmod, epicsTime mtime, QStringList data){
	type = EVEMESSAGETYPE_DATA;
	ident = Id;
	dataStatus = stat;
	dataModifier = dmod;
	dataType = epicsStringT;
	dataStrings = QStringList(data);
	arraySize = dataStrings.size();
	timestamp = mtime;
};

eveDataMessage::~eveDataMessage() {
};

/**
 * \brief return a copy of this message object
 */
eveDataMessage* eveDataMessage::clone(){

	eveDataMessage* message;

	switch (dataType) {
		case epicsInt8T:					/* epicsInt8 */
			message = new eveDataMessage(ident, dataStatus, dataModifier, timestamp, dataArrayChar);
		break;
		case epicsInt16T:					/* epicsInt16 */
			message = new eveDataMessage(ident, dataStatus, dataModifier, timestamp, dataArrayShort);
		break;
		case epicsInt32T:					/* epicsInt32 */
			message = new eveDataMessage(ident, dataStatus, dataModifier, timestamp, dataArrayInt);
		break;
		case epicsFloat32T:					/* epicsFloat32 */
			message = new eveDataMessage(ident, dataStatus, dataModifier, timestamp, dataArrayFloat);
		break;
		case epicsFloat64T:					/* epicsFloat64 */
			message = new eveDataMessage(ident, dataStatus, dataModifier, timestamp, dataArrayDouble);
		break;
		case epicsStringT:					/* epicsString */
			message = new eveDataMessage(ident, dataStatus, dataModifier, timestamp, dataStrings);
		break;
		default:
			eveError::log(4,"eveDataMessage unknown data type");
			assert(true);
		break;
	}
	message->setPriority(this->getPriority());
	return message;
};

eveVariant eveDataMessage::toVariant(){
	eveVariant result;

	switch (dataType) {
		case epicsInt8T:					/* epicsInt8 */
			result.setType(eveINT);
			if (dataArrayChar.size() > 0) result.setValue(dataArrayChar[0]);
		break;
		case epicsInt16T:					/* epicsInt16 */
			result.setType(eveINT);
			if (dataArrayShort.size() > 0) result.setValue(dataArrayShort[0]);
		break;
		case epicsInt32T:					/* epicsInt32 */
			result.setType(eveINT);
			if (dataArrayInt.size() > 0) result.setValue(dataArrayInt[0]);
		break;
		case epicsFloat32T:					/* epicsFloat32 */
			result.setType(eveDOUBLE);
			if (dataArrayFloat.size() > 0) result.setValue(dataArrayFloat[0]);
		break;
		case epicsFloat64T:					/* epicsFloat64 */
			result.setType(eveDOUBLE);
			if (dataArrayDouble.size() > 0) result.setValue(dataArrayDouble[0]);
		break;
		case epicsStringT:					/* epicsString */
			result.setType(eveSTRING);
			if (dataStrings.size() > 0) result.setValue(dataStrings.at(0));
		break;
		default:
			eveError::log(4,"eveDataMessage unknown data type");
		break;
	}
	return result;
}

/*
 * Class evePlayListMessage
 */
/**
 * \param playlist current playlist
 */
evePlayListMessage::evePlayListMessage(const QList<evePlayListEntry> playlist)
{
	type = EVEMESSAGETYPE_PLAYLIST;
	plList = new QList<evePlayListEntry>(playlist);
}
evePlayListMessage::~evePlayListMessage() {
	delete plList;
};

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
 * \brief get playlist entry at specified index
 * \param index index
 * \return entry at index, returns an entry with pid=-1, if index is out of range
 */
evePlayListEntry & evePlayListMessage::getEntry(int index){

	entry.pid=-1;
	if ((index >= 0) && (index < plList->size()))
		entry = plList->at(index);
	return entry;
}

