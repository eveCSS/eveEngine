/*
 * eveEventProperty.h
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#ifndef EVEEVENTPROPERTY_H_
#define EVEEVENTPROPERTY_H_

#include <QObject>
#include <QString>
#include "eveVariant.h"
#include "eveDevice.h"

enum eventTypeT {eveEventTypeMONITOR=1, eveEventTypeSCHEDULE, eveEventTypeDETECTOR} ;
enum incidentTypeT {eveIncidentNONE, eveIncidentSTART, eveIncidentEND} ;

/*
 *
 */
class eveEventProperty : public QObject {

	Q_OBJECT

public:
	enum actionTypeT {NONE, START, PAUSE, HALT, BREAK, STOP, REDO, TRIGGER} ;

	eveEventProperty(QString, QString, eveVariant limit, eventTypeT, incidentTypeT, actionTypeT, eveDeviceCommand*);
	virtual ~eveEventProperty();
	void setOn(bool state){onstate = state;};
	bool getOn(){return onstate;};
	void sendEvent(){emit signalEvent(this);};
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
	void setSignalOff(bool doOffToo){signalOff=doOffToo;};
	bool getSignalOff(){return signalOff;};
	eveDeviceCommand* getDevCommand(){return devCommand;};
	QString getCompareOperator(){return comparison;};
	QString getName(){return name;};
	incidentTypeT getIncident(){return incidentType;};

signals:
	void signalEvent(eveEventProperty*);

private:
	bool chainAction;	// registered by a chain or by a SM
	bool onstate;			// eventSource changed to true/on
	bool isConnected;
	int eventId;
	int smId;
	QString name;
	eveVariant eventValue;
	eveVariant eventLimit;
	actionTypeT actionType;
	eventTypeT eventType;
	incidentTypeT incidentType;
	QString comparison;
	eveDeviceCommand* devCommand;;
	// call eventAction too, if eventSource changes from true to false
	bool signalOff;
};

#endif /* EVEEVENTPROPERTY_H_ */
