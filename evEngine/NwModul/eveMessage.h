#ifndef EVEMESSAGE_H_
#define EVEMESSAGE_H_

#include <QString>
#include <QStringList>
#include <QVector>
#include <QByteArray>
#include <epicsTime.h>
#include <epicsTypes.h>
#include <alarm.h>

#define EVEMESSAGE_STARTTAG 0x0d0f0d0a
#define EVEMESSAGE_VERSION 0x0100
#define EVEMESSAGE_MAXLENGTH 300000

#define EVEMESSAGE_MIN_SUPPORTEDVERSION EVEMESSAGE_VERSION
#define EVEMESSAGE_MAX_SUPPORTEDVERSION EVEMESSAGE_VERSION

#define EVEMESSAGETYPE_START 0x0001
#define EVEMESSAGETYPE_HALT 0x0002
#define EVEMESSAGETYPE_BREAK 0x0003
#define EVEMESSAGETYPE_STOP 0x0004
#define EVEMESSAGETYPE_PAUSE 0x0005
#define EVEMESSAGETYPE_ENDPROGRAM 0x0006
#define EVEMESSAGETYPE_REQUESTANSWER 0x0008
// #define EVEMESSAGETYPE_SENDXML 0x0010
// #define EVEMESSAGETYPE_SENDXMLSTART 0x0011
#define EVEMESSAGETYPE_LIVEDESCRIPTION 0x0012
#define EVEMESSAGETYPE_AUTOPLAY 0x0013

#define EVEMESSAGETYPE_ERROR 0x0101
#define EVEMESSAGETYPE_ENGINESTATUS 0x0102
#define EVEMESSAGETYPE_CHAINSTATUS 0x0103
#define EVEMESSAGETYPE_DATA 0x0104
#define EVEMESSAGETYPE_REQUEST 0x0108
#define EVEMESSAGETYPE_REQUESTCANCEL 0x0109
#define EVEMESSAGETYPE_CURRENTXML 0x0110

#define EVEMESSAGETYPE_PLAYLIST 0x0301

#define EVEMESSAGETYPE_ADDTOPLAYLIST 0x0201
#define EVEMESSAGETYPE_REMOVEFROMPLAYLIST 0x0202
#define EVEMESSAGETYPE_REORDERPLAYLIST 0x0203

#define EVEREQUESTTYPE_YESNO 0x00
#define EVEREQUESTTYPE_OKCANCEL 0x01
#define EVEREQUESTTYPE_INT 0x02
#define EVEREQUESTTYPE_FLOAT 0x03
#define EVEREQUESTTYPE_TEXT 0x04
#define EVEREQUESTTYPE_ERRORTEXT 0x05

#define EVEMESSAGEPRIO_NORMAL 0x0
#define EVEMESSAGEPRIO_HIGH 0x01

#define EVEMESSAGESEVERITY_DEBUG 0x01
#define EVEMESSAGESEVERITY_INFO 0x02
#define EVEMESSAGESEVERITY_MINOR 0x03
#define EVEMESSAGESEVERITY_ERROR 0x04
#define EVEMESSAGESEVERITY_FATAL 0x05

#define DEBUG EVEMESSAGESEVERITY_DEBUG
#define INFO EVEMESSAGESEVERITY_INFO
#define MINOR EVEMESSAGESEVERITY_MINOR
#define ERROR EVEMESSAGESEVERITY_ERROR
#define FATAL EVEMESSAGESEVERITY_FATAL

#define EVEMESSAGEFACILITY_CPARSER 0x0a
#define EVEMESSAGEFACILITY_NETWORK 0x0b
#define EVEMESSAGEFACILITY_MHUB 0x0c
#define EVEMESSAGEFACILITY_PLAYLIST 0x0d

#define EVEERROR_TIMEOUT 0x0009

#define EVEENGINESTATUS_IDLENOXML 0x00 		// Idle, no XML-File loaded
#define EVEENGINESTATUS_IDLEXML 0x01		// Idle, XML loaded
#define EVEENGINESTATUS_LOADING 0x02		// loading XML
#define EVEENGINESTATUS_EXECUTING 0x03		// executing


