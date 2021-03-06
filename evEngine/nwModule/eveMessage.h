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
#include "eveDataStatus.h"

#define EVEMESSAGE_STARTTAG 0x0d0f0d0a
#define EVEMESSAGE_VERSION 0x0200
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
#define EVEMESSAGETYPE_REPEATCOUNT 0x0014

#define EVEMESSAGETYPE_ERROR 0x0101
#define EVEMESSAGETYPE_ENGINESTATUS 0x0102
#define EVEMESSAGETYPE_DATA 0x0104
#define EVEMESSAGETYPE_BASEDATA 0x0105
#define EVEMESSAGETYPE_REQUEST 0x0108
#define EVEMESSAGETYPE_REQUESTCANCEL 0x0109
#define EVEMESSAGETYPE_METADATA 0x010a
#define EVEMESSAGETYPE_CURRENTXML 0x0110
#define EVEMESSAGETYPE_STORAGEDONE 0x0111
#define EVEMESSAGETYPE_CHAINSTATUS 0x0112
#define EVEMESSAGETYPE_VERSION 0x0113
#define EVEMESSAGETYPE_CHAINPROGRESS 0x0114

#define EVEMESSAGETYPE_PLAYLIST 0x0301

#define EVEMESSAGETYPE_ADDTOPLAYLIST 0x0201
#define EVEMESSAGETYPE_REMOVEFROMPLAYLIST 0x0202
#define EVEMESSAGETYPE_REORDERPLAYLIST 0x0203
#define EVEMESSAGETYPE_STORAGECONFIG 0x1000
#define EVEMESSAGETYPE_DEVINFO 0x1002
#define EVEMESSAGETYPE_EVENTREGISTER 0x1003
#define EVEMESSAGETYPE_MONITORREGISTER 0x1004
#define EVEMESSAGETYPE_DETECTORREADY 0x1005

#define EVEREQUESTTYPE_YESNO 0x00
#define EVEREQUESTTYPE_OKCANCEL 0x01
#define EVEREQUESTTYPE_INT 0x02
#define EVEREQUESTTYPE_FLOAT 0x03
#define EVEREQUESTTYPE_TEXT 0x04
#define EVEREQUESTTYPE_ERRORTEXT 0x05
#define EVEREQUESTTYPE_TRIGGER 0x10

#define EVEMESSAGEPRIO_NORMAL 0x0
#define EVEMESSAGEPRIO_HIGH 0x01

#define EVEMESSAGESEVERITY_SYSTEM 0x00
#define EVEMESSAGESEVERITY_FATAL 0x01
#define EVEMESSAGESEVERITY_ERROR 0x02
#define EVEMESSAGESEVERITY_MINOR 0x03
#define EVEMESSAGESEVERITY_INFO 0x04
#define EVEMESSAGESEVERITY_DEBUG 0x05

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
#define EVEMESSAGEFACILITY_MATH 0x18
#define EVEMESSAGEFACILITY_XMLVALIDATOR 0x19

#define EVEERRORMESSAGETYPE_FILENAME 0x000a
#define EVEERRORMESSAGETYPE_TOTALCOUNT 0x000b

#define EVEERROR_TIMEOUT 0x0009

#define EVEENGINESTATUS_IDLENOXML 0x00 		// Idle, no XML-File loaded
#define EVEENGINESTATUS_IDLEXML 0x01		// Idle, XML loaded
#define EVEENGINESTATUS_LOADING 0x02		// loading XML
#define EVEENGINESTATUS_EXECUTING 0x03		// executing
#define EVEENGINESTATUS_AUTOSTART 0x0100		// autostart bit

struct evePlayListEntry {
        unsigned int pid;
        QString name;
        QString author;
};

enum eveDataModType {DMTunmodified, DMTcenter, DMTedge, DMTmin, DMTmax, DMTfwhm, DMTmean, DMTstandarddev, DMTsum, DMTnormalized, DMTpeak, DMTdeviceData, DMTaverageParams, DMTmetaData, DMTunknown};

/**
 * \brief base class for errorMessage, engineStatus, chainStatus, dataMessage ...
 */
class eveMessage
{
public:
	eveMessage();
	eveMessage(int, int prio=0, int dest = 0);
	virtual ~eveMessage();

