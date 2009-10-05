/*
 * eveEventRegisterMessage.cpp
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#include "eveEventRegisterMessage.h"

eveEventRegisterMessage::eveEventRegisterMessage(bool regORunreg, eveEventProperty* evprop) {
	event = evprop;
	registerEvent = regORunreg;

}
// copy constructor
eveEventRegisterMessage::eveEventRegisterMessage(const eveEventRegisterMessage& regmessage) :
	eveMessage(EVEMESSAGETYPE_EVENTREGISTER), registerEvent(regmessage.registerEvent), event(NULL)
{

}

eveEventRegisterMessage::~eveEventRegisterMessage() {
	if (event != NULL) delete event;
}

eveEventProperty* eveEventRegisterMessage::takeEventProperty() {
	eveEventProperty* tmpEvent = event;
	event = NULL;
	return tmpEvent;
}

int eveEventRegisterMessage::getEventId(){
	if (event == NULL) return 0;
	return event->getEventId();
}
