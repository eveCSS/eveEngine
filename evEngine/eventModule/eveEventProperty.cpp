/*
 * eveEventProperty.cpp
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#include "eveEventProperty.h"

/**
 *
 * @param evname detectorEventId (DetectorEvent), device name (monitorEvent), empty (scheduleEvent)
 * @param operation comparison operator (monitorEvent), detectorid (DetectorEvent), empty (scheduleEvent)
 * @param limit limit to compare against (monitorEvent, mangled chainid/smid (detectorEvent and scheduleEvent)
 * @param type eventtype (monitorEvent, DetectorEvent or scheduleEvent)
 * @param incident	Start/End (scheduleEvent)
 * @param actiontype (Start/Stop/Pause/Halt/Break/Redo)
 * @param deviceCommand CommandDefinition of the monitor device (monitorEvent)
 * @return
 */
eveEventProperty::eveEventProperty(QString evname, QString operation, eveVariant limit, eventTypeT type, incidentTypeT incident, actionTypeT actiontype, eveDeviceCommand* deviceCommand) : eventLimit(limit)
{
	devCommand = deviceCommand;
	eventType = type;
	actionType = actiontype;
	incidentType = incident;
	name = evname;
	comparison = operation;
	signalOff = false;
	eventId = 0;
	smId = 0;
	chainAction=false;
}

eveEventProperty::~eveEventProperty() {
	if (devCommand != NULL) delete devCommand;
}

