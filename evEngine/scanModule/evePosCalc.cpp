/*
 * evePosCalc.cpp
 *
 *  Created on: 16.12.2008
 *      Author: eden
 */

#include <math.h>
#include <QTextStream>
#include <QFileInfo>
#include <QFile>
#include "evePosCalc.h"
#include "eveScanModule.h"
#include "eveMessage.h"

/**
 *
 * @param sm corresponding ScanModule, Stepfunction,
 * @param stepfunction
 * @param abs absolute (true) or relative (false)
 * @param type datatype of corresponding axis
 *
 */
evePosCalc::evePosCalc(eveScanModule* sm, QString stepfunction, bool abs, eveType type) {

	scanModule = sm;
	readyToGo = false;
	absolute = abs;
	totalSteps = -1;
	doNotMove = false;
	isAtEnd = false;

	if (stepfunction.toLower() == "add"){
		stepmode = STARTSTOP;
		stepFunction = &evePosCalc::stepfuncAdd;
	}
	else if (stepfunction.toLower() == "multiply"){
		stepmode = STARTSTOP;
		stepFunction = &evePosCalc::stepfuncDummy;
	}
	else if (stepfunction.toLower() == "double"){
		stepmode = STARTSTOP;
		stepFunction = &evePosCalc::stepfuncDummy;
	}
	else if (stepfunction.toLower() == "file"){
		stepmode = FILE;
		stepFunction = &evePosCalc::stepfuncList;
	}
	else if (stepfunction.toLower() == "list"){
		stepmode = LIST;
		stepFunction = &evePosCalc::stepfuncList;
	}
	else if (stepfunction.toLower() == "plugin"){
		stepmode = PLUGIN;
		stepFunction = &evePosCalc::stepfuncDummy;
	}
	else {
		stepmode = NONE;
		sendError(ERROR, QString("unknown Step-Function: %1").arg(stepfunction));
		stepFunction = &evePosCalc::stepfuncDummy;
	}
	axisType = type;
	if ((axisType != eveINT) &&  (axisType != eveDOUBLE) && (axisType != eveSTRING)
			&& (axisType != eveDateTimeT)) {
		sendError(ERROR, "unknown axis type (allowed values: int, double, string, datetime)");
	}

	switch (axisType) {
		case eveFloat32T:
		case eveFloat64T:
			startPos.setType(eveDOUBLE);
			endPos.setType(eveDOUBLE);
			startPosAbs.setType(eveDOUBLE);
			endPosAbs.setType(eveDOUBLE);
			currentPos.setType(eveDOUBLE);
			stepWidth.setType(eveDOUBLE);
			offSet.setType(eveDOUBLE);
			break;
		case eveStringT:
			startPos.setType(eveSTRING);
			endPos.setType(eveSTRING);
			startPosAbs.setType(eveSTRING);
			endPosAbs.setType(eveSTRING);
			currentPos.setType(eveSTRING);
			stepWidth.setType(eveINT);
			offSet.setType(eveINT);
			break;
		case eveDateTimeT:
			startPos.setType(eveDateTimeT);
			endPos.setType(eveDateTimeT);
			startPosAbs.setType(eveDateTimeT);
			endPosAbs.setType(eveDateTimeT);
			currentPos.setType(eveDateTimeT);
			stepWidth.setType(eveDOUBLE);
			offSet.setType(eveDateTimeT);
			break;
		default:
			startPos.setType(eveINT);
			endPos.setType(eveINT);
			startPosAbs.setType(eveINT);
			endPosAbs.setType(eveINT);
			currentPos.setType(eveINT);
			stepWidth.setType(eveINT);
			offSet.setType(eveINT);
			break;
	}
}

evePosCalc::~evePosCalc() {
	// TODO Auto-generated destructor stub
}

/**
 *
 * @param startpos start or end position relative or absolute
 * @param start true if startposition else endposition
 */
/**
 *
 * @param position start or end position relative or absolute
 * @param posVariant the variant startpos or endpos
 */
