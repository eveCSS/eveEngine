/*
 * evePosCalc.cpp
 *
 *  Created on: 16.12.2008
 *      Author: eden
 */

#include "evePosCalc.h"

evePosCalc::evePosCalc(QString stepfunction, eveType type) {

	if (stepfunction.toLower() == "add"){
		stepFunction = eveSTEPFUNCTION_Add;
	}
	else if (stepfunction.toLower() == "multiply"){
		stepFunction = eveSTEPFUNCTION_Multiply;
	}
	else if (stepfunction.toLower() == "double"){
		stepFunction = eveSTEPFUNCTION_Double;
	}
	else if (stepfunction.toLower() == "file"){
		stepFunction = eveSTEPFUNCTION_File;
	}
	else if (stepfunction.toLower() == "plugin"){
		stepFunction = eveSTEPFUNCTION_Plugin;
	}
	else if (stepfunction.toLower() == "positionlist"){
		stepFunction = eveSTEPFUNCTION_Positionlist;
	}
	else {
		//TODO ERROR: unknown Step-Function
	}
	axisType = type;
	if ((axisType != eveINT) &&  (axisType != eveDOUBLE) && (axisType != eveSTRING)) {
		// TODO ERROR unknown type
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

	if (axisType == eveINT) {
		bool ok;
		startPos = new eveIntMotorPosition(startpos.toInt(&ok));
		// TODO if (!ok) error
	}
	else if (axisType == eveDOUBLE) {
		bool ok;
		startPos = new eveDoubleMotorPosition(startpos.toDouble(&ok));
		// TODO if (!ok) error
	}
	else
		startPos = new eveStringMotorPosition(startpos);
}

/**
 *
 * @param endpos axis endposition
 */
void evePosCalc::setEndPos(QString endpos) {

	if (axisType == eveINT) {
		bool ok;
		endPos = new eveIntMotorPosition(endpos.toInt(&ok));
		// TODO if (!ok) error
	}
	else if (axisType == eveDOUBLE) {
		bool ok;
		endPos = new eveDoubleMotorPosition(endpos.toDouble(&ok));
		// TODO if (!ok) error
	}
	else
		endPos = new eveStringMotorPosition(endpos);
}

/**
 *
 * @param stepwidth to calculate the next step ( must be convertible to int or double)
 */
void evePosCalc::setStepWidth(QString stepwidth) {

	if (axisType == eveINT) {
		bool ok;
		stepWidth = new eveIntMotorPosition(stepwidth.toInt(&ok));
		// TODO if (!ok) error
	}
	else if (axisType == eveDOUBLE) {
		bool ok;
		stepWidth = new eveDoubleMotorPosition(stepwidth.toDouble(&ok));
		// TODO if (!ok) error
	}
	else {
		bool ok;
		int value = stepwidth.toInt(&ok);
		// TODO if (!ok) error
		if (value < 0 ) value = -1 * value;	// for position strings we do not allow negative steps
		stepWidth = new eveIntMotorPosition(stepwidth.toInt(&ok));
	}
}

void evePosCalc::setStepPara(QString steppara) {
	stepPara = steppara;
}

void evePosCalc::setStepFile(QString stepfile) {
	stepFile = stepfile;
}

void evePosCalc::setPositionList(QString poslist) {
	positionList = poslist.split(",",QString::SkipEmptyParts);
	foreach (QString value, positionList){
		if (axisType == eveDOUBLE){
			bool ok;
			posIntList.append(value.toInt(&ok));
			// TODO if (!ok) error
		}
		else if (axisType == eveINT){
			bool ok;
			posDoubleList.append(value.toDouble(&ok));
			// TODO if (!ok) error
		}
	}
}

/**
 *
 * @return the next valid motorposition if any, else NULL
 */
eveMotorPosition* evePosCalc::getNextPos(){

	if (isAtEnd){
		return NULL;
	}
	++posCounter;
	if ( stepFunction == eveSTEPFUNCTION_Add){
		return funcAdd();
	}
	else if (stepFunction == eveSTEPFUNCTION_Multiply){
		// TODO
	}
	else if (stepFunction == eveSTEPFUNCTION_Double){
		// TODO
	}
	else if (stepFunction == eveSTEPFUNCTION_File){
		// TODO
	}
	else if (stepFunction == eveSTEPFUNCTION_Plugin){
		// TODO
	}
	else if (stepFunction == eveSTEPFUNCTION_Positionlist){
		// TODO
	}
	return NULL;
}

/**
 *
 * @return the startposition, reset the internal position counter
 */
eveMotorPosition* evePosCalc::getStartPos(){

	posCounter = 0;
	isAtEnd = false;
	if (axisType == eveINT){
		return new eveIntMotorPosition(((eveIntMotorPosition*)startPos)->getPosition());
	}
	else if (axisType == eveDOUBLE){
		return new eveDoubleMotorPosition(((eveDoubleMotorPosition*)startPos)->getPosition());
	}
	return new eveStringMotorPosition(((eveStringMotorPosition*)startPos)->getPosition());

}

/**
 *
 * @return the next valid motorposition if any, else NULL
 */
eveMotorPosition* evePosCalc::funcAdd(){

	if (axisType == eveINT){
		int stepwidth = ((eveIntMotorPosition*)stepWidth)->getPosition();
		int current = ((eveIntMotorPosition*)currentPos)->getPosition();
		int endpos = ((eveIntMotorPosition*)endPos)->getPosition();
		current += ((eveIntMotorPosition*)stepWidth)->getPosition();
		if (((stepwidth >=0) && (current >= endpos)) || ((stepwidth < 0) && (current <= endpos))){
			current = endpos;
			isAtEnd = true;
		}
		((eveIntMotorPosition*)currentPos)->setPosition(current);
		return new eveIntMotorPosition(current);
	}
	else if (axisType == eveDOUBLE){
		double stepwidth = ((eveDoubleMotorPosition*)stepWidth)->getPosition();
		double current = ((eveDoubleMotorPosition*)currentPos)->getPosition();
		double endpos = ((eveDoubleMotorPosition*)endPos)->getPosition();
		current += ((eveDoubleMotorPosition*)stepWidth)->getPosition();
		if (((stepwidth >=0) && (current >= endpos)) || ((stepwidth < 0) && (current <= endpos))){
			current = endpos;
			isAtEnd = true;
		}
		((eveDoubleMotorPosition*)currentPos)->setPosition(current);
		return new eveDoubleMotorPosition(current);
	}
	else if (axisType == eveSTRING){
		int stepwidth = ((eveIntMotorPosition*)stepWidth)->getPosition();
		int next = posCounter * stepwidth; // posCounter is already incremented
		if (next >= positionList.count()){
			isAtEnd = true;
			next = positionList.count()-1;
		}
		((eveStringMotorPosition*)currentPos)->setPosition(positionList.at(next));
		return new eveStringMotorPosition(positionList.at(next));
	}
	return NULL;
}

