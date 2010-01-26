#ifndef EVEMESSAGE_H_
#define EVEMESSAGE_H_

#include <QString>
#include <QHash>
#include <QDateTime>
#include <QStringList>
#include <QVector>
#include <QByteArray>
#include <eveTime.h>
#include <eveTypes.h>
#include <alarm.h>
#include "eveVariant.h"

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
#define EVEMESSAGETYPE_BASEDATA 0x0105
#define EVEMESSAGETYPE_REQUEST 0x0108
#define EVEMESSAGETYPE_REQUESTCANCEL 0x0109
#define EVEMESSAGETYPE_CURRENTXML 0x0110

#define EVEMESSAGETYPE_PLAYLIST 0x0301

#define EVEMESSAGETYPE_ADDTOPLAYLIST 0x0201
#define EVEMESSAGETYPE_REMOVEFROMPLAYLIST 0x0202
#define EVEMESSAGETYPE_REORDERPLAYLIST 0x0203
#define EVEMESSAGETYPE_STORAGECONFIG 0x1000
#define EVEMESSAGETYPE_STORAGEACK 0x1001
#define EVEMESSAGETYPE_DEVINFO 0x1002
#define EVEMESSAGETYPE_EVENTREGISTER 0x1003

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

#define SUCCESS 0x00
#define DEBUG EVEMESSAGESEVERITY_DEBUG
#define INFO EVEMESSAGESEVERITY_INFO
#define MINOR EVEMESSAGESEVERITY_MINOR
#define ERROR EVEMESSAGESEVERITY_ERROR
#define FATAL EVEMESSAGESEVERITY_FATAL

#define EVEMESSAGEFACILITY_MFILTER 0x09
#define EVEMESSAGEFACILITY_CPARSER 0x0a
#define EVEMESSAGEFACILITY_NETWORK 0x0b
#define EVEMESSAGEFACILITY_MHUB 0x0c
#define EVEMESSAGEFACILITY_PLAYLIST 0x0d
#define EVEMESSAGEFACILITY_MANAGER 0x0e
#define EVEMESSAGEFACILITY_XMLPARSER 0x0f
#define EVEMESSAGEFACILITY_SCANCHAIN 0x10
#define EVEMESSAGEFACILITY_POSITIONCALC 0x11
#define EVEMESSAGEFACILITY_SMDEVICE 0x12
#define EVEMESSAGEFACILITY_CATRANSPORT 0x13
#define EVEMESSAGEFACILITY_SCANMODULE 0x14
#define EVEMESSAGEFACILITY_STORAGE 0x15
#define EVEMESSAGEFACILITY_EVENT 0x16
#define EVEMESSAGEFACILITY_LOCALTIMER 0x17

#define EVEERROR_TIMEOUT 0x0009

#define EVEENGINESTATUS_IDLENOXML 0x00 		// Idle, no XML-File loaded
#define EVEENGINESTATUS_IDLEXML 0x01		// Idle, XML loaded
#define EVEENGINESTATUS_LOADING 0x02		// loading XML
#define EVEENGINESTATUS_EXECUTING 0x03		// executing
#define EVEENGINESTATUS_AUTOSTART 0x0100		// autostart bit


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

// TODO
// remove all the clone() stuff except where virtual constructors are needed and use explicit copy constructors where needed


/**
 * \brief base class for errorMessage, engineStatus, chainStatus, dataMessage ...
 */
// TODO modify all constructors to init priority and destination properly
class eveMessage
{
public:
	eveMessage();
	eveMessage(int, int prio=0, int dest = 0);
	virtual ~eveMessage();

	int getType(){return type;};
	int getPriority(){return priority;};
	void setPriority(int prio){priority = prio;};
	int getDestination(){return destination;};
	void setDestination(int channel){destination = channel;};
	virtual bool compare(eveMessage *);
	virtual eveMessage* clone(){return new eveMessage(type, priority, destination);};
protected:
	int destination;
	int type;
	int priority;
};

/**
 * \brief a message containing simple text
 */
class eveMessageText : public eveMessage
{
public:
	eveMessageText(int, QString, int prio=0);
	virtual ~eveMessageText(){};
	QString& getText(){return messageText;};
	bool compare(eveMessage *);
	eveMessageText* clone(){return new eveMessageText(type,messageText, priority);};

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
	eveMessageInt(int, int, int prio=0, int dest=0);
	virtual ~eveMessageInt(){};
	int getInt(){return value;};
	bool compare(eveMessage *);
	eveMessageInt* clone(){return new eveMessageInt(type,value, priority, destination);}

private:
	int value;
};

/**
 * \brief a message containing an array of integers ( two integers for now)
 *
 */
class eveMessageIntList : public eveMessage
{
public:
	eveMessageIntList(int, int, int, int prio=0);
	virtual ~eveMessageIntList(){};
	bool compare(eveMessage *);
	int getInt(int);
	eveMessageIntList* clone(){return new eveMessageIntList(type,ivalue1,ivalue2, priority);};

private:
	int ivalue1;
	int ivalue2;
};

