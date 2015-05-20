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

eveMathManager::eveMathManager(int chainId, int schannel, QList<eveMathConfig*>* mathConfigList) {
	chid = chainId;
	shutdownPending = false;
	storageChannel=schannel;
	// create a hash with math objects delete mathConfigList

    int lowestPlotWindowId = 0xfffffff;
    eveMath* preferredMath = NULL;

	while (!mathConfigList->isEmpty()){
		eveMathConfig *mathConfig = mathConfigList->takeFirst();
		eveMath* math = new eveMath(*mathConfig, this);
        if (mathConfig->getPlotWindowId() < lowestPlotWindowId) {
            lowestPlotWindowId = mathConfig->getPlotWindowId();
            preferredMath = math;
        }
        delete mathConfig;
		foreach (int scanmoduleId, math->getAllScanModuleIds()){
			mathHash.insert(scanmoduleId, math);
		}
	}
	delete mathConfigList;
    if (preferredMath != NULL) preferredMath->setPreferred(true);

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

	switch (message->getType()) {
		case EVEMESSAGETYPE_DATA:
		{
			eveDataMessage* datamessage = (eveDataMessage*)message;
            if (datamessage->getDataMod() == DMTunmodified){
				eveVariant data = datamessage->toVariant();
				foreach(eveMath* math, mathHash.values(datamessage->getSmId())){
					math->addValue(datamessage->getXmlId(), datamessage->getSmId(), datamessage->getPositionCount(), data);
				}
			}
            else if (datamessage->getDataMod() == DMTnormalized){
                eveVariant data = datamessage->toVariant();
                foreach(eveMath* math, mathHash.values(datamessage->getSmId())){
                    // feed math with normalized values, if it doesn't computes them by itself
                    if (math->getNormalizeExternal() && (math->getNormalizeId() == datamessage->getNormalizeId())
                            && (math->getChannelId() == datamessage->getXmlId()))
                        math->addValue(datamessage->getXmlId(), datamessage->getSmId(), datamessage->getPositionCount(), data);
                }
            }
            message->setDestinationChannel(0);
            message->setDestinationFacility(EVECHANNEL_NET);
            addMessage(message);
        }
		break;
        case EVEMESSAGETYPE_CHAINSTATUS:
		{
            bool sendShutdown = false;
            if (((eveChainStatusMessage*)message)->getChainId() == chid){
                int smid = ((eveChainStatusMessage*)message)->getLastSmId();
                SMStatusT lastSMStatus = ((eveChainStatusMessage*)message)->getLastSmStatus();
                if (((eveChainStatusMessage*)message)->isDoneSM()){
					foreach (eveMath* math, mathHash.values(smid)){
						foreach (MathAlgorithm algorithm, eveMath::getAlgorithms()){
                            foreach (eveDataMessage* sendmessage, math->getResultMessage(algorithm, chid, smid)){
								sendError(DEBUG, 0, QString("MathManager sending math data: %1/%2").arg(chid).arg(smid));
                                sendmessage->addDestinationFacility(EVECHANNEL_NET);
                                addMessage(sendmessage);
							}
						}
					}
				}
                else if (lastSMStatus == SMStatusPAUSE){
					if (!pauseList.contains(smid)) pauseList.append(smid);
				}
                else if (lastSMStatus == SMStatusEXECUTING){
					if (pauseList.contains(smid)){
						pauseList.removeOne(smid);
					}
					else {
						foreach (eveMath* math, mathHash.values(smid)){
							if (math->hasInit()) math->reset();
						}
					}
				}
                else if (((eveChainStatusMessage*)message)->getChainStatus()== CHStatusDONE)
                    sendShutdown = true;
			}
            else
                sendError(ERROR,0,QString("received chainstatus message for chain %1 my id: %2").arg(((eveChainStatusMessage*)message)->getChainId()).arg(chid));

            // forward message
            message->setDestinationChannel(0);
            message->setDestinationFacility(EVECHANNEL_STORAGE | EVECHANNEL_NET | EVECHANNEL_MANAGER);
            message->addDestinationFacility(EVECHANNEL_EVENT);
            addMessage(message);
            if (sendShutdown){
                eveChainStatusMessage* doneMessage = new eveChainStatusMessage(chid, CHStatusMATHDONE);
                doneMessage->setDestinationChannel(storageChannel);
                doneMessage->addDestinationFacility(EVECHANNEL_STORAGE);
                sendError(DEBUG, 0, QString("MathManager send MathDone message chid: %1").arg(chid));
                addMessage(doneMessage);
                shutdown();
            }
        }
        break;
		default:
			sendError(ERROR,0,"eveMathManager::handleMessage: unknown message");
            delete message;
            break;
	}
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
void eveMathManager::sendMessage(eveMessage* message){
    if (message->getType() == EVEMESSAGETYPE_DATA){
        ((eveDataMessage*)message)->setChainId(chid);
        ((eveDataMessage*)message)->setDestinationChannel(storageChannel);
        if (((eveDataMessage*)message)->getDataMod() == DMTnormalized)
            sendError(DEBUG,0,QString("sending normalized DataMessage: %1").arg(((eveDataMessage*)message)->getName()));
    }
    else if (message->getType() == EVEMESSAGETYPE_METADATA) {
        ((eveMessageTextList*)message)->setChainId(chid);
        ((eveMessageTextList*)message)->setDestinationFacility(EVECHANNEL_STORAGE);
    }
    addMessage(message);
}