	int getType(){return type;};
	int getPriority(){return priority;};
	void setPriority(int prio){priority = prio;};
    int getDestinationChannel(){return destination;};
    void setDestinationChannel(int channel){destination = channel;};
    int getDestinationFacility(){return facility;};
    void setDestinationFacility(int fac){facility = fac;};
    void addDestinationFacility(int fac){facility |= fac;};
    bool hasDestinationFacility(int fac){return facility & fac;};
    virtual bool compare(eveMessage *);
	void dump();
	virtual eveMessage* clone(){return new eveMessage(type, priority, destination);};
protected:
	int destination;
	int type;
	int priority;
    int facility;
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
	void dump();
	virtual eveMessageText* clone(){return new eveMessageText(type,messageText, priority);};

private:
	QString messageText;
};

/**
 * \brief a message containing a list of text
 */
class eveMessageTextList : public eveMessage
{
public:
	eveMessageTextList(int, QStringList&, int prio=0);
	virtual ~eveMessageTextList(){};
	QStringList& getText(){return messageTextList;};
	bool compare(eveMessage *);
	virtual eveMessageTextList* clone(){return new eveMessageTextList(type,messageTextList, priority);};
	int getChainId(){return chainId;};
	void setChainId(int id){chainId = id;};

private:
	QStringList messageTextList;
	int chainId;
};

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
	virtual eveMessageInt* clone(){return new eveMessageInt(type,value, priority, destination);}

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
	virtual eveMessageIntList* clone(){return new eveMessageIntList(type,ivalue1,ivalue2, priority);};

private:
	int ivalue1;
	int ivalue2;
};

/**
 * \brief a message to send the current version
 */
class eveVersionMessage : public eveMessage
{
public:
    eveVersionMessage();
    eveVersionMessage(quint32, quint32, quint32);
    virtual ~eveVersionMessage(){};
    quint32 getVersion(){return version;};
    quint32 getRevision(){return revision;};
    quint32 getPatch(){return patch;};
    bool compare(eveMessage *) {return false;};
    virtual eveVersionMessage* clone(){return new eveVersionMessage(*this);};

private:
    quint32 version;
    quint32 revision;
    quint32 patch;
};

/**
 * \brief a message to add an entry to the playlist
 */
class eveAddToPlMessage : public eveMessage
{
public:
	eveAddToPlMessage(QString, QString, QByteArray, int prio=0);
	virtual ~eveAddToPlMessage();
	QString getXmlName(){return XmlName;};
	QString getXmlAuthor(){return XmlAuthor;};
	QByteArray getXmlData(){return XmlData;};
	bool compare(eveMessage *);
//	virtual eveAddToPlMessage* clone(){return new eveAddToPlMessage(*this);};


protected:
	QString XmlName;
	QString XmlAuthor;
	QByteArray XmlData;
};

/**
 * \brief a message to send the current XML-description
 */
class eveCurrentXmlMessage : public eveAddToPlMessage
{
public:
	eveCurrentXmlMessage(QString, QString, QByteArray, int prio=0);
	virtual ~eveCurrentXmlMessage();
	virtual eveCurrentXmlMessage* clone(){return new eveCurrentXmlMessage(XmlName, XmlAuthor, XmlData, priority);};
};

/**
 *\brief status of engine
 */
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
	eveEngineStatusMessage(unsigned int, QString, int prio=0, int dest=0);
	virtual ~eveEngineStatusMessage();
	unsigned int getStatus(){return estatus;};
	QString getXmlId(){return XmlId;};
	eveTime getTime(){return timestamp;};
	bool compare(eveMessage *);
	virtual eveEngineStatusMessage* clone(){return new eveEngineStatusMessage(estatus, XmlId, priority, destination);};

private:
	eveTime timestamp;
	unsigned int estatus;
	QString XmlId;
};

/**
 *\brief chain progress message
 */
class eveChainProgressMessage : public eveMessage
{
public:
    eveChainProgressMessage(int, int, eveTime, int, int prio=0, int dest=0);
    virtual ~eveChainProgressMessage(){};
	int getChainId(){return chainId;};
	int getPosCnt(){return posCounter;};
	eveTime getTime(){return timestamp;};
	int getRemainingTime(){return remainingTime;};
    bool compare(eveMessage *){return false;};
    virtual eveChainProgressMessage* clone(){return new eveChainProgressMessage(*this);};

private:
	eveTime timestamp;
	int chainId;
	int posCounter;
	int remainingTime;
};