void evePosCalc::setPos(QString position, eveVariant* posVariant) {

	if ((axisType == eveDateTimeT) && absolute){
		// absolute: we accept only "yyyy-MM-dd HH:mm:ss(.zzz)" or "HH:mm:ss(.zzz)" format
		if (!position.contains(QRegExp("^\\d{4}-\\d{1,2}-\\d{1,2} \\d{1,2}:\\d{1,2}:\\d{1,2}([.]\\d{1,3})?$")) &&
									!position.contains(QRegExp("^\\d{1,2}:\\d{1,2}:\\d{1,2}([.]\\d{1,3})?$"))){
			sendError(ERROR, QString("invalid absolute datetime format %1, using current time").arg(position));
			posVariant->setValue(QDateTime::currentDateTime());
			return;
		}
	}
	else if ((axisType == eveDateTimeT) && !absolute){
		// relative: we accept only a number for seconds or HH:mm:ss format
		if (position.contains(QRegExp("^\\d{1,2}:\\d{1,2}:\\d{1,2}([.]\\d{1,3})?$"))){
			QString format;
			if (position.contains("."))
				format = "h:m:s.z";
			else
				format = "h:m:s";
			posVariant->setValue(QDateTime::fromString(position,format));
			return;
		}
		else {
			sendError(ERROR, QString("invalid relative datetime format %1, using 0").arg(position));
			posVariant->setValue(QDateTime());
			return;
		}
	}
	if (!posVariant->setValue(position))
		sendError(ERROR, QString("unable to set %1 as start/end position").arg(position));
	checkValues();
	return;
}

/**
 * \brief call setPos with endpos
 * @param pos
 */
void evePosCalc::setEndPos(QString pos) {
	setPos(pos, &endPos);
}

/**
 * \brief call setPos with startpos
 * @param pos
 */
void evePosCalc::setStartPos(QString pos) {
	setPos(pos, &startPos);
	if (startPos.getType() == eveDateTimeT) sendError(DEBUG, QString("setting startpos to %1").arg(startPos.toDateTime().toString()));
}

/**
 * \brief stepwidth is always relative (it is a double in case axistype == datetime)
 *
 * @param stepwidth to calculate the next step ( must be convertible to int or double)
 */
void evePosCalc::setStepWidth(QString stepwidth) {

	bool ok=false;
	if (axisType == eveDateTimeT){
		// stepwidth is a double, we accept a number or HH:mm:ss(.mmm) format
		QRegExp regex = QRegExp("^(\\d{1,2}):(\\d{1,2}):(\\d{1,2}([.]\\d{1,3})?)$");
		if (stepwidth.contains(regex) && (regex.numCaptures() > 3)){
			QStringList list = regex.capturedTexts();
			int hours = list.at(1).toInt(&ok);
			int minutes = list.at(2).toInt(&ok);
			double seconds = list.at(3).toDouble(&ok) + (double)(60*minutes + 3600*hours);
			stepWidth.setValue(seconds);
		}
		else {
			stepWidth.setValue(stepwidth.toDouble(&ok));
		}
		sendError(DEBUG, QString("set stepwidth to %1 (%2)").arg(stepWidth.toDouble(&ok)).arg(stepwidth));
	}
	else if (axisType == eveDOUBLE){
		stepWidth.setValue(stepwidth.toDouble(&ok));
	}
	else if (axisType == eveINT){
		stepWidth.setValue(stepwidth.toInt(&ok));
	}

	if (!ok) sendError(ERROR, QString("unable to set %1 as stepwidth").arg(stepwidth));
	checkValues();
}

/**
 *
 * @param stepfile name of file with steps
 */
void evePosCalc::setStepFile(QString stepfilename) {

	if (stepmode != FILE) return;

	posIntList.clear();
	posDoubleList.clear();
	positionList.clear();
	QFileInfo fileInfo(stepfilename);
	if (fileInfo.isRelative()){
		//TODO we might add an absolute Path from environment or parameter
	}
	if (fileInfo.exists() && fileInfo.isReadable()){
		QFile file(fileInfo.absoluteFilePath());
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
		     QTextStream inStream(&file);
	 		 bool ok;
		     while (!inStream.atEnd()) {
		         QString line = inStream.readLine().trimmed();
		         if (axisType == eveINT){
		 			posIntList.append(line.toInt(&ok));
		 			if (!ok) sendError(ERROR, QString("unable to set %1 as (integer) position from file %2").arg(line).arg(fileInfo.absoluteFilePath()));
		         } else if (axisType == eveDOUBLE){
		 			posDoubleList.append(line.toDouble(&ok));
		 			if (!ok) sendError(ERROR, QString("unable to set %1 as (double) position from file %2").arg(line).arg(fileInfo.absoluteFilePath()));
		         } else if (axisType == eveSTRING){
		        	positionList.append(line);
		         }
		     }
		     file.close();
		}
		else {
			sendError(ERROR, QString("unable to open position file %1").arg(fileInfo.absoluteFilePath()));
		}
	}
    if ((axisType == eveINT) && (posIntList.count() > 0)){
    	startPos = posIntList.at(0);
    	totalSteps = posIntList.count();
    } else if ((axisType == eveDOUBLE) && (posDoubleList.count() > 0)){
    	startPos = posDoubleList.at(0);
    	totalSteps = posDoubleList.count();
    } else if ((axisType == eveSTRING) && (positionList.count() > 0)){
    	startPos = positionList.at(0);
    	totalSteps = positionList.count();
    }
}

