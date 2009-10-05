/*
 * eveEventProperty.h
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#ifndef EVEEVENTPROPERTY_H_
#define EVEEVENTPROPERTY_H_

#include <QObject>
#include "eveVariant.h"

class eveScanManager;

enum actionTypeT {eveEventActionNONE, eveEventActionSTART, eveEventActionPAUSE, eveEventActionHALT, eveEventActionBREAK, eveEventActionSTOP, eveEventActionREDO} ;
enum eventTypeT {eveEventTypeMONITOR=1, eveEventTypeSCHEDULE} ;
enum incidentTypeT {eveIncidentTypeSTART, eveIncidentTypeEND} ;

/*
 *
 */
class eveEventProperty : public QObject {

	Q_OBJECT

public:
	eveEventProperty(QObject*, eveVariant limit, eventTypeT);
	virtual ~eveEventProperty();
	void setOn(bool state){onstate = state;};
	bool getOn(){return onstate;};
	void sendEvent(){emit signalEvent(this);};
	bool connectEvent(eveScanManager*);
	eventTypeT getEventType(){return eventType;};
	actionTypeT getActionType(){return actionType;};
	void setValue(const eveVariant& value){eventValue = value;};
	eveVariant getValue(){return eventValue;};
	eveVariant getLimit(){return eventLimit;};
	void setSmId(int smid){smId = smid;};
	int getSmId(){return smId;};
	void setChainAction(bool chainaction){chainAction = chainaction;};
	bool isChainAction(){return chainAction;};
	int getEventId(){return eventId;};
	void setEventId(int id){eventId = id;};
	void fireEvent(){emit signalEvent(this);};


signals:
	void signalEvent(eveEventProperty*);

private:
	bool chainAction;	// registered by a chain or by a SM
	bool onstate;			// eventSource changed to true/on
	bool isConnected;
	int eventId;
	int smId;
	eveVariant eventValue;
	eveVariant eventLimit;
	actionTypeT actionType;
	eventTypeT eventType;
	QString comparison;
	// call eventAction too, if eventSource changes from true to false
	bool signalOff;
};

#endif /* EVEEVENTPROPERTY_H_ */