/**
 * \brief a message to add an entry to the playlist
 */
// TODO make one message out of the following, get rid of virtual clone method and protected members
class eveAddToPlMessage : public eveMessage
{
public:
	eveAddToPlMessage(QString, QString, QByteArray, int prio=0);
	virtual ~eveAddToPlMessage();
	QString * getXmlName(){return XmlName;};
	QString * getXmlAuthor(){return XmlAuthor;};
	QByteArray * getXmlData(){return XmlData;};
	bool compare(eveMessage *);
	virtual eveAddToPlMessage* clone(){return new eveAddToPlMessage(*XmlName, *XmlAuthor, *XmlData, priority);};


protected:
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
	eveCurrentXmlMessage(QString, QString, QByteArray, int prio=0);
	virtual ~eveCurrentXmlMessage();
	eveCurrentXmlMessage* clone(){return new eveCurrentXmlMessage(*XmlName, *XmlAuthor, *XmlData, priority);};
};

enum engineStatusT {eveEngIDLENOXML=1, eveEngIDLEXML, eveEngLOADINGXML, eveEngEXECUTING, eveEngPAUSED, eveEngSTOPPED, eveEngHALTED} ;
/**
 * \brief a message containing the status of the currently processing scanmodule
 *
 * possible status values are:
 * eveEngIDLENOXML: No XML is loaded, engine has just be started or all current chains have been done
 * eveEngIDLEXML: XML is loaded, but chains are not yet executing
 * eveEngLOADINGXML: XML is currently loading
 * eveEngEXECUTING: at least one chain is executing (execution may be paused)
 * eveEngPAUSED: Pause has been activated
 * eveEngSTOPPED: Stop has been activated
 * eveEngHALTED: Halt has been activated
 */
class eveEngineStatusMessage : public eveMessage
{
public:
	eveEngineStatusMessage(int, QString, int prio=0, int dest=0);
	virtual ~eveEngineStatusMessage();
	int getStatus(){return estatus;};
	QString * getXmlId(){return XmlId;};
	eveTime getTime(){return timestamp;};
	bool compare(eveMessage *);
	eveEngineStatusMessage* clone(){return new eveEngineStatusMessage(estatus, *XmlId, priority, destination);};

private:
	eveTime timestamp;
	int estatus;
	QString * XmlId;
};

enum chainStatusT {eveChainSmIDLE=1, eveChainSmINITIALIZING, eveChainSmEXECUTING, eveChainSmPAUSED, eveChainSmTRIGGERWAIT, eveChainSmDONE, eveChainDONE, eveChainSTORAGEDONE};
/**
 * \brief a message containing the status of the currently processing chain
 *
 * possible status values are
 * eveChainSmIDLE: Chain has not been started yet
 * eveChainSmINITIALIZING: scanmodule is initializing, but may not yet started
 * eveChainSmEXECUTING: scanmodule (and chain) is executing
 * eveChainSmPAUSED: scanmodule (and chain) has been paused
 * eveChainSmTRIGGERWAIT: scanmodule waits for manual trigger
 * eveChainSmDONE: a scanmodule has finished
 * eveChainDONE: the chain ( all scanmodules) has finished
 */
class eveChainStatusMessage : public eveMessage
{
public:
	eveChainStatusMessage(chainStatusT, int, int, int);
	eveChainStatusMessage(chainStatusT, int, int, int, eveTime, int, int prio=0, int dest=0);
	virtual ~eveChainStatusMessage();
	chainStatusT getStatus(){return cstatus;};
	void setStatus(chainStatusT stat){cstatus=stat;};
	int getChainId(){return chainId;};
	int getSmId(){return smId;};
	int getPosCnt(){return posCounter;};
	eveTime getTime(){return timestamp;};
	int getRemainingTime(){return remainingTime;};
	bool compare(eveMessage *);
	eveChainStatusMessage* clone(){return new eveChainStatusMessage(cstatus, chainId, smId, posCounter, timestamp, remainingTime, priority, destination);};

private:
	eveTime timestamp;
	chainStatusT cstatus;
	int chainId;
	int smId;
	int posCounter;
	int remainingTime;
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
	eveRequestMessage* clone(){return new eveRequestMessage(requestId, requestType, requestString);};

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
	eveRequestCancelMessage* clone(){return new eveRequestCancelMessage(requestId);};

private:
	int requestId;
};

/**
 * \brief Error message
 */
class eveErrorMessage : public eveMessage
{
public:
	eveErrorMessage(int, int, int, QString, int prio=0);
	virtual ~eveErrorMessage();

	int getSeverity(){return severity;};
	int getFacility(){return facility;};
	int getErrorType(){return errorType;};
	eveTime getTime(){return timestamp;};
	QString getErrorText(){return errorString;};
	bool compare(eveMessage *);
	eveErrorMessage* clone(){return new eveErrorMessage(severity, facility, errorType, errorString, priority);};

private:
	int severity;
	int facility;
	int errorType;
	eveTime timestamp;
	QString errorString;
};