/**
 *
 * @param pluginname name of step plugin
 */
void evePosCalc::setStepPlugin(QString pluginname, QHash<QString, QString>& paraHash) {
	stepPlugin = pluginname;
	// The plugins ReferenceMultiply, MotionDisabled are not regular plugins, they are hardcoded
	if (pluginname == "ReferenceMultiply"){
		if (paraHash.contains("factor")){
			bool ok;
			RefMultiplyFactor = paraHash.value("factor").toDouble(&ok);
			if (!ok)sendError(ERROR, QString("unable to convert ReferenceMultiply factor to double %1").arg(paraHash.value("factor")));
		}
		if (paraHash.contains("referenceaxis")){
			RefMultiplyAxis = scanModule->findAxis(paraHash.value("referenceaxis"));
		}
		stepFunction = &evePosCalc::ReferenceMultiply;
	}
	if (pluginname == "MotionDisabled"){
		doNotMove = true;
		startPos=0;
		endPos=0;
		stepFunction = &evePosCalc::MotionDisabled;
		isAtEnd = true;
	}
}

void evePosCalc::setPositionList(QString poslist) {

	if (stepmode != LIST) return;

	positionList = poslist.split(",",QString::SkipEmptyParts);
	foreach (QString value, positionList){
		if (axisType == eveINT){
			bool ok;
			posIntList.append(value.toInt(&ok));
			if (!ok) sendError(ERROR, QString("unable to set %1 as (integer) position").arg(value));
		}
		else if (axisType == eveDOUBLE){
			bool ok;
			posDoubleList.append(value.toDouble(&ok));
			if (!ok) sendError(ERROR, QString("unable to set %1 as (double) position").arg(value));
		}
	}
    if ((axisType == eveINT) && (posIntList.count() > 0)){
    	startPos = posIntList.at(0);
    	totalSteps = posIntList.count();
    } else if ((axisType == eveDOUBLE) && (posDoubleList.count() > 0)){
    	startPos = posDoubleList.at(0);
    	totalSteps = posDoubleList.count();
    } else if ((axisType == eveSTRING) && (positionList.count() > 0)){
    	startPos = positionList.at(0);
    	totalSteps = positionList.count();
    }
}

/**
 * \brief calculate and return the next position, increment internal values
 *
 * @return the next valid motorposition if any, else a NULL value
 */
eveVariant& evePosCalc::getNextPos(){

	if (!isAtEnd){
		++posCounter;
		(this->*stepFunction)();
	}
	sendError(DEBUG, QString("next Position %1").arg(currentPos.toString()));
	return currentPos;
}

bool evePosCalc::setOffset(eveVariant offset){

	if (!absolute && !offset.isValid()){
		if(offset.getType() == eveDateTimeT){
			sendError(ERROR, QString("invalid absolute datetime %1, using current time").arg(offset.toDateTime().toString()));
			offSet.setValue(QDateTime::currentDateTime());
		}
		else {
			sendError(ERROR, "invalid absolute value, using 0");
			offSet.setValue((int)0);
		}
		return false;
	}
	offSet = offset;
	return true;
}

/**
 *
 * @return reset the internal position counter and find startpos/endpos if relative
 */
void evePosCalc::reset(){

	// here we add the current values to startpos/endpos if positionmode is relative
	// needs to be done here for Timer
	// CAUTION! addition is not commutative abs = abs + rel
	if (absolute){
		startPosAbs = startPos;
		endPosAbs = endPos;
	}
	else {
		startPosAbs = offSet + startPos;
		endPosAbs = offSet + endPos;
	}
	if (!startPosAbs.isValid() || !endPosAbs.isValid())
		sendError(ERROR, "startPos or endPos invalid");

	posCounter = 0;
	isAtEnd = false;
	if (doNotMove) isAtEnd = true;
	currentPos = startPosAbs;
}

/**
 *
 * @brief set the next valid motorposition
 */
