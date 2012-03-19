/*
 * eveMathManager.cpp
 *
 *  Created on: 20.05.2010
 *      Author: eden
 */

#include <QThread>
#include <QList>
#include "eveMessage.h"
#include "eveMathManager.h"
#include "eveMessageHub.h"
#include "eveError.h"

eveMathManager::eveMathManager(int chainId, QList<eveMathConfig*>* mathConfigList) {
	chid = chainId;
	shutdownPending = false;
	// create a hash with math objects delete mathConfigList

	while (!mathConfigList->isEmpty()){
		eveMathConfig *mathConfig = mathConfigList->takeFirst();
		eveMath* math = new eveMath(*mathConfig, this);
		delete mathConfig;
		foreach (int scanmoduleId, math->getAllScanModuleIds()){
			mathHash.insert(scanmoduleId, math);
		}
	}
	delete mathConfigList;

	// register with messageHub
	channelId = eveMessageHub::getmHub()->registerChannel(this, EVECHANNEL_MATH);

}

eveMathManager::~eveMathManager() {
	// TODO Auto-generated destructor stub
}

/**
 * \brief process messages sent to eveMathManager
 * @param message pointer to the message which eveMathManager received
 */
void eveMathManager::handleMessage(eveMessage *message){

	if (shutdownPending) {
		delete message;
		return;
	}

	//eveError::log(4, "eveMathManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_DATA:
		{
			//sendError(DEBUG, 0 ,"eveMathManager::handleMessage: data message arrived");
			eveDataMessage* datamessage = (eveDataMessage*)message;
			if ((datamessage->getChainId() == chid) &&
					(datamessage->getDataMod() == DMTunmodified)){
				eveVariant data = datamessage->toVariant();
				foreach(eveMath* math, mathHash.values(datamessage->getSmId())){
					math->addValue(datamessage->getXmlId(), datamessage->getSmId(), datamessage->getPositionCount(), data);
				}
			}
		}
		break;
		case EVEMESSAGETYPE_CHAINSTATUS:
		{
			if (((eveChainStatusMessage*)message)->getChainId() == chid){
				if (((eveChainStatusMessage*)message)->getStatus()== eveChainSmDONE){
					int smid = ((eveChainStatusMessage*)message)->getSmId();
					foreach (eveMath* math, mathHash.values(smid)){
						foreach (MathAlgorithm algorithm, eveMath::getAlgorithms()){
							foreach (eveDataMessage* sendmessage, math->getResultMessage(algorithm, chid, smid)){
								addMessage(sendmessage);
							}
						}
					}
				}
				else if (((eveChainStatusMessage*)message)->getStatus()== eveChainSmPAUSED){
					int smid = ((eveChainStatusMessage*)message)->getSmId();
					if (!pauseList.contains(smid)) pauseList.append(smid);
				}
				else if (((eveChainStatusMessage*)message)->getStatus()== eveChainSmEXECUTING){
					int smid = ((eveChainStatusMessage*)message)->getSmId();
					if (pauseList.contains(smid)){
						pauseList.removeOne(smid);
					}
					else {
						foreach (eveMath* math, mathHash.values(smid)){
							if (math->hasInit()) math->reset();
						}
					}
				}
				else if (((eveChainStatusMessage*)message)->getStatus()== eveChainDONE){
					shutdown();
				}
			}
		}
		break;
		default:
			sendError(ERROR,0,"eveMathManager::handleMessage: unknown message");
			break;
	}
	delete message;
}

/**
 * \brief shutdown associated chain and this thread
 */
void eveMathManager::shutdown(){

	if (!shutdownPending) {
		shutdownPending = true;

		// stop input Queue
		disableInput();

		QList<eveMath*> deletedHash;
		// Caution: multiHash may contain the math object several times
		foreach(eveMath* math, mathHash.values()) {
			if (!deletedHash.contains(math)){
				deletedHash.append(math);
				delete math;
			}
		}
		mathHash.clear();
		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}

	// make sure mHub reads all outstanding messages before closing the channel
	shutdownThreadIfQueueIsEmpty();

}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveMathManager::sendError(int severity, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, EVEMESSAGEFACILITY_MATH, errorType, errorString));
}

/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveMathManager::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}


/**
 *\brief queue the specified message to the send queue
 *
 * @param message message to be sent
 */
void eveMathManager::sendMessage(eveDataMessage* message){
	message->setChainId(chid);
	addMessage(message);
}
