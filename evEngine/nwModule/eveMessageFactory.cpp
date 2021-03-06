
#include <QByteArray>
#include <QDataStream>
#include <QString>

#include "eveMessageFactory.h"
#include "eveChainStatus.h"
#include "eveError.h"

eveMessageFactory::eveMessageFactory()
{
}

eveMessageFactory::~eveMessageFactory()
{
}

/**
 * \brief build a message from stream data
 * \param type		type of message to build
 * \param length	length of data stream
 * \param byteArray the data stream
 * \return the corresponding message or an errorMessage if an error occured
 */
eveMessage * eveMessageFactory::getNewMessage(quint16 type, quint32 length, QByteArray *byteArray){

	eveMessage * message=NULL;

	if (length > 0){
		if (byteArray == 0) eveError::log(ERROR,"eveMessageFactory::getNewMessage byteArray is NullPointer but length > 0");
		if ((quint32)byteArray->length() != length) {
			eveError::log(ERROR,"eveMessageFactory::getNewMessage length of byteArray != length");
			return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "length mismatch");
		}
	}
	QDataStream inStream(*byteArray);
	inStream.setVersion(QDataStream::Qt_4_0);


	switch (type) {
    case EVEMESSAGETYPE_VERSION:
    {
        quint32 version;
        quint32 revision;
        quint32 patch;

        inStream >> version >> revision >> patch;
        message = new eveVersionMessage(version, revision, patch);
    }
    break;
        case EVEMESSAGETYPE_ADDTOPLAYLIST:
		case EVEMESSAGETYPE_CURRENTXML:
		{
			QString xmlName, xmlAuthor;
			QByteArray xmlData;

			inStream >> xmlName >> xmlAuthor >> xmlData;
			if (2*xmlName.length() + 2*xmlAuthor.length() + xmlData.length()+ 12 != (qint32) length) {
				eveError::log(ERROR,"eveMessageFactory::getNewMessage malformed EVEMESSAGETYPE_CURRENTXML / EVEMESSAGETYPE_ADDTOPLAYLIST");
				return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "malformed EVEMESSAGETYPE_CURRENTXML / EVEMESSAGETYPE_ADDTOPLAYLIST");
			}
			if (type == EVEMESSAGETYPE_CURRENTXML)
				message = new eveCurrentXmlMessage(xmlName, xmlAuthor, xmlData);
			else
				message = new eveAddToPlMessage(xmlName, xmlAuthor, xmlData);
		}
		break;
		case EVEMESSAGETYPE_START:
		case EVEMESSAGETYPE_HALT:
		case EVEMESSAGETYPE_BREAK:
		case EVEMESSAGETYPE_STOP:
		case EVEMESSAGETYPE_PAUSE:
		case EVEMESSAGETYPE_ENDPROGRAM:
            message = new eveMessage(type);
		break;
		case EVEMESSAGETYPE_LIVEDESCRIPTION:
		{
			QString messageText;
			inStream >> messageText;
			message = new eveMessageText(EVEMESSAGETYPE_LIVEDESCRIPTION, messageText);
		}
		break;
		case EVEMESSAGETYPE_AUTOPLAY:
        case EVEMESSAGETYPE_REPEATCOUNT:
        case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
		{
			int intValue;
			inStream >> intValue;
            message = new eveMessageInt(type, intValue);
		}
		break;
		case EVEMESSAGETYPE_REORDERPLAYLIST:
		{
			int ival1, ival2 ;
			inStream >> ival1 >> ival2;
			message = new eveMessageIntList(EVEMESSAGETYPE_REORDERPLAYLIST, ival1, ival2);
		}
		break;
		case EVEMESSAGETYPE_REQUEST:
		{
			quint32 rid;
			quint32 rtype;
			QString rtext;

			inStream >> rid >> rtype >> rtext;
			message = new eveRequestMessage(rid, rtype, rtext);
		}
		break;
		case EVEMESSAGETYPE_REQUESTANSWER:
		{
			int rid, rtype;

			inStream >> rid >> rtype;
			if ((rtype == EVEREQUESTTYPE_YESNO) || (rtype == EVEREQUESTTYPE_OKCANCEL)){
                                bool answer = false;
                                int intAnswer;
                                inStream >> intAnswer;
                                if (intAnswer != 0) answer = true;
				message = new eveRequestAnswerMessage(rid, rtype, answer);
			}
			else if ((rtype == EVEREQUESTTYPE_INT) || (rtype == EVEREQUESTTYPE_TRIGGER)){
				int answer;
				inStream >> answer;
				message = new eveRequestAnswerMessage(rid, rtype, answer);
			}
			else if (rtype == EVEREQUESTTYPE_FLOAT){
				float answer;
				inStream >> answer;
				message = new eveRequestAnswerMessage(rid, rtype, answer);
			}
			else if ((rtype == EVEREQUESTTYPE_TEXT) || (rtype == EVEREQUESTTYPE_ERRORTEXT)){
				QString answer;
				inStream >> answer;
				if (2*answer.length() + 12 != (qint32) length) {
					eveError::log(ERROR,"eveMessageFactory::getNewMessage: malformed EVEMESSAGETYPE_REQUESTANSWER");
					return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "malformed EVEMESSAGETYPE_REQUESTANSWER");
				}
				message = new eveRequestAnswerMessage(rid, rtype, answer);
			}
			else {
				return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, QString("unknown request type: %1").arg(rtype));
			}
		}
		break;
		case EVEMESSAGETYPE_REQUESTCANCEL:
		{
			quint32 rid;

			inStream >> rid;
			message = new eveRequestCancelMessage(rid);
		}
		break;
		case EVEMESSAGETYPE_ERROR:
		{
			quint8 severity, facility;
			quint16 errType ;
			QString errText;
			quint32 timeFirst,timeSecond;
			inStream >> timeFirst >> timeSecond >> severity >> facility >> errType >> errText;
			message = new eveErrorMessage(severity,facility,errType,errText);
		}
		break;
		case EVEMESSAGETYPE_ENGINESTATUS:
		{
			quint32 estatus ;
			QString xmlId;
			quint32 timeFirst,timeSecond;
			inStream >> timeFirst >> timeSecond >> estatus >> xmlId;
			message = new eveEngineStatusMessage(estatus, xmlId);
		}
		break;
		case EVEMESSAGETYPE_CHAINPROGRESS:
		{
			struct timespec statTime;
			quint32 secs, nsecs, cstatus, cid, smid, poscnt, remtime;
			inStream >> secs >> nsecs >> cstatus >> cid >> smid >> poscnt >> remtime;
			statTime.tv_sec = secs;
			statTime.tv_nsec = nsecs;
			eveTime messageTime(statTime);
            message = new eveChainProgressMessage(cid, poscnt, messageTime, remtime);
		}
		break;
    case EVEMESSAGETYPE_CHAINSTATUS:
    {
        struct timespec statTime;
        int smid;
        quint32 secs, nsecs, cstatus, cid, smstatus;
        QHash<int, quint32> smstatusHash;

        inStream >> secs >> nsecs >> cid >> cstatus;
        quint32 msglen = length - 16;
        while (msglen > 0 && (msglen % 8 == 0)){
            inStream >> smid >> smstatus;
            smstatusHash.insert(smid, smstatus);
            msglen -= 8;
        }
        statTime.tv_sec = secs;
        statTime.tv_nsec = nsecs;
        eveTime messageTime(statTime);
        eveChainStatus chstatus((CHStatusT) cstatus, smstatusHash);
        message = new eveChainStatusMessage(cid, chstatus);
    }
    break;
        case EVEMESSAGETYPE_PLAYLIST:
		{
			quint32 sum = 0;
			qint32 plcount;
			QList<evePlayListEntry> plList;
			evePlayListEntry plEntry;

			inStream >> plcount;
                        eveError::log(DEBUG, QString("eveMessageFactory::getNewMessage: EVEMESSAGETYPE_PLAYLIST plcount: %1").arg(plcount));
			sum += 4;
			for (int i=0; i < plcount; ++i ){
				if (length < sum +12) {
					return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "corrupt playlist message");
				}
				inStream >> plEntry.pid >> plEntry.name >> plEntry.author;
				plList.append(plEntry);
				sum += 12 + 2*plEntry.name.length() + 2*plEntry.author.length();
			}
			if (length == sum) {
				message = new evePlayListMessage(plList);
			}
			else {
				return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "corrupt playlist message (length mismatch)");
			}

			//for (int i=0; i < plList.size(); ++i) delete &(plList.takeFirst());
		}
		break;
		case EVEMESSAGETYPE_DATA:
		{
			quint32 chid, smid, poscounter, dtype;
			quint32 dataMod;
			quint32 seconds, nanoseconds;
			QString xmlId;
			QString name = "";
			quint8 severity;
			quint8 condition;
			quint16 acqStatus;

			inStream >> chid >> smid >> poscounter;
			inStream >> dtype >> dataMod >> severity >> condition >> acqStatus;
			inStream >> seconds >> nanoseconds >> xmlId;
			eveTime timeStamp = eveTime::eveTimeFromTime_tNano(seconds, nanoseconds);
			eveDataStatus dstatus = eveDataStatus(severity, condition, acqStatus);

			switch (dtype) {
				case eveInt8T:					/* eveInt8 */
				{
					QVector<qint8> cArray;
					inStream >> cArray;
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, cArray);
				}
				break;
				case eveInt16T:					/* eveInt16 */
				{
					QVector<short> sArray;
					inStream >> sArray;
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, sArray);
				}
				break;
				case eveInt32T:					/* eveInt32 */
				{
					QVector<int> iArray;
					inStream >> iArray;
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, iArray);
				}
				break;
				case eveFloat32T:					/* eveFLoat32 */
				{
					QVector<float> fArray;
					inStream >> fArray;
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, fArray);
				}break;
				case eveFloat64T:					/* eveFloat64 */
				{
					QVector<double> dArray;
					inStream >> dArray;
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, dArray);
				}
				break;
				case eveStringT:					/* eveString */
				{
					QStringList stringData;
					inStream >> stringData;
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, stringData);
				}
				break;
				case eveDateTimeT:					/* eveString */
				{
					QStringList stringData;
					inStream >> stringData;
					QDateTime dt = QDateTime::fromString(stringData.at(0), "HH:mm:ss.zzz");
					if (!dt.isValid()) dt = QDateTime();
					message = new eveDataMessage(xmlId, name, dstatus, (eveDataModType)dataMod, timeStamp, dt);
				}
				break;
			}
			if (message != NULL){
				((eveDataMessage*)message)->setChainId(chid);
				((eveDataMessage*)message)->setSmId(smid);
				((eveDataMessage*)message)->setPositionCount(poscounter);
			}
		}
		break;
		default:
			eveError::log(ERROR,QString("eveMessageFactory::getNewMessage: unknown message type %1").arg(type));
			return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "unknown message type");
			break;
	}
	return message;
}

