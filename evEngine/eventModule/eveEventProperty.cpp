/*
 * eveEventProperty.cpp
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#include "eveEventProperty.h"
#include "eveScanManager.h"

eveEventProperty::eveEventProperty(QObject *parent, eveVariant limit, eventTypeT type) : eventLimit(limit), QObject(parent)
{
	isConnected = false;
	eventType = type;
	actionType = eveEventActionNONE;

}

eveEventProperty::~eveEventProperty() {
	// TODO Auto-generated destructor stub
}

bool eveEventProperty::connectEvent(eveScanManager* manager) {

	if (manager == NULL) return false;
	connect(this, SIGNAL(signalEvent(eveEventProperty*)), manager, SLOT(newEvent(eveEventProperty*)), Qt::QueuedConnection);
	return true;
}