enum CHStatusT {CHStatusUNKNOWN, CHStatusIDLE, CHStatusExecuting, CHStatusDONE, CHStatusSTORAGEDONE, CHStatusMATHDONE, CHStatusALLDONE};
enum SMStatusT {SMStatusNOTSTARTED, SMStatusINITIALIZING, SMStatusEXECUTING, SMStatusPAUSE, SMStatusTRIGGERWAIT, SMStatusAPPEND, SMStatusDONE} ;
enum SMReasonT {SMReasonNone, SMReasonSMREDOACTIVE, SMReasonCHREDOACTIVE, SMReasonSMPAUSE, SMReasonCHPAUSE, SMReasonGUIPAUSE, SMReasonSMSKIP,
                SMReasonCHSKIP, SMReasonGUISKIP, SMReasonCHSTOP, SMReasonGUISTOP};

class eveChainStatus;
/**
 * \brief a message containing the extended status of chain and scan modules
 *
 */
class eveChainStatusMessage : public eveMessage
{
public:
    eveChainStatusMessage(int, eveChainStatus& chstatus);
    eveChainStatusMessage(int, CHStatusT cstat);
    virtual ~eveChainStatusMessage();
    int getChainId(){return chainId;};
    eveTime getTime(){return timestamp;};
    int getLastSmId(){return lastSMsmid;};
    SMStatusT getLastSmStatus(){return lastSMStatus;};
    CHStatusT getChainStatus(){return ecstatus;};
    QHash<int, quint32>& getSMStatusHash(){return esmstatus;};
    bool isDoneSM(){return lastSMisDone;};
    bool isSmStarting(){return lastSmStarted;};
    bool compare(eveMessage *);
    virtual eveChainStatusMessage* clone(){return new eveChainStatusMessage(*this);};

private:
    eveTime timestamp;
    CHStatusT ecstatus;
    int chainId;
    QHash<int, quint32> esmstatus;
    int lastSMsmid;
    SMStatusT lastSMStatus;
    bool lastSMisDone;
    bool lastSmStarted;
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
	virtual eveRequestMessage* clone(){return new eveRequestMessage(requestId, requestType, requestString);};

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
	//int getAnswerInt(){return answerInt;};
        bool getAnswerBool(){return answerBool;};
        bool compare(eveMessage *);
	virtual eveRequestAnswerMessage* clone(){return new eveRequestAnswerMessage(*this);};

private:
	int requestId;
	int requestType;
	QString answerString;
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
	virtual eveRequestCancelMessage* clone(){return new eveRequestCancelMessage(requestId);};

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
	virtual eveErrorMessage* clone(){return new eveErrorMessage(severity, facility, errorType, errorString, priority);};

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
	eveBaseDataMessage(int, int, int, QString, QString, QString, eveDataModType mod=DMTunmodified, int msecs=-1, int prio=0, int dest=0);
	eveBaseDataMessage(int, int, int, QString, QString, int prio=0, int dest=0);
	virtual ~eveBaseDataMessage();
    virtual eveBaseDataMessage* clone()=0;
    virtual bool compare(eveMessage *)=0;
	int getChainId(){return chainId;};
	void setChainId(int id){chainId = id;};
	int getSmId(){return smId;};
	void setSmId(int id){smId = id;};
	QString getXmlId(){return xmlId;};
	void setXmlId(QString id){xmlId = id;};
	QString getName(){return name;};
	void setName(QString id){name = id;};
	void setAuxString(QString aux){auxInfo = aux;};
	QString getAuxString(){return auxInfo;};
	void setNormalizeId(QString nId){normalizeId = nId;};
	QString getNormalizeId(){return normalizeId;};
    void setStorageHint(QString hint){storageHint = hint;};
    QString getStorageHint(){return storageHint;};
    eveDataModType getDataMod(){return dataModifier;};
	void setDataMod(eveDataModType mod){dataModifier=mod;};
	int getMSecsSinceStart(){return mSecsSinceStart;};

protected:
	void setMSecsSinceStart(int msecs){mSecsSinceStart = msecs;};
	int chainId;
	int smId;
	eveDataModType dataModifier;
	QString name;
	QString xmlId;
	QString auxInfo;
	QString normalizeId;
    QString storageHint;
    int mSecsSinceStart;
};

/**
 * \brief message containing measured compound data
 */
