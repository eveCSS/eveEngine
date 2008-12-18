#include <QByteArray>
#include <QDataStream>
#include <QString>

#include "eveMessageFactory.h"
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
		if (byteArray == 0) eveError::log(4,"eveMessageFactory::getNewMessage byteArray is NullPointer but length > 0");
		if ((quint32)byteArray->length() != length) {
			eveError::log(4,"eveMessageFactory::getNewMessage length of byteArray != length");
			return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "length mismatch");
		}
	}
	QDataStream inStream(*byteArray);
	inStream.setVersion(QDataStream::Qt_4_0);


	switch (type) {
		case EVEMESSAGETYPE_ADDTOPLAYLIST:
		case EVEMESSAGETYPE_CURRENTXML:
		{
			QString xmlName, xmlAuthor;
			QByteArray xmlData;

			inStream >> xmlName >> xmlAuthor >> xmlData;
			if (2*xmlName.length() + 2*xmlAuthor.length() + xmlData.length()+ 12 != (qint32) length) {
				eveError::log(4,"eveMessageFactory::getNewMessage malformed EVEMESSAGETYPE_CURRENTXML / EVEMESSAGETYPE_ADDTOPLAYLIST");
				return new eveErrorMessage(ERROR, EVEMESSAGEFACILITY_CPARSER, 0x0008, "malformed EVEMESSAGETYPE_CURRENTXML / EVEMESSAGETYPE_ADDTOPLAYLIST");
			}
			if (type == EVEMESSAGETYPE_CURRENTXML)
				message = new eveCurrentXmlMessage(xmlName, xmlAuthor, xmlData);
			else
				message = new eveAddToPlMessage(xmlName, xmlAuthor, xmlData);
		}
		break;
		case EVEMESSAGETYPE_START:
			message = new eveMessage(EVEMESSAGETYPE_START);
		break;
		case EVEMESSAGETYPE_HALT:
			message = new eveMessage(EVEMESSAGETYPE_HALT);
		break;
		case EVEMESSAGETYPE_BREAK:
			message = new eveMessage(EVEMESSAGETYPE_BREAK);
		break;
		case EVEMESSAGETYPE_STOP:
			message = new eveMessage(EVEMESSAGETYPE_STOP);
		break;
		case EVEMESSAGETYPE_PAUSE:
			message = new eveMessage(EVEMESSAGETYPE_PAUSE);
		break;
		case EVEMESSAGETYPE_ENDPROGRAM:
			message = new eveMessage(EVEMESSAGETYPE_ENDPROGRAM);
		break;
		case EVEMESSAGETYPE_LIVEDESCRIPTION:
		{
			QString messageText;
			inStream >> messageText;
			message = new eveMessageText(EVEMESSAGETYPE_LIVEDESCRIPTION, messageText);
		}
		break;
		case EVEMESSAGETYPE_AUTOPLAY:
		case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
		{
			int intValue;
			inStream >> intValue;
			if (type == EVEMESSAGETYPE_AUTOPLAY)
				message = new eveMessageInt(EVEMESSAGETYPE_AUTOPLAY, intValue);
			else
				message = new eveMessageInt(EVEMESSAGETYPE_REMOVEFROMPLAYLIST, intValue);
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
				bool answer;
				inStream >> answer;
				message = new eveRequestAnswerMessage(rid, rtype, answer);
			}
			else if (rtype == EVEREQUESTTYPE_INT){
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
					eveError::log(4,"eveMessageFactory::getNewMessage: malformed EVEMESSAGETYPE_REQUESTANSWER");
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
		case EVEMESSAGETYPE_CHAINSTATUS:
		{
			struct timespec statTime;
			quint32 secs, nsecs, cstatus, cid, smid, poscnt, remtime;
			inStream >> secs >> nsecs >> cstatus >> cid >> smid >> poscnt >> remtime;
			statTime.tv_sec = secs;
			statTime.tv_nsec = nsecs;
			epicsTime messageTime = statTime;
			message = new eveChainStatusMessage(cstatus, cid, smid, poscnt, messageTime, remtime);
		}
		break;
		case EVEMESSAGETYPE_PLAYLIST:
		{
			quint32 sum = 0;
			qint32 plcount;
			QList<evePlayListEntry> plList;
			evePlayListEntry plEntry;

			inStream >> plcount;
			eveError::log(4,QString("eveMessageFactory::getNewMessage: EVEMESSAGETYPE_PLAYLIST plcount: %1").arg(plcount));
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
			quint32 dtype;
			eveDataStatus stat;
			quint32 dataMod;
			epicsTimeStamp timestamp;
			QString dname;
			quint32 arraycount;

			inStream >> dtype >> dataMod >> stat.severity >> stat.condition >> stat.acqStatus;
			inStream >> timestamp.secPastEpoch >> timestamp.nsec >> dname >> arraycount;
			// TODO remove this and make sure we have a correct time, otherwise epics assert is called
			timestamp.nsec = 1000;
			timestamp.secPastEpoch = 10008;

			switch (dtype) {
				case epicsInt8T:					/* epicsInt8 */
				{
					QVector<char> cArray;
					quint8 val;
					// TODO check if length == arraycount *bytesize
					for (quint32 i = 0; i < arraycount; ++i) {
						inStream >> val;
						cArray[i] = (char)val;
					}
					message = new eveDataMessage(dname, stat, (eveDataModType)dataMod, epicsTime(timestamp), cArray);
				}
				break;
				case epicsInt16T:					/* epicsInt16 */
				{
					QVector<short> sArray;
					for (quint32 i = 0; i < arraycount; ++i) {
						short value;
						inStream >> value;
						sArray.append(value);
					}
					message = new eveDataMessage(dname, stat, (eveDataModType)dataMod, epicsTime(timestamp), sArray);
				}
				break;
				case epicsInt32T:					/* epicsInt32 */
				{
					QVector<int> iArray;
					// TODO check if length == arraycount *bytesize
					for (quint32 i = 0; i < arraycount; ++i) {
						int value;
						inStream >> value;
						iArray.append(value);
					}
					message = new eveDataMessage(dname, stat, (eveDataModType)dataMod, epicsTime(timestamp), iArray);
				}
				break;
				case epicsFloat32T:					/* epicsFLoat32 */
				{
					QVector<float> fArray;
					for (quint32 i = 0; i < arraycount; ++i) {
						float value;
						inStream >> value;
						fArray.append(value);
					}
					message = new eveDataMessage(dname, stat, (eveDataModType)dataMod, epicsTime(timestamp), fArray);
				}break;
				case epicsFloat64T:					/* epicsFloat64 */
				{
					QVector<double> dArray;
					for (quint32 i = 0; i < arraycount; ++i) {
						double value;
						inStream >> value;
						dArray.append(value);
					}
					message = new eveDataMessage(dname, stat, (eveDataModType)dataMod, epicsTime(timestamp), dArray);
				}
				break;
				case epicsStringT:					/* epicsString */
				{
					QStringList stringData;
					QString instring;
					// TODO check if length == arraycount *bytesize
					for (quint32 i = 0; i < arraycount; ++i) {
						inStream >> instring;
						stringData.insert(i, instring);
					}
					message = new eveDataMessage(dname, stat, (eveDataModType)dataMod, epicsTime(timestamp), stringData);
				}
				break;
			}
		}
		break;
		default:
			eveError::log(4,QString("eveMessageFactory::getNewMessage: unknown message type %1").arg(type));
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

	QByteArray * block = new QByteArray();
    QDataStream outStream(block, QIODevice::WriteOnly);
    outStream.setVersion(QDataStream::Qt_4_0);

    outStream << starttag << version << messageType;

	switch(messageType) {
		case EVEMESSAGETYPE_ERROR:
		{
			struct timespec errTime = ((eveErrorMessage*)message)->getTime();
			quint8 severity = ((eveErrorMessage*)message)->getSeverity();
			quint8 facility = ((eveErrorMessage*)message)->getFacility();
			quint16 errType = ((eveErrorMessage*)message)->getErrorType();
			QString * errText = ((eveErrorMessage*)message)->getErrorText();
			quint32 messageLength = 0;
			outStream << messageLength << (quint32) errTime.tv_sec << (quint32) errTime.tv_nsec << severity << facility << errType << *errText;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
			break;
		case EVEMESSAGETYPE_ENGINESTATUS:
		{
			struct timespec statTime = ((eveEngineStatusMessage*)message)->getTime();
			QString * xmlId = ((eveEngineStatusMessage*)message)->getXmlId();
			quint32 estatus = ((eveEngineStatusMessage*)message)->getStatus();
			quint32 messageLength = 0;
			outStream << messageLength << (quint32) statTime.tv_sec << (quint32) statTime.tv_nsec << estatus << *xmlId;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_CHAINSTATUS:
		{
			struct timespec statTime = ((eveChainStatusMessage*)message)->getTime();
			quint32 cstatus = ((eveChainStatusMessage*)message)->getStatus();
			quint32 cid  = ((eveChainStatusMessage*)message)->getChainId();
			quint32 smid = ((eveChainStatusMessage*)message)->getSmId();
			quint32 poscnt = ((eveChainStatusMessage*)message)->getPosCnt();
			quint32 remtime = ((eveChainStatusMessage*)message)->getRemainingTime();
			quint32 messageLength = 0;
			outStream << messageLength << (quint32) statTime.tv_sec << (quint32) statTime.tv_nsec << cstatus << cid << smid << poscnt << remtime;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_REQUEST:
		{
			quint32 rid  = ((eveRequestMessage*)message)->getReqId();
			quint32 rtype = ((eveRequestMessage*)message)->getReqType();
			QString rtext = ((eveRequestMessage*)message)->getReqText();
			quint32 messageLength = 0;
			outStream << messageLength << rid << rtype << rtext;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_REQUESTCANCEL:
		{
			quint32 rid  = ((eveRequestCancelMessage*)message)->getReqId();
			quint32 messageLength = 0;
			outStream << messageLength << rid;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_AUTOPLAY:
		case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
		{
			qint32 ival  = ((eveMessageInt*)message)->getInt();
			quint32 messageLength = 0;
			outStream << messageLength << ival;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_REORDERPLAYLIST:
		{
			qint32 ival1 = ((eveMessageIntList*)message)->getInt(0);
			qint32 ival2 = ((eveMessageIntList*)message)->getInt(1);
			quint32 messageLength = 0;

			outStream << messageLength << ival1 << ival2;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_ADDTOPLAYLIST:
		case EVEMESSAGETYPE_CURRENTXML:
		{
			QString * xmlName = ((eveAddToPlMessage*)message)->getXmlName();
			QString * xmlAuthor = ((eveAddToPlMessage*)message)->getXmlAuthor();
			QByteArray * xmlData = ((eveAddToPlMessage*)message)->getXmlData();
			quint32 messageLength = 0;
			outStream << messageLength << *xmlName << *xmlAuthor << *xmlData;
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_DATA:
		{
			quint32 dtype = (quint32)((eveDataMessage*)message)->getDataType();
			eveDataStatus stat = ((eveDataMessage*)message)->getDataStatus();
			quint32 dataMod = (quint32)((eveDataMessage*)message)->getDataMod();
			epicsTime dtime = ((eveDataMessage*)message)->getDataTimeStamp();
			epicsTimeStamp dtimestamp;
			// TODO Don't know why this doesn't work (runtime error)
			//dtimestamp = dtime;
			QString dname = ((eveDataMessage*)message)->getId();
			quint32 messageLength = 0;
			outStream << messageLength << dtype << dataMod << stat.severity << stat.condition << stat.acqStatus << dtimestamp.secPastEpoch << dtimestamp.nsec << dname;
			switch (dtype) {
				case epicsInt8T:					/* epicsInt8 */
				{
					QVector<char> cArray = ((eveDataMessage*)message)->getCharArray();
					outStream << cArray.count() << cArray ;
				} break;
				case epicsInt16T:					/* epicsInt16 */
				{
					QVector<short> sArray = ((eveDataMessage*)message)->getShortArray();
					outStream << sArray.count() << sArray ;
				} break;
				case epicsInt32T:					/* epicsInt32 */
				{
					QVector<int> iArray = ((eveDataMessage*)message)->getIntArray();
					outStream << iArray.count() << iArray ;
				}break;
				case epicsFloat32T:					/* epicsFLoat32 */
				{
					QVector<float> fArray = ((eveDataMessage*)message)->getFloatArray();
					outStream << fArray.count() << fArray ;
				}break;
				case epicsFloat64T:					/* epicsFloat64 */
				{
					QVector<double> dArray = ((eveDataMessage*)message)->getDoubleArray();
					outStream << dArray.count() << dArray ;
				}break;
				case epicsStringT:					/* epicsString */
				{
					QStringList strings = ((eveDataMessage*)message)->getStringArray();
					outStream << strings.size();
					foreach (QString outstring, strings) {outStream << outstring; };
				} break;
			}
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;
		case EVEMESSAGETYPE_PLAYLIST:
		{
			qint32 count = ((evePlayListMessage*)message)->getCount();
			quint32 messageLength = 0;
			evePlayListEntry plentry;

			outStream << messageLength << count;
			for (int i=0; i < count; ++i){
				plentry = ((evePlayListMessage*)message)->getEntry(i);
				if (plentry.pid == -1) continue;
				outStream << plentry.pid << plentry.name << plentry.author;
			}
			outStream.device()->seek(8);
			messageLength = block->length() - 12;
			outStream << messageLength;
		}
		break;

		default:
			eveError::log(4,QString("eveMessageFactory::getNewStream: unknown message type (%1)").arg(messageType));
			block->clear();
			break;
	}
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