void evePosCalc::stepfuncAdd(){

	if (axisType == eveSTRING){
		sendError(ERROR, "stepfunction add may be used with integer, double or datetime values only");
	}
	else {
		if ((currentPos.getType() == eveDateTimeT) && (stepWidth.getType() == eveDOUBLE)){
			sendError(DEBUG, QString("adding stepwidth %1 to current position %2").arg(stepWidth.toDouble()*1000.0).arg(currentPos.toDateTime().toString()));
			QDateTime dt = currentPos.toDateTime().addMSecs((qint64)(stepWidth.toDouble(NULL)*1000.0));
			currentPos.setValue(currentPos.toDateTime().addMSecs((qint64)(stepWidth.toDouble(NULL)*1000.0)));
			sendError(DEBUG, QString("new daytime %2, new current position %1").arg(currentPos.toDateTime().toString()).arg(dt.toString()));
		}
		else {
			currentPos = currentPos + stepWidth;
		}
		if (((stepWidth >=0) && (currentPos >= endPosAbs)) || ((stepWidth < 0) && (currentPos <= endPosAbs))){
			currentPos = endPosAbs;
			isAtEnd = true;
		}
	}
}

/**
 *
 * @brief read Motorpositions from File
 */
void evePosCalc::stepfuncList(){

	// posCounter == 0 => start Position,

	if (axisType == eveSTRING){
		if (posCounter >= (positionList.count()-1)) {
			isAtEnd = true;
			if (posCounter > positionList.count()) return;
		}
		else {
			currentPos.setValue(positionList.at(posCounter));
		}
	}
	else if (axisType == eveINT){
		if (posCounter >= (posIntList.count()-1)) {
			isAtEnd = true;
			if (posCounter > posIntList.count()) return;
		}
		else {
			currentPos.setValue(posIntList.at(posCounter));
		}
	}
	else if (axisType == eveDOUBLE){
		if (posCounter >= (posDoubleList.count())) {
			isAtEnd = true;
			if (posCounter > posDoubleList.count()) return;
		}
		else {
			currentPos.setValue(posDoubleList.at(posCounter));
		}
	}
}

/**
 * @brief stepfunction moves the axis to a multiple of the reference axis
 *
 */
void evePosCalc::ReferenceMultiply(){
	if (RefMultiplyAxis != NULL)
		currentPos = RefMultiplyAxis->getTargetPos() * RefMultiplyFactor;
	else
		sendError(ERROR, "ReferenceMultiply: invalid reference axis");
}

/**
 * @brief stepfunction does not move anything
 *
 */
void evePosCalc::MotionDisabled(){
	isAtEnd=true;
}

/**
 * @brief dummy Function
 *
 */
void evePosCalc::stepfuncDummy(){

	sendError(ERROR, "called unknown (dummy) stepfunction");
}

void evePosCalc::checkValues()
{
	if (stepmode == STARTSTOP){
		// TODO readyToGo is unused, start-/endPos are Null until after reset()
		readyToGo = false;
		bool ok1, ok2;

		if (startPos.isNull() || endPos.isNull() || stepWidth.isNull()) return;

		if ((startPos.getType() == eveINT) || (startPos.getType() == eveDOUBLE)){
			if (((startPos > endPos) && (stepWidth > 0)) || ((startPos < endPos) && (stepWidth < 0))){
				sendError(ERROR, "sign of stepwidth does not match startpos/endpos-values, using -1* stepwidth");
				stepWidth = stepWidth * eveVariant(-1);
			}
			if (startPos.getType() == eveINT) {
				int stepw = abs(stepWidth.toInt(&ok1));
				if (ok1 && (stepw > 0)){
					int distance = abs(endPos.toInt(&ok1)-startPos.toInt(&ok2));
					if (ok1 && ok2){
						totalSteps = distance/stepw;
						if ((distance%stepw) != 0) ++totalSteps;
					}
				}
			}
			else if (startPos.getType() == eveDOUBLE) {
				double stepw = fabs(stepWidth.toDouble(&ok1));
				if (ok1 && (stepw > 0.0)){
					double distance = fabs(endPos.toDouble(&ok1)-startPos.toDouble(&ok2));
					if (ok1 && ok2){
						totalSteps = (int) (distance/stepw);
						if (((double)totalSteps)*stepw < distance) ++totalSteps;
					}
				}
			}
		}
		readyToGo = true;
	}
}

void evePosCalc::sendError(int severity, QString message)
{
		scanModule->sendError(severity, EVEMESSAGEFACILITY_POSITIONCALC, 0,  message);
}