class eveDataMessage : public eveBaseDataMessage
{
public:
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QVector<int>, int prio=0, int dest=0 );
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QVector<short>, int prio=0, int dest=0 );
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QVector<signed char>, int prio=0, int dest=0 );
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QVector<float>, int prio=0, int dest=0 );
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QVector<double>, int prio=0, int dest=0 );
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QStringList, int prio=0, int dest=0 );
	eveDataMessage(QString, QString, eveDataStatus, eveDataModType, eveTime, QDateTime, int prio=0, int dest=0 );
	virtual ~eveDataMessage();

	const QVector<int>& getIntArray(){return dataArrayInt;};
	const QVector<short>& getShortArray(){return dataArrayShort;};
	const QVector<signed char>& getCharArray(){return dataArrayChar;};
	const QVector<float>& getFloatArray(){return dataArrayFloat;};
	const QVector<double>& getDoubleArray(){return dataArrayDouble;};
	const QStringList& getStringArray(){return dataStrings;};
	eveDataStatus getDataStatus(){return dataStatus;};
	eveType getDataType(){return dataType;};
	eveTime getDataTimeStamp(){return timestamp;};
	eveTime geteveDT(){return eveTime::eveTimeFromDateTime(dateTime);};
    QStringList getAttributes(){return attribList;};
    void addAttribute(QString attribute){attribList.append(attribute);};
    void invalidate();

	bool isEmpty(){return !(arraySize);};
	int getArraySize(){return arraySize;};
	int getPositionCount(){return posCount;};
	void setPositionCount(int pc){posCount = pc;};
	virtual eveDataMessage* clone();
	eveVariant toVariant();
	virtual bool compare(eveMessage *);
	bool isInteger();
	bool isFloat();
    // TODO needs to be inline for hdf5plugin
    int getBufferLength(){
        int bufferSize = 0;

        switch (dataType) {
        case eveInt8T:
        case eveUInt8T:
            bufferSize = 1;
            break;
        case eveInt16T:
        case eveUInt16T:
            bufferSize = 2;
            break;
        case eveInt32T:
        case eveUInt32T:
        case eveFloat32T:
            bufferSize = 4;
            break;
        case eveFloat64T:
            bufferSize = 8;
            break;
        default:
            break;
        }
        bufferSize *= arraySize;

        return bufferSize;
    };
    void* getBufferAddr(){

        switch (dataType) {
        case eveInt8T:
        case eveUInt8T:
            return (void*) dataArrayChar.constData();
            break;
        case eveInt16T:
        case eveUInt16T:
            return (void*) dataArrayShort.constData();
            break;
        case eveInt32T:
        case eveUInt32T:
            return (void*) dataArrayInt.constData();
            break;
        case eveFloat32T:
            return (void*) dataArrayFloat.constData();
            break;
        case eveFloat64T:
            return (void*) dataArrayDouble.constData();
            break;
        default:
            return NULL;
        }
    };

private:
	int posCount;
	eveTime timestamp;
	quint32 arraySize;
	eveDataStatus dataStatus;				// epicsSeverity, SeverityCondition, AcquisitionStatus
	eveType dataType;
	QVector<int> dataArrayInt;
	QVector<short> dataArrayShort;
	QVector<signed char> dataArrayChar;
	QVector<float> dataArrayFloat;
	QVector<double> dataArrayDouble;
	QStringList dataStrings;
	QDateTime dateTime;
    QStringList attribList;
};

/**
 * \brief device info message
 */
class eveDevInfoMessage : public eveBaseDataMessage
{
public:
	eveDevInfoMessage(QStringList*, int prio=0, int dest=0);
	eveDevInfoMessage(int, int, QString, QString, eveType, bool, eveDataModType, QString, QStringList*, int prio=0, int dest=0);
	eveDevInfoMessage(QString, QString, QStringList*, int prio=0, int dest=0);
	eveDevInfoMessage(QString, QString, QStringList*, eveDataModType, QString, int prio=0, int dest=0);
	virtual ~eveDevInfoMessage();
	QStringList* getText(){return infoList;};
	bool isArray(){return isarray;};
	eveType getDataType(){return dataType;};
	virtual eveDevInfoMessage* clone();
	bool compare(eveMessage*);

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
	evePlayListMessage(evePlayListMessage&);
	virtual ~evePlayListMessage();
	int getCount(){return plList.size();};
	evePlayListEntry getEntry(int);
	bool compare(eveMessage *);
//	virtual evePlayListMessage* clone();
	void dump();

private:
	QList<evePlayListEntry> plList;
};

/**
 * \brief message containing storage parameters (will change in future)
 */
class eveStorageMessage : public eveMessage
{
public:
	eveStorageMessage(int, int, QHash<QString, QString>&, int prio=0, int dest=0);
	virtual ~eveStorageMessage();
	bool compare(eveMessage *);
	virtual eveStorageMessage* clone();
	int getChainId(){return chainId;};
	int getChannelId(){return channelId;};
	QString getFileName(){return filename;};
	QHash<QString, QString>& getHash();
	void dump();

private:
	int chainId;
	int channelId;
	QString filename;
	QHash<QString, QString> paraHash;
};


#endif /*EVEMESSAGE_H_*/