/**
 * \brief create a data stream from a message
 * \param message	message to be put to a stream
 * \return the data stream
 */
QByteArray * eveMessageFactory::getNewStream(eveMessage *message){

	quint32 starttag = EVEMESSAGE_STARTTAG;
	quint16 version = EVEMESSAGE_VERSION;
	quint16 messageType = message->getType();
    quint32 messageLength = 0;

	QByteArray * block = new QByteArray();
    QDataStream outStream(block, QIODevice::WriteOnly);
    outStream.setVersion(QDataStream::Qt_4_0);

    outStream << starttag << version << messageType;

	switch(messageType) {
    case EVEMESSAGETYPE_VERSION:
    {
        eveVersionMessage * vMsg = (eveVersionMessage*)message;
        outStream << messageLength << vMsg->getVersion() << vMsg->getRevision() << vMsg->getPatch();
    }
        break;
        case EVEMESSAGETYPE_ERROR:
		{
			eveTime errTime = ((eveErrorMessage*)message)->getTime();
			quint8 severity = ((eveErrorMessage*)message)->getSeverity();
			quint8 facility = ((eveErrorMessage*)message)->getFacility();
			quint16 errType = ((eveErrorMessage*)message)->getErrorType();
			QString errText = ((eveErrorMessage*)message)->getErrorText();
			outStream << messageLength << errTime.seconds() << errTime.nanoSeconds() << severity << facility << errType << errText;
		}
			break;
		case EVEMESSAGETYPE_ENGINESTATUS:
		{
			eveTime statTime = ((eveEngineStatusMessage*)message)->getTime();
			QString xmlId = ((eveEngineStatusMessage*)message)->getXmlId();
			quint32 estatus = ((eveEngineStatusMessage*)message)->getStatus();
			outStream << messageLength << statTime.seconds() << statTime.nanoSeconds() << estatus << xmlId;
		}
		break;
		case EVEMESSAGETYPE_CHAINPROGRESS:
		{
			eveTime statTime = ((eveChainProgressMessage*)message)->getTime();
			quint32 cid  = ((eveChainProgressMessage*)message)->getChainId();
			quint32 poscnt = ((eveChainProgressMessage*)message)->getPosCnt();
			quint32 remtime = ((eveChainProgressMessage*)message)->getRemainingTime();
            outStream << messageLength << statTime.seconds() << statTime.nanoSeconds() << cid << poscnt << remtime;
		}
		break;
    case EVEMESSAGETYPE_CHAINSTATUS:
    {
        eveTime statTime = ((eveChainStatusMessage*)message)->getTime();
        quint32 cstatus = (quint32)((eveChainStatusMessage*)message)->getChainStatus();
        quint32 cid  = (quint32) ((eveChainStatusMessage*)message)->getChainId();
        outStream << messageLength << statTime.seconds() << statTime.nanoSeconds() << cid << cstatus;
        QHash<int, quint32> smstatusHash =  ((eveChainStatusMessage*)message)->getSMStatusHash();
        foreach (int smid, smstatusHash.keys()) {
            outStream << smid << smstatusHash.value(smid);
        }
    }
    break;
        case EVEMESSAGETYPE_REQUEST:
		{
			quint32 rid  = ((eveRequestMessage*)message)->getReqId();
			quint32 rtype = ((eveRequestMessage*)message)->getReqType();
			QString rtext = ((eveRequestMessage*)message)->getReqText();
			outStream << messageLength << rid << rtype << rtext;
		}
		break;
		case EVEMESSAGETYPE_REQUESTCANCEL:
		{
			quint32 rid  = ((eveRequestCancelMessage*)message)->getReqId();
			outStream << messageLength << rid;
		}
		break;
		case EVEMESSAGETYPE_AUTOPLAY:
        case EVEMESSAGETYPE_REPEATCOUNT:
        case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
		{
			qint32 ival  = ((eveMessageInt*)message)->getInt();
			outStream << messageLength << ival;
        }
		break;
		case EVEMESSAGETYPE_REORDERPLAYLIST:
		{
			qint32 ival1 = ((eveMessageIntList*)message)->getInt(0);
			qint32 ival2 = ((eveMessageIntList*)message)->getInt(1);
			outStream << messageLength << ival1 << ival2;
        }
		break;
		case EVEMESSAGETYPE_ADDTOPLAYLIST:
		case EVEMESSAGETYPE_CURRENTXML:
		{
			QString xmlName = ((eveAddToPlMessage*)message)->getXmlName();
			QString xmlAuthor = ((eveAddToPlMessage*)message)->getXmlAuthor();
			QByteArray xmlData = ((eveAddToPlMessage*)message)->getXmlData();
			outStream << messageLength << xmlName << xmlAuthor << xmlData;
		}
		break;
		case EVEMESSAGETYPE_DATA:
		{
			quint32 chid = ((eveDataMessage*)message)->getChainId();
			quint32 smid = ((eveDataMessage*)message)->getSmId();
			quint32 poscounter = ((eveDataMessage*)message)->getPositionCount();
			quint32 dtype = (quint32)((eveDataMessage*)message)->getDataType();
			eveDataStatus dstatus = ((eveDataMessage*)message)->getDataStatus();
			quint32 dataMod = (quint32)((eveDataMessage*)message)->getDataMod();
			eveTime dtime = ((eveDataMessage*)message)->getDataTimeStamp();
			QString xmlId = ((eveDataMessage*)message)->getXmlId();
			outStream << messageLength << chid << smid << poscounter << dtype << dataMod;
			outStream << dstatus.getSeverity() << dstatus.getAlarmCondition() << dstatus.getAcquisitionStatus() << dtime.seconds() << dtime.nanoSeconds() << xmlId;
			switch (dtype) {
				case eveInt8T:					/* eveInt8 */
				{
					QVector<signed char> cArray = ((eveDataMessage*)message)->getCharArray();
					outStream << cArray ;
				} break;
				case eveInt16T:					/* eveInt16 */
				{
					QVector<short> sArray = ((eveDataMessage*)message)->getShortArray();
					outStream << sArray ;
				} break;
				case eveInt32T:					/* eveInt32 */
				{
					QVector<int> iArray = ((eveDataMessage*)message)->getIntArray();
					outStream << iArray ;
				}break;
				case eveFloat32T:					/* eveFLoat32 */
				{
					QVector<float> fArray = ((eveDataMessage*)message)->getFloatArray();
					outStream  << fArray ;
				}break;
				case eveFloat64T:					/* eveFloat64 */
				{
					QVector<double> dArray = ((eveDataMessage*)message)->getDoubleArray();
					outStream << dArray ;
				}break;
				case eveStringT:					/* eveString */
				{
					QStringList strings = ((eveDataMessage*)message)->getStringArray();
					outStream << strings;
				} break;
				case eveDateTimeT:					/* eveString */
				{
					QStringList strings = ((eveDataMessage*)message)->getStringArray();
					outStream << strings;
				} break;
			}
		}
		break;
		case EVEMESSAGETYPE_PLAYLIST:
		{
			qint32 count = ((evePlayListMessage*)message)->getCount();
			evePlayListEntry plentry;

			outStream << messageLength << count;
			for (int i=0; i < count; ++i){
				plentry = ((evePlayListMessage*)message)->getEntry(i);
				if (plentry.pid == -1) continue;
				outStream << plentry.pid << plentry.name << plentry.author;
			}
		}
		break;

		default:
			eveError::log(ERROR,QString("eveMessageFactory::getNewStream: unknown message type (%1)").arg(messageType));
			block->clear();
            return block;
			break;
    }
    outStream.device()->seek(8);
    messageLength = block->length() - 12;
    outStream << messageLength;

	return block;
}

bool eveMessageFactory::validate(quint16 version, quint16 type, quint32 length){

	bool valid = true;

	if (!((version >= EVEMESSAGE_MIN_SUPPORTEDVERSION) && (version <= EVEMESSAGE_MAX_SUPPORTEDVERSION))){
		valid = false;
	}

	if ((type < EVEMESSAGETYPE_REQUESTANSWER) && length != 0){
		valid = false;
	}

	if ((type == EVEMESSAGETYPE_REQUESTANSWER) && (length > (EVEMESSAGE_MAXLENGTH -12))){
		valid = false;
	}
	return valid;
}

