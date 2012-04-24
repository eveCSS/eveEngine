/*
 * eveChainStatus.cpp
 *
 *  Created on: 30.03.2012
 *      Author: eden
 */

#include "eveChainStatus.h"

eveChainStatus::eveChainStatus() {

	cstatus = eveChainSmINITIALIZING;
	pause = false;
	redo = false;
}

eveChainStatus::~eveChainStatus() {
	// TODO Auto-generated destructor stub
}

/**
 *
 * @param newStatus status of a SM
 * @return true if chainStatus changed else false
 */
bool eveChainStatus::setStatus(smStatusT newStatus ) {

	chainStatusT oldStatus = cstatus;
	// TODO
	// makes not much  sense to have different types smStatusT and chainStatusT
	if (newStatus == eveSmINITIALIZING)
		cstatus = eveChainSmINITIALIZING;
	else if (newStatus == eveSmNOTSTARTED)
		cstatus = eveChainSmIDLE;
	else if (newStatus == eveSmEXECUTING)
		cstatus = eveChainSmEXECUTING;
	else if (newStatus == eveSmPAUSED)
		cstatus = eveChainSmPAUSED;
	else if (newStatus == eveSmTRIGGERWAIT)
		cstatus = eveChainSmTRIGGERWAIT;
	else if (newStatus == eveSmAPPEND)
		// we need this to finish the plot
		cstatus = eveChainSmDONE;
	else if (newStatus == eveSmDONE)
		cstatus = eveChainSmDONE;

	if (oldStatus != cstatus) return true;
	return false;
}

bool eveChainStatus::setEvent(eveEventProperty* evprop ) {

	bool statusChanged = false;

	switch (evprop->getActionType()){
	case eveEventProperty::START:
		if (pause) {
			pause = false;
		}
		// status might have changed, this is safe
		statusChanged = true;
		break;
	case eveEventProperty::PAUSE:
		if (!pause && evprop->getOn()) {
			pause = true;
			statusChanged = true;
		}
		else if (pause && evprop->getSignalOff() && !evprop->getOn()) {
			pause = false;
			statusChanged = true;
		}
		break;
	case eveEventProperty::REDO:
		if (!redo && evprop->getOn()) {
			redo = true;
			statusChanged = true;
		}
		else if (redo && evprop->getSignalOff() && !evprop->getOn()) {
			redo = false;
			statusChanged = true;
		}
		break;
	case eveEventProperty::HALT:
	case eveEventProperty::BREAK:
	case eveEventProperty::STOP:
		statusChanged = true;
		break;
	default:
		break;
	}
	return statusChanged;
}
