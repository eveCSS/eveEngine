/*
 * eveSMMotor.cpp
 *
 *  Created on: 18.06.2012
 *      Author: eden
 */

#include "eveSMMotor.h"
#include "eveCaTransport.h"

eveSMMotor::eveSMMotor(eveScanModule* scanmodule, eveMotorDefinition* definition) : eveSMBaseDevice(scanmodule) {

	triggerTrans = NULL;
	unitTrans = NULL;
	name = definition->getName();
	scanModule = scanmodule;

	if ((definition->getTrigCmd() != NULL) && (definition->getTrigCmd()->getTrans()!= NULL)){
		if (definition->getTrigCmd()->getTrans()->getTransType() == eveTRANS_CA){
			triggerTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)definition->getTrigCmd()->getTrans());
			triggerValue.setType(definition->getTrigCmd()->getValueType());
			triggerValue.setValue(definition->getTrigCmd()->getValueString());
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

eveSMMotor::~eveSMMotor() {
	// TODO Auto-generated destructor stub
}

/**
 * \brief send an error message to display
 * @param severity
 * @param errorType
 * @param message
 */
void eveSMMotor::sendError(int severity, int errorType,  QString message){

	scanModule->sendError(severity, EVEMESSAGEFACILITY_SMDEVICE,
							errorType,  QString("Motor %1: %2").arg(name).arg(message));
}

void eveSMMotor::sendError(int severity, int facility, int errorType, QString message){
	scanModule->sendError(severity, facility, errorType, message);
}
