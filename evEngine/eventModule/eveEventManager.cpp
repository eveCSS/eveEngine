/*
 * eveEventManager.cpp
 *
 *  Created on: 28.09.2009
 *      Author: eden
 */

#include "eveEventManager.h"
#include "eveMessageHub.h"
#include "eveError.h"

eveEventManager::eveEventManager() {
	channelId = eveMessageHub::getmHub()->registerChannel(this, EVECHANNEL_EVENT);
	shutdownPending = false;
}

eveEventManager::~eveEventManager() {
	// TODO Auto-generated destructor stub
}

/**
 * \brief process messages sent to eveStorageManager
 * @param message pointer to the message which eveStorageManager received
 */
void eveEventManager::handleMessage(eveMessage *message){

	eveError::log(4, "eveEventManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_EVENTREGISTER:
			registerEvent((eveEventRegisterMessage*)message);
			break;
		case EVEMESSAGETYPE_CHAINSTATUS:
		{
			int chainId = ((eveChainStatusMessage*)message)->getChainId();
			int smId = ((eveChainStatusMessage*)message)->getSmId();
			if (scheduleHash.contains(eveVariant::getMangled(chainId,smId))) triggerSchedule(chainId, smId);
			break;
		}
	default:
		sendError(ERROR,0,QString("handleMessage: unknown message, type: %1").arg(message->getType()));
		break;
	}
	delete message;
}

/**
 * \brief register or unregister an event
 * @param message EVENTREGISTER message
 */
void eveEventManager::registerEvent(eveEventRegisterMessage* message){

	eveEventProperty* event = message->takeEventProperty();
	if (event == NULL){
		sendError(ERROR, 0, "unable to register; invalid event property");
		return;
	}

	if (message->isRegisterEvent()){
		// register
		if (event->getEventType() == eveEventTypeSCHEDULE){
			bool ok;
			quint64 id = event->getLimit().toULongLong(&ok);
			if (ok)
				scheduleHash.insert(id, event);
		}
		else {
			// monitor event
			// TODO
		}
	}
	else {
		// unregister
		if (event->getEventType() == eveEventTypeSCHEDULE){
			bool ok;
			quint64 id = event->getLimit().toULongLong(&ok);
			if (ok) {
				scheduleHash.remove(id);
			}
		}
		else {
			// monitor event
			// TODO
		}
	}
}

void eveEventManager::triggerSchedule(int chainid, int smid){

	eveEventProperty* event = scheduleHash.value(eveVariant::getMangled(chainid,smid));
	event->setValue(event->getLimit());
	event->fireEvent();
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveEventManager::sendError(int severity, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, EVEMESSAGEFACILITY_EVENT, errorType, errorString));
}

/**
 * \brief shutdown eveEventManager and this thread
 */
void eveEventManager::shutdown(){

	eveError::log(1, QString("eveEventManager: shutdown"));

	if (!shutdownPending) {
		shutdownPending = true;

		// stop input Queue
		disableInput();

		// delete all events
	}

	// make sure mHub reads all outstanding messages before closing the channel
	if (unregisterIfQueueIsEmpty()){
		QThread::currentThread()->quit();
	}
	else {
		// call us if an outstanding message has been taken
		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}
}