struct eveDataStatus {
	quint8	severity;
	quint8	condition;
	quint16	acqStatus;
};

struct evePlayListEntry {
	int pid;
	QString name;
	QString author;
};

enum eveDataModType {DMTunmodified, DMTcenter, DMTedge, DMTmin, DMTmax, DMTfwhm, DMTmean, DMTstandarddev};

/**
 * \brief base class for errorMessage, engineStatus, chainStatus, dataMessage ...
 */
class eveMessage
{
public:
	eveMessage();
	eveMessage(int);
	virtual ~eveMessage();

	int getType(){return type;};
	int getPriority(){return priority;};
	void setPriority(int prio){priority = prio;};
	virtual bool compare(eveMessage *);

protected:
	int type;
	int priority;
};

/**
 * \brief a message containing simple text
 */
class eveMessageText : public eveMessage
{
public:
	eveMessageText(int, QString);
	virtual ~eveMessageText(){};
	QString& getText(){return messageText;};
	bool compare(eveMessage *);

private:
	QString messageText;
};

/*
 * do we need this?
 *
class eveMessageTextList : public eveMessage
{
public:
	eveMessageTextList(int, QStringList);
	virtual ~eveMessageTextList(){};
	QStringList& getList(){return messageList;};
	bool compare(eveMessage *);

private:
	QStringList messageList;
};
*/

/**
 * \brief a message containing one integer
 */
class eveMessageInt : public eveMessage
{
public:
	eveMessageInt(int, int);
	virtual ~eveMessageInt(){};
	int getInt(){return value;};
	bool compare(eveMessage *);

private:
	int value;
};

/**
 * \brief a message containing an array of integers
 *
 */
class eveMessageIntList : public eveMessage
{
public:
	eveMessageIntList(int, int, int);
	virtual ~eveMessageIntList(){};
	bool compare(eveMessage *);
	int getInt(int);

private:
	int ivalue1;
	int ivalue2;
};

/**
 * \brief a message to add an entry to the playlist
 */
class eveAddToPlMessage : public eveMessage
{
public:
	eveAddToPlMessage(QString, QString, QByteArray);
	virtual ~eveAddToPlMessage();
	QString * getXmlName(){return XmlName;};
	QString * getXmlAuthor(){return XmlAuthor;};
	QByteArray * getXmlData(){return XmlData;};
	bool compare(eveMessage *);

private:
	QString * XmlName;
	QString * XmlAuthor;
	QByteArray * XmlData;
};

/**
 * \brief a message to send the current XML-description
 */
class eveCurrentXmlMessage : public eveAddToPlMessage
{
public:
	eveCurrentXmlMessage(QString, QString, QByteArray);
	virtual ~eveCurrentXmlMessage();
};

/**
 * \brief a message containing the status of the currently processing scanmodule
 */
class eveEngineStatusMessage : public eveMessage
{
public:
	eveEngineStatusMessage(int, QString);
	virtual ~eveEngineStatusMessage();
	int getStatus(){return estatus;};
	QString * getXmlId(){return XmlId;};
	epicsTime getTime(){return timestamp;};
	bool compare(eveMessage *);

private:
	epicsTime timestamp;
	int estatus;
	QString * XmlId;
};

/**
 * \brief a message containing the status of the currently processing chain
 */
class eveChainStatusMessage : public eveMessage
{
public:
	eveChainStatusMessage(int, int, int, int);
	eveChainStatusMessage(int, int, int, int, epicsTime);
	virtual ~eveChainStatusMessage();
	int getStatus(){return cstatus;};
	int getChainId(){return chainId;};
	int getSmId(){return smId;};
	int getPosCnt(){return posCounter;};
	epicsTime getTime(){return timestamp;};
	bool compare(eveMessage *);

private:
	epicsTime timestamp;
	int cstatus;
	int chainId;
	int smId;
	int posCounter;
};

/**
 * \brief a message containing a request to be sent to viewers
 *
 */
class eveRequestMessage : public eveMessage
{
public:
	eveRequestMessage(int, int, QString);
	virtual ~eveRequestMessage(){};
	int getReqId(){return requestId;};
	int getReqType(){return requestType;};
	QString& getReqText() {return requestString;};
	bool compare(eveMessage *);

private:
	int requestId;
	int requestType;
	QString requestString;
};

