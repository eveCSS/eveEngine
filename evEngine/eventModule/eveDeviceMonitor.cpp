/*
 * eveDeviceMonitor.cpp
 *
 *  Created on: 07.10.2009
 *      Author: eden
 */

#include "eveDeviceMonitor.h"
#include "eveCaTransport.h"
#include "eveEventManager.h"
#include "eveError.h"

eveDeviceMonitor::eveDeviceMonitor(eveEventManager* eventManager, eveEventProperty* eventProp) : eveSMBaseDevice(eventManager){

	// Note: eventProp lives in a different thread with unknown lifetime
	event = eventProp;
	name = "";
	xmlId = "";
	manager = eventManager;
	if (event != NULL) {
		name = event->getName();
		limit = event->getLimit();
		if ((event->getDevCommand() != NULL) && (event->getDevCommand()->getTrans()!= NULL)){
			if (event->getDevCommand()->getTrans()->getTransType() == eveTRANS_CA){
				monitorTrans = new eveCaTransport(this, xmlId, name, (eveTransportDefinition*)event->getDevCommand()->getTrans());
				if (monitorTrans != NULL) {
					connect(monitorTrans, SIGNAL(valueChanged(eveDataMessage*)), this, SLOT(valueChange(eveDataMessage*)));

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
	eveError::log(DEBUG, "eveDeviceMonitor: new Monitor", EVEMESSAGEFACILITY_EVENT);

}

eveDeviceMonitor::eveDeviceMonitor(eveEventManager* eventManager, eveMonitorRegisterMessage* message) : eveSMBaseDevice(eventManager){

	manager = eventManager;
	name = message->getName();
	xmlId = message->getXMLId();
	destination = message->getStorageChannel();

	if ((message->getTransport() != NULL) && (message->getTransport()->getTransType() == eveTRANS_CA)) {
		monitorTrans = new eveCaTransport(this, xmlId, name, message->getTransport());
		connect(monitorTrans, SIGNAL(valueChanged(eveDataMessage*)), this, SLOT(saveValue(eveDataMessage*)));
	}
	if (monitorTrans == NULL)
		manager->sendError(ERROR, 0, QString("unable to create a monitor transport for %1").arg(name));
	else {
		int status = monitorTrans->monitorTrans();
	}
	eveError::log(DEBUG, "eveDeviceMonitor: new Monitor", EVEMESSAGEFACILITY_EVENT);
}

eveDeviceMonitor::~eveDeviceMonitor() {
	// TODO Auto-generated destructor stub
	if (monitorTrans != NULL) delete monitorTrans;
}

// slot, called by underlying transport for every value change
void eveDeviceMonitor::valueChange(eveDataMessage* newdata) {

	if (newdata == NULL) return;

	eveVariant* newValue = new eveVariant(newdata->toVariant());
	delete newdata;

	bool newState = (this->*compare)(*newValue);

	// TODO remove this debug stuff
	if (newValue->canConvert(QVariant::String)){
		QString result = "false";
		if (newState) result = "true";
                manager->sendError(DEBUG, 0, QString("valueChange event, newValue: %1, limit: %2, result: %3").arg(newValue->toString()).arg(limit.toString()).arg(result));
	}

	try {
		if (newState != event->getOn()) {
			event->setOn(newState);
			if (newState || event->isBothDirections()) {
				event->setValue(*newValue);
				event->fireEvent();
				manager->sendError(DEBUG, 0, QString("fired a valueChange event, newValue: %1").arg(newValue->toDouble()));
			}
		}
	}
	catch (std::exception& e)
	{
		//printf("C++ Exception %s\n",e.what());
		manager->sendError(ERROR,0,QString("C++ Exception %1 eveDeviceMonitor::valueChange").arg(e.what()));
	}
	if (newValue != NULL) delete newValue;
}

// slot, called by underlying transport for every value change
void eveDeviceMonitor::saveValue(eveDataMessage* newdata) {

	// TODO remove this debug stuff
	eveVariant newValue = newdata->toVariant();
	if (newValue.canConvert(QVariant::String)){
		manager->sendError(DEBUG, 0, QString("valueChange monitor, newValue: %1").arg(newValue.toString()));
	}

	if (newdata != NULL){
        newdata->setDestinationChannel(destination);
        newdata->setDestinationFacility(EVECHANNEL_STORAGE);
		newdata->setDataMod(DMTdeviceData);
		manager->addMessage(newdata);
		if ((newdata->getDataStatus().getSeverity() != 0) || (newdata->getDataStatus().getAlarmCondition() != 0)){
			int status = MINOR;
			quint8 dstatus = newdata->getDataStatus().getAlarmCondition();
			if (dstatus > 1) status = ERROR;
			manager->sendError(status, 0, QString("device: %1 (%2) Severity: %3 Alarm Condition: %4").arg(
					newdata->getXmlId()).arg(newdata->getName()).arg(newdata->getDataStatus().getSeverityString()).arg(newdata->getDataStatus().getAlarmString()));
		}
		manager->sendError(DEBUG, 0, QString("valueChange monitor send to: %1").arg(destination));
	}
}

void eveDeviceMonitor::sendError(int severity, int facility, int errorType, QString message){
	// TODO
	// allow more facilities in event manager
    //	manager->sendError(severity, facility, errorType, message);
	manager->sendError(severity, errorType, message);
}
