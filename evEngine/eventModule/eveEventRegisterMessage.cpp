/*
 * eveEventRegisterMessage.cpp
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#include "eveEventRegisterMessage.h"

eveEventRegisterMessage::eveEventRegisterMessage(bool regOrUnreg, eveEventProperty* evprop) : eveMessage(EVEMESSAGETYPE_EVENTREGISTER) {
	event = evprop;
	registerEvent = regOrUnreg;

}
// copy constructor
eveEventRegisterMessage::eveEventRegisterMessage(const eveEventRegisterMessage& regmessage) :
	eveMessage(EVEMESSAGETYPE_EVENTREGISTER), registerEvent(regmessage.registerEvent), event(NULL)
{

}

eveEventRegisterMessage::~eveEventRegisterMessage() {
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
