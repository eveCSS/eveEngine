/*
 * evePosCalc.cpp
 *
 *  Created on: 16.12.2008
 *      Author: eden
 */

#include "evePosCalc.h"
#include "eveScanModule.h"
#include "eveMessage.h"

enum directionT {evePOSITIVE, eveNEGATIVE};

evePosCalc::evePosCalc(eveScanModule* sm, QString stepfunction, eveType type) {

	scanModule = sm;
	readyToGo = false;
	if (stepfunction.toLower() == "add"){
		stepFunction = &evePosCalc::stepfuncAdd;
	}
	else if (stepfunction.toLower() == "multiply"){
		stepFunction = &evePosCalc::stepfuncDummy;
	}
	else {
		sendMessage(ERROR, QString("unknown Step-Function: %1").arg(stepfunction));
		stepFunction = &evePosCalc::stepfuncDummy;
	}
	axisType = type;
	if ((axisType != eveINT) &&  (axisType != eveDOUBLE) && (axisType != eveSTRING)) {
		sendMessage(ERROR, "unknown axis type (allowed values: int, double, string)");
	}
	switch (axisType) {
		case eveFloat32T:
		case eveFloat64T:
			startPos.setType(eveDOUBLE);
			endPos.setType(eveDOUBLE);
			currentPos.setType(eveDOUBLE);
			stepWidth.setType(eveDOUBLE);
			break;
		case eveStringT:
			startPos.setType(eveSTRING);
			endPos.setType(eveSTRING);
			currentPos.setType(eveSTRING);
			stepWidth.setType(eveINT);
			break;
		default:
			startPos.setType(eveINT);
			endPos.setType(eveINT);
			currentPos.setType(eveINT);
			stepWidth.setType(eveINT);
			break;
	}
}

evePosCalc::~evePosCalc() {
	// TODO Auto-generated destructor stub
}

/**
 *
 * @param startpos axis startposition
 */
void evePosCalc::setStartPos(QString startpos) {

	if (!startPos.setValue(startpos))
		sendMessage(ERROR, QString("unable to set %1 as start position").arg(startpos));
	checkValues();
}

/**
 *
 * @param endpos axis endposition
 */
void evePosCalc::setEndPos(QString endpos) {

	if (!endPos.setValue(endpos))
		sendMessage(ERROR, QString("unable to set %1 as End position").arg(endpos));
}

/**
 *
 * @param stepwidth to calculate the next step ( must be convertible to int or double)
 */
void evePosCalc::setStepWidth(QString stepwidth) {

	bool ok=false;
	if (axisType == eveINT){
		stepWidth.setValue(stepwidth.toInt(&ok));
	}
	else if (axisType == eveDOUBLE){
		stepWidth.setValue(stepwidth.toDouble(&ok));
	}
	if (!ok) sendMessage(ERROR, QString("unable to set %1 as stepwidth").arg(stepwidth));
}

/**
 *
 * @param paraname parameter name
 * @param paravalue parameter value
 */
void evePosCalc::setStepPara(QString paraname, QString paravalue) {
	conParaHash.insert(paraname, paravalue);
}

/**
 *
 * @param stepfile name of file with steps
 */
void evePosCalc::setStepFile(QString stepfile) {
	stepFile = stepfile;
}

/**
 *
 * @param pluginname name of step plugin
 */
void evePosCalc::setStepPlugin(QString pluginname) {
	stepPlugin = pluginname;
}

void evePosCalc::setPositionList(QString poslist) {
	positionList = poslist.split(",",QString::SkipEmptyParts);
	foreach (QString value, positionList){
		if (axisType == eveINT){
			bool ok;
			posIntList.append(value.toInt(&ok));
			if (!ok) sendMessage(ERROR, QString("unable to set %1 as (integer) position").arg(value));
		}
		else if (axisType == eveDOUBLE){
			bool ok;
			posDoubleList.append(value.toDouble(&ok));
			if (!ok) sendMessage(ERROR, QString("unable to set %1 as (double) position").arg(value));
		}
	}
}

/**
 *
 * @return the next valid motorposition if any, else a NULL value
 */
eveVariant& evePosCalc::getNextPos(){

	bool ok = true;

	if (!isAtEnd){
		++posCounter;
		(this->*stepFunction)();
	}
	if (ok)
		return currentPos;
	else
		return nullVal;
}

/**
 *
 * @return reset the internal position counter
 */
void evePosCalc::reset(){

	posCounter = 0;
	isAtEnd = false;
	currentPos = startPos;
}

/**
 *
 * @brief set the next valid motorposition
 */
void evePosCalc::stepfuncAdd(){

	if (axisType == eveSTRING){
		int stepwidth = stepWidth.toInt();
		int next = posCounter * stepwidth; // posCounter is already incremented
		if (next >= positionList.count()){
			isAtEnd = true;
			next = positionList.count()-1;
		}
		currentPos.setValue(positionList.at(next));
	}
	else {
		currentPos = currentPos + stepWidth;
		if (((stepWidth >=0) && (currentPos >= endPos)) || ((stepWidth < 0) && (currentPos <= endPos))){
			currentPos = endPos;
			isAtEnd = true;
		}
	}
}

/**
 * @brief dummy Function
 *
 */
void evePosCalc::stepfuncDummy(){

	sendMessage(ERROR, "called unknown (dummy) stepfunction");
}

void evePosCalc::checkValues()
{
	readyToGo = false;
	if (startPos.isNull() || endPos.isNull() || stepWidth.isNull()) return;

	if ((startPos.getType() == eveINT) || (startPos.getType() == eveDOUBLE)){
		if (((startPos > endPos) && (stepWidth > 0)) || ((startPos < endPos) && (stepWidth < 0))){
			sendMessage(ERROR, "sign of stepwidth does not match startpos/endpos-values");
			stepWidth = stepWidth * eveVariant(-1);

		}

	}
}

void evePosCalc::sendMessage(int severity, QString message)
{
		scanModule->sendError(severity, EVEMESSAGEFACILITY_POSITIONCALC, 0,  message);
}