/**
 * \brief a message containing a request answer from viewers
 */
class eveRequestAnswerMessage : public eveMessage
{
public:
	eveRequestAnswerMessage(int, int, bool);
	eveRequestAnswerMessage(int, int, int);
	eveRequestAnswerMessage(int, int, float);
	eveRequestAnswerMessage(int, int, QString);
	virtual ~eveRequestAnswerMessage();
	int getReqId(){return requestId;};
	int getReqType(){return requestType;};
	bool compare(eveMessage *);

private:
	int requestId;
	int requestType;
	QString *answerString;
	bool answerBool;
	int answerInt;
	float answerFloat;
};

/**
 * \brief message to cancel an outstanding request
 */
// TODO same as eveMessageInt, obsolete
class eveRequestCancelMessage : public eveMessage
{
public:
	eveRequestCancelMessage(int);
	virtual ~eveRequestCancelMessage(){};
	int getReqId(){return requestId;};
	bool compare(eveMessage *);

private:
	int requestId;
};

/**
 * \brief message to cancel an outstanding request
 */
class eveErrorMessage : public eveMessage
{
public:
	eveErrorMessage(int, int, int, QString);
	virtual ~eveErrorMessage();

	int getSeverity(){return severity;};
	int getFacility(){return facility;};
	int getErrorType(){return errorType;};
	epicsTime getTime(){return timestamp;};
	QString * getErrorText(){return errorString;};
	bool compare(eveMessage *);

private:
	int severity;
	int facility;
	int errorType;
	epicsTime timestamp;
	QString *errorString;
};

/**
 * \brief message containing measured compound data
 */
class eveDataMessage : public eveMessage
{
public:
	eveDataMessage(QString, eveDataStatus);
	eveDataMessage(QString, eveDataStatus, eveDataModType, epicsTime, QVector<int> );
	eveDataMessage(QString, eveDataStatus, eveDataModType, epicsTime, QVector<short> );
	eveDataMessage(QString, eveDataStatus, eveDataModType, epicsTime, QVector<char> );
	eveDataMessage(QString, eveDataStatus, eveDataModType, epicsTime, QVector<float> );
	eveDataMessage(QString, eveDataStatus, eveDataModType, epicsTime, QVector<double> );
	eveDataMessage(QString, eveDataStatus, eveDataModType, epicsTime, QStringList );
	virtual ~eveDataMessage();

	const QVector<int>& getIntArray(){return dataArrayInt;};
	const QVector<short>& getShortArray(){return dataArrayShort;};
	const QVector<char>& getCharArray(){return dataArrayChar;};
	const QVector<float>& getFloatArray(){return dataArrayFloat;};
	const QVector<double>& getDoubleArray(){return dataArrayDouble;};
	const QStringList& getStringArray(){return dataStrings;};
	QString getId(){return ident;};
	eveDataStatus getStatus(){return dataStatus;};
	epicsType getType(){return dataType;};
	eveDataModType getDataMod(){return dataModifier;};
	epicsTime getTimeStamp(){return timestamp;};
	bool isEmpty(){return !(arraySize);};
	int getArraySize(){return arraySize;};

private:
	QString ident;
	epicsTime timestamp;
	quint32 arraySize;
	eveDataStatus dataStatus;				// epicsSeverity, SeverityCondition, AcquisitionStatus
	epicsType dataType;
	eveDataModType dataModifier;
	QVector<int> dataArrayInt;
	QVector<short> dataArrayShort;
	QVector<char> dataArrayChar;
	QVector<float> dataArrayFloat;
	QVector<double> dataArrayDouble;
	QStringList dataStrings;
};


/**
 * \brief message containing the current playlist
 */
class evePlayListMessage : public eveMessage
{
public:
	evePlayListMessage(const QList<evePlayListEntry>);
	~evePlayListMessage();
	int getCount(){return plList->size();};
	evePlayListEntry & getEntry(int);
	bool compare(eveMessage *);

private:
	QList<evePlayListEntry> *plList;
	evePlayListEntry entry;
};


#endif /*EVEMESSAGE_H_*/
