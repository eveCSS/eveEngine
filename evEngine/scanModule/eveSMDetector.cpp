/*
 * eveSMDetector.cpp
 *
 *  Created on: 15.06.2012
 *      Author: eden
 */

#include "eveSMDetector.h"
#include "eveCaTransport.h"

eveSMDetector::eveSMDetector(eveScanModule* scanmodule, eveDetectorDefinition* definition) : eveSMBaseDevice(scanmodule) {

	triggerTrans = NULL;
	unitTrans = NULL;
	stopTrans = NULL;
	name = definition->getName();
	scanModule = scanmodule;
	triggerTimeout = 0.0;

	if ((definition->getTrigCmd() != NULL) && (definition->getTrigCmd()->getTrans()!= NULL)){
		if (definition->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
			triggerTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getTrigCmd()->getTrans());
			triggerValue.setType(definition->getTrigCmd()->getValueType());
			triggerValue.setValue(definition->getTrigCmd()->getValueString());
		}
		triggerTimeout = definition->getTrigCmd()->getTrans()->getTimeout();
	}

	if ((definition->getStopCmd() != NULL) && (definition->getStopCmd()->getTrans()!= NULL)){
		if (definition->getStopCmd()->getTrans()->getTransType() == eveTRANS_CA){
			stopTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getStopCmd()->getTrans());
			stopValue.setType(definition->getStopCmd()->getValueType());
			stopValue.setValue(definition->getStopCmd()->getValueString());
		}
	}

	if (definition->getUnitCmd() != NULL){
		if (definition->getUnitCmd()->getTrans()== NULL){
			unitString = definition->getUnitCmd()->getValueString();
		}
		else if (definition->getUnitCmd()->getTrans()->getTransType() == eveTRANS_CA){
			unitTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getUnitCmd()->getTrans());
		}
	}
}

eveSMDetector::~eveSMDetector() {
	// TODO
	sendError(DEBUG, 0, QString("Deleting Detector %1").arg(name));
	if (triggerTrans != NULL) delete triggerTrans;
	if (unitTrans != NULL) delete unitTrans;
}

/**
 * \brief send an error message to display
 * @param severity
 * @param errorType
 * @param message
 */
void eveSMDetector::sendError(int severity, int errorType,  QString message){

	scanModule->sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
							errorType,  QString("Detector %1: %2").arg(name).arg(message));
}

void eveSMDetector::sendError(int severity, int facility, int errorType, QString message){
	scanModule->sendError(severity, facility, errorType, message);
}
