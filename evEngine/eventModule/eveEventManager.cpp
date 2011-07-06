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
			if (scheduleHash.contains(eveVariant::getMangled(chainId,smId))) triggerSchedule(chainId, smId, (eveChainStatusMessage*)message);
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

	if (shutdownPending) return;

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
			if (ok) scheduleHash.insert(id, event);
			sendError(DEBUG, 0, QString("registering schedule event id: %1").arg(id));
		}
		else if (event->getEventType() == eveEventTypeDETECTOR){
			detectorHash.insert(event->getName(), event);
			sendError(DEBUG, 0, QString("registering detector event: %1").arg(event->getName()));
		}
		else if (event->getEventType() == eveEventTypeMONITOR){
			// monitor event
			sendError(DEBUG, 0, "registering monitor event");
			eveDeviceMonitor* devMon = new eveDeviceMonitor(this, event);
			monitorHash.insert(event, devMon);
		}
		else
			sendError(DEBUG, 0, QString("unable to register event: %1, unknown event type").arg(event->getName()));
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
		else if (event->getEventType() == eveEventTypeDETECTOR){
			if (detectorHash.contains(event->getName())){
				detectorHash.remove(event->getName());
				sendError(DEBUG, 0, QString("unregistering detector event: %1").arg(event->getName()));
				delete event;
			}
		}
		else if (event->getEventType() == eveEventTypeMONITOR) {
			if (monitorHash.contains(event)){
				eveDeviceMonitor* devMon = monitorHash.take(event);
				delete event;
				delete devMon;
			}
		}
		else
			sendError(DEBUG, 0, QString("unable to unregister event: %1, unknown event type").arg(event->getName()));
	}
}

void eveEventManager::triggerSchedule(int chainid, int smid, eveChainStatusMessage* message){

	eveEventProperty* event = scheduleHash.value(eveVariant::getMangled(chainid,smid));
	if (((event->getIncident() == eveIncidentEND) && (message->getStatus() == eveChainSmDONE)) ||
			((event->getIncident() == eveIncidentSTART) && (message->getStatus() == eveChainSmEXECUTING))){
		event->setValue(event->getLimit());
		event->fireEvent();
		sendError(DEBUG, 0, QString("fired a schedule event (%1/%2)").arg(chainid).arg(smid));
	}
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveEventManager::sendError(int severity, int errorType,  QString errorString){
	sendError(severity, EVEMESSAGEFACILITY_EVENT, errorType, errorString);
	//addMessage(new eveErrorMessage(severity, EVEMESSAGEFACILITY_EVENT, errorType, errorString));
}

/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveEventManager::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
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
		foreach (eveEventProperty* event, scheduleHash){
			delete event;
		}
		scheduleHash.clear();
		foreach (eveEventProperty* monevent, monitorHash.keys()){
			delete monitorHash.take(monevent);
			delete monevent;
		}
		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}

	// make sure mHub reads all outstanding messages before closing the channel
	shutdownThreadIfQueueIsEmpty();

}