/**
 * \brief base class for data / devinfo message
 */
class eveBaseDataMessage : public eveMessage
{
public:
	eveBaseDataMessage(int, int prio=0, int dest=0);
	eveBaseDataMessage(int, int, int, QString, QString, int prio=0, int dest=0);
	virtual ~eveBaseDataMessage();
	virtual eveBaseDataMessage* clone();
	virtual bool compare(eveBaseDataMessage *);

	int getChainId(){return chainId;};
	void setChainId(int id){chainId = id;};
	int getSmId(){return smId;};
	void setSmId(int id){smId = id;};
	QString getXmlId(){return xmlId;};
	void setXmlId(QString id){xmlId = id;};
	QString getName(){return name;};
	void setName(QString id){name = id;};

protected:
	int chainId;
	int smId;
	QString name;
	QString xmlId;
};

/**
 * \brief message containing measured compound data
 */
// TODO ident is name ?? ==> redundant!
class eveDataMessage : public eveBaseDataMessage
{
public:
//	eveDataMessage(QString, eveDataStatus, int prio=0, int dest=0);
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QVector<int>, int prio=0, int dest=0 );
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QVector<short>, int prio=0, int dest=0 );
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QVector<signed char>, int prio=0, int dest=0 );
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QVector<float>, int prio=0, int dest=0 );
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QVector<double>, int prio=0, int dest=0 );
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QStringList, int prio=0, int dest=0 );
	eveDataMessage(QString, eveDataStatus, eveDataModType, eveTime, QDateTime, int prio=0, int dest=0 );
	virtual ~eveDataMessage();

	const QVector<int>& getIntArray(){return dataArrayInt;};
	const QVector<short>& getShortArray(){return dataArrayShort;};
	const QVector<signed char>& getCharArray(){return dataArrayChar;};
	const QVector<float>& getFloatArray(){return dataArrayFloat;};
	const QVector<double>& getDoubleArray(){return dataArrayDouble;};
	const QStringList& getStringArray(){return dataStrings;};
	QString getId(){return ident;};
	eveDataStatus getDataStatus(){return dataStatus;};
	eveType getDataType(){return dataType;};
	eveDataModType getDataMod(){return dataModifier;};
	eveTime getDataTimeStamp(){return timestamp;};
	eveTime getDateTime(){return eveTime::eveTimeFromDateTime(dateTime);};
	bool isEmpty(){return !(arraySize);};
	int getArraySize(){return arraySize;};
	int getPositionCount(){return posCount;};
	void setPositionCount(int pc){posCount = pc;};
	eveDataMessage* clone();
	eveVariant toVariant();

private:
	int posCount;
	QString ident;
	eveTime timestamp;
	quint32 arraySize;
	eveDataStatus dataStatus;				// epicsSeverity, SeverityCondition, AcquisitionStatus
	eveType dataType;
	eveDataModType dataModifier;
	QVector<int> dataArrayInt;
	QVector<short> dataArrayShort;
	QVector<signed char> dataArrayChar;
	QVector<float> dataArrayFloat;
	QVector<double> dataArrayDouble;
	QStringList dataStrings;
	QDateTime dateTime;
};

/**
 * \brief device info message
 */
class eveDevInfoMessage : public eveBaseDataMessage
{
public:
	eveDevInfoMessage(QStringList*, int prio=0, int dest=0);
	eveDevInfoMessage(int, int, QString, QString, eveType, bool, QStringList*, int prio=0, int dest=0);
	eveDevInfoMessage(QString, QString, QStringList*, int prio=0, int dest=0);
	virtual ~eveDevInfoMessage();
	QStringList* getText(){return infoList;};
	bool isArray(){return isarray;};
	eveType getDataType(){return dataType;};
	eveDevInfoMessage* clone();
	bool compare(eveDevInfoMessage*);

private:
	QStringList *infoList;
	eveType dataType;
	bool isarray;
};

/**
 * \brief message containing the current playlist
 */
class evePlayListMessage : public eveMessage
{
public:
	evePlayListMessage(const QList<evePlayListEntry> , int prio=0);
	~evePlayListMessage();
	int getCount(){return plList->size();};
	evePlayListEntry & getEntry(int);
	bool compare(eveMessage *);
	evePlayListMessage* clone();

private:
	QList<evePlayListEntry> *plList;
	evePlayListEntry entry;
};

/**
 * \brief message containing storage parameters (will change in future)
 */
class eveStorageMessage : public eveMessage
{
public:
	eveStorageMessage(int, int, QHash<QString, QString>*, int prio=0, int dest=0);
	~eveStorageMessage();
	bool compare(eveMessage *);
	eveStorageMessage* clone();
	int getChainId(){return chainId;};
	int getChannelId(){return channelId;};
	QString getFileName(){return filename;};
	QHash<QString, QString>* takeHash();

private:
	int chainId;
	int channelId;
	QString filename;
	QHash<QString, QString>* paraHash;
};


#endif /*EVEMESSAGE_H_*/
