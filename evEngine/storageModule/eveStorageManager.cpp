/*
 * eveStorageManager.cpp
 *
 *  Created on: 28.08.2009
 *      Author: eden
 */

#include <QTimer>
#include <QThread>
#include <QStringList>
#include "eveMessageHub.h"
#include "eveStorageManager.h"
#include "eveDataCollector.h"
#include "eveError.h"

eveStorageManager::eveStorageManager(QString filename, QByteArray* xmldata) {
	// register with messageHub
	xmlData = new QByteArray(*xmldata);
	fileName = filename;
	shutdownPending = false;
	channelId = eveMessageHub::getmHub()->registerChannel(this, EVECHANNEL_STORAGE);
}

eveStorageManager::~eveStorageManager() {
	foreach (eveDataCollector* dc, chainIdDCHash) delete dc;
}

/**
 * \brief process messages sent to eveStorageManager
 * @param message pointer to the message which eveStorageManager received
 */
void eveStorageManager::handleMessage(eveMessage *message){

	switch (message->getType()) {
		case EVEMESSAGETYPE_STORAGECONFIG:
			sendError(DEBUG,0,"eveStorageManager::handleMessage: got STORAGECONFIG-message");
			if (((eveStorageMessage*)message)->getFileName() == fileName){
				if (!configStorage((eveStorageMessage*)message))
					sendError(ERROR,0,QString("eveStorageManager::handleMessage: unable to init StorageObject for File %1 (%2)").arg(fileName).arg(message->getType()));
			}
			else {
				sendError(ERROR,0,QString("eveStorageManager::handleMessage: wrong Filename %1, expected %2").arg(((eveStorageMessage*)message)->getFileName()).arg(fileName));
			}
			break;
		case EVEMESSAGETYPE_CHAINSTATUS:
			if (((eveChainStatusMessage*)message)->getStatus() == eveChainDONE){
				int id = ((eveChainStatusMessage*)message)->getChainId();
				if (chainIdChannelHash.remove(id) == 0){
					sendError(ERROR,0,QString("handleMessage: unable to remove not existing chainId %1 from chainList").arg(id));
				}
				else {
					// if id is in chainIdChannelHash, then it is in chainIdDCHash
					delete chainIdDCHash.take(id);
					// close input queue, if we are done
					if (chainIdChannelHash.isEmpty()) disableInput();
					eveChainStatusMessage* answer = new eveChainStatusMessage(eveChainSTORAGEDONE, id, 0, 0);
					answer->setStatus(eveChainSTORAGEDONE);
					addMessage(answer);
					if (chainIdChannelHash.isEmpty()) shutdown();
				}
			}
			break;
		case EVEMESSAGETYPE_DATA:
		{
			int id = ((eveDataMessage*)message)->getChainId();
			if (chainIdChannelHash.contains(id)){
				(chainIdDCHash.value(id))->addData((eveDataMessage*)message);
			}
			else
				sendError(ERROR, 0, QString("handleMessage: received data message with invalid chainId: %1").arg(id));
		}
		break;
		case EVEMESSAGETYPE_DEVINFO:
		{
			sendError(DEBUG, 0, QString("Device Info: (%1/%2) Name: %3 (%4), %5").arg(((eveDevInfoMessage*)message)->getChainId())
					.arg(((eveDevInfoMessage*)message)->getSmId())
					.arg(((eveDevInfoMessage*)message)->getName())
					.arg(((eveDevInfoMessage*)message)->getXmlId())
					.arg((((eveDevInfoMessage*)message)->getText())->value(0, QString())));
			int id = ((eveDevInfoMessage*)message)->getChainId();
			if (chainIdChannelHash.contains(id)){
				sendError(DEBUG, 0, "found DataCollector, calling with new device info");
				(chainIdDCHash.value(id))->addDevice((eveDevInfoMessage*)message);
			}
			else
				sendError(ERROR, 0, QString("handleMessage: received data message with invalid chainId: %1").arg(id));
		}
		break;
		case EVEMESSAGETYPE_LIVEDESCRIPTION:
			foreach (eveDataCollector* dc, chainIdDCHash){
				sendError(DEBUG, 0, QString("got Livedescription: %1").arg(((eveMessageText*)message)->getText()));
				//TODO save the livedescription as comment
				dc->addComment((eveMessageText*)message);
			}
			break;
		default:
			sendError(ERROR,0,QString("eveStorageManager::handleMessage: unknown message, type: %1").arg(message->getType()));
			break;
	}
	delete message;
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveStorageManager::sendError(int severity, int errorType,  QString errorString){
	sendError(severity, EVEMESSAGEFACILITY_STORAGE, errorType, errorString);
}
/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveStorageManager::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}

/**
 * \brief shutdown StorageManager and this thread
 */
void eveStorageManager::shutdown(){

	// stop input Queue
	if (!shutdownPending) {
		shutdownPending = true;
		disableInput();
		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}

	// TODO
	// wait until all data has been saved

	// make sure mHub reads all outstanding messages before closing the channel
	shutdownThreadIfQueueIsEmpty();
}

/**
 * \brief create a DataCollector for the chain of this message
 * @param message storageConfiguration message from scanmodule chain
 * @return true if initialization was successful
 */
bool eveStorageManager::configStorage(eveStorageMessage* message){

	int chainId = message->getChainId();
	if (!chainIdChannelHash.contains(chainId)){
		chainIdChannelHash.insert(chainId, message->getChannelId());
		eveDataCollector* dc = new eveDataCollector(this, message, xmlData);
		xmlData = NULL;
		chainIdDCHash.insert(chainId, dc);
		return true;
	}
	return false;
}

