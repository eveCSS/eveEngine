/*
 * eveDataStatus.h
 *
 *  Created on: 21.05.2012
 *      Author: eden
 */

#ifndef EVEDATASTATUS_H_
#define EVEDATASTATUS_H_

#include <QObject>

enum eveAcqStatusT {ACQSTATok, ACQSTATtimeout, ACQSTATmaxattempt};

class eveDataStatus {
public:
	eveDataStatus();
	eveDataStatus(quint8, quint8, quint16);
	virtual ~eveDataStatus();
	quint8 getSeverity(){return severity;};
	QString getSeverityString(){return convertToSeverityString(severity);};
	quint8 getAlarmCondition(){return condition;};
	QString getAlarmString(){return convertToAlarmString(condition);};
	quint16 getAcquisitionStatus(){return (quint16) acqStatus;};
	void setAcquisitionStatus(eveAcqStatusT status){acqStatus = status;};
	static QString convertToSeverityString(int severity);
	static QString convertToAlarmString(int severity);

private:
	quint8	severity;
	quint8	condition;
	eveAcqStatusT acqStatus;
};

#endif /* EVEDATASTATUS_H_ */
