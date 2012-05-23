/*
 * eveDataStatus.cpp
 *
 *  Created on: 21.05.2012
 *      Author: eden
 */

#include "eveDataStatus.h"

eveDataStatus::eveDataStatus() {
	severity = 0;
	condition = 0;
	acqStatus = ACQSTATok;
}

eveDataStatus::eveDataStatus(quint8 severity, quint8 condition, quint16 acquisitionStatus) {
	this->severity = (quint8) severity;
	this->condition = (quint8) condition;
	acqStatus = (eveAcqStatusT) acquisitionStatus;
}

eveDataStatus::~eveDataStatus() {
	// TODO Auto-generated destructor stub
}

QString eveDataStatus::convertToSeverityString(int severity){
	switch (severity) {
	case 0:
		return "NO_ALARM";
		break;
	case 1:
		return "MINOR";
		break;
	case 2:
		return "MAJOR";
		break;
	case 3:
		return "INVALID";
		break;
		default:
		return "Unknown Severity";
	}
}

QString eveDataStatus::convertToAlarmString(int severity){
	switch (severity) {
	case 0:
		return "NO_ALARM";
		break;
	case 1:
		return "READ";
		break;
	case 2:
		return "WRITE";
		break;
	case 3:
		return "HIHI";
		break;
	case 4:
		return "HIGH";
		break;
	case 5:
		return "LOLO";
		break;
	case 6:
		return "LOW";
		break;
	case 7:
		return "STATE";
		break;
	case 8:
		return "COS";
		break;
	case 9:
		return "COMM";
		break;
	case 10:
		return "TIMEOUT";
		break;
	case 11:
		return "HWLIMIT";
		break;
	case 12:
		return "CALC";
		break;
	case 13:
		return "SCAN";
		break;
	case 14:
		return "LINK";
		break;
	case 15:
		return "SOFT";
		break;
	case 16:
		return "BAD_SUB";
		break;
	case 17:
		return "UDF";
		break;
	case 18:
		return "DISABLE";
		break;
	case 19:
		return "SIMM";
		break;
	case 20:
		return "READ_ACCESS";
		break;
	case 21:
		return "WRITE_ACCESS";
		break;
	default:
		return "Unknown Alarm Condition";
	}
}
