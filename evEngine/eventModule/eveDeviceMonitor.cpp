/*
 * eveDeviceMonitor.cpp
 *
 *  Created on: 07.10.2009
 *      Author: eden
 */

#include "eveDeviceMonitor.h"
#include "eveCaTransport.h"
#include "eveEventManager.h"

eveDeviceMonitor::eveDeviceMonitor(eveEventManager* eventManager, eveEventProperty* eventProp) : eveSMBaseDevice(eventManager){

	// Note: eventProp lives in a different thread with unknown lifetime
	currentState = false;
	event = eventProp;
	QString name = "";
	manager = eventManager;
	if (event != NULL) {
		name = event->getName();
		limit = event->getLimit();
		if ((event->getDevCommand() != NULL) && (event->getDevCommand()->getTrans()!= NULL)){
			if (event->getDevCommand()->getTrans()->getTransType() == eveTRANS_CA){
				monitorTrans = new eveCaTransport(this, name, (eveTransportDef*)event->getDevCommand()->getTrans());
				if (monitorTrans != NULL) {
					connect(monitorTrans, SIGNAL(valueChanged(eveVariant*)), this, SLOT(valueChange(eveVariant*)));

					if (event->getCompareOperator() == "eq")
						compare = &eveDeviceMonitor::operatorEQ;
					else if (event->getCompareOperator() == "ne")
						compare = &eveDeviceMonitor::operatorNE;
					else if (event->getCompareOperator() == "gt")
						compare = &eveDeviceMonitor::operatorGT;
					else if (event->getCompareOperator() == "lt")
						compare = &eveDeviceMonitor::operatorLT;
					else
						manager->sendError(ERROR, 0, QString("unknown compare operator %1").arg(event->getCompareOperator()));
				}
			}
		}
	}
	if (monitorTrans == NULL)
		manager->sendError(ERROR, 0, QString("unable to create a monitor transport for %1").arg(name));
	else {
		int status = monitorTrans->monitorTrans();
	}

}

eveDeviceMonitor::~eveDeviceMonitor() {
	// TODO Auto-generated destructor stub
	if (monitorTrans != NULL) delete monitorTrans;
}

// slot, called by underlying transport for every value change
void eveDeviceMonitor::valueChange(eveVariant* newValue) {

	bool newState = (this->*compare)(*newValue);

	if (newState != currentState) {
		currentState = newState;
		if (currentState || event->getSignalOff()) {
			event->setValue(*newValue);
			event->fireEvent();
			manager->sendError(DEBUG, 0, QString("fired a valueChange event, newValue: %1").arg(newValue->toDouble()));
		}
	}
	if (newValue != NULL) delete newValue;
}

void eveDeviceMonitor::sendError(int severity, int facility, int errorType, QString message){
	// TODO
	// allow more facilities in event manager
    //	manager->sendError(severity, facility, errorType, message);
	manager->sendError(severity, errorType, message);
}
