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
    expectedPositions = 1;
    doNotMove = false;
    axisType = type;
    referenceOffset = 0.0;
    multiplyFactor = 1.0;
    referencePosCalc = NULL;

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
        offSet.setValue(0.0);
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
        if (absolute){
            startPos.setType(eveDateTimeT);
            endPos.setType(eveDateTimeT);
        }
        else {
            startPos.setType(eveDOUBLE);
            endPos.setType(eveDOUBLE);
        }
        startPosAbs.setType(eveDateTimeT);
        endPosAbs.setType(eveDateTimeT);
        currentPos.setType(eveDateTimeT);
        stepWidth.setType(eveDOUBLE);
        offSet.setType(eveDateTimeT);
        break;
    case eveINT:
        startPos.setType(eveINT);
        endPos.setType(eveINT);
        startPosAbs.setType(eveINT);
        endPosAbs.setType(eveINT);
        currentPos.setType(eveINT);
        stepWidth.setType(eveINT);
        offSet.setType(eveINT);
        offSet.setValue(0);
        break;
    default:
        sendError(ERROR, "unknown axis type (allowed values: int, double, string, datetime)");
        break;
    }

    if (stepfunction.toLower() == "add"){
        stepmode = STARTSTOP;
        stepFunction = &evePosCalc::stepfuncAdd;
        doneFunction = &evePosCalc::donefuncAdd;
    }
    else if (stepfunction.toLower() == "multiply"){
        // for multiply startPos, endPos, stepWidth hold the factor parameters
        stepmode = MULTIPLY;
        stepFunction = &evePosCalc::stepfuncMultiply;
        doneFunction = &evePosCalc::donefuncMultiply;
        startPos.setType(eveDOUBLE);
        endPos.setType(eveDOUBLE);
        stepWidth.setType(eveDOUBLE);
        if ((axisType != eveINT) &&  (axisType != eveDOUBLE))
            sendError(ERROR, "StepFunction Multiply may only be used with axes of type integer or double)");
        if (absolute) sendError(MINOR, "StepFunction Multiply: invalid absolute axis mode, switching to relative");
        absolute = false;
    }
    else if (stepfunction.toLower() == "double"){
        stepmode = STARTSTOP;
        stepFunction = &evePosCalc::stepfuncDummy;
        doneFunction = &evePosCalc::donefuncAlwaysTrue;
    }
    else if (stepfunction.toLower() == "file"){
        stepmode = FILE;
        stepFunction = &evePosCalc::stepfuncList;
        doneFunction = &evePosCalc::donefuncList;
    }
    else if (stepfunction.toLower() == "positionlist"){
        stepmode = LIST;
        stepFunction = &evePosCalc::stepfuncList;
        doneFunction = &evePosCalc::donefuncList;
    }
    else if (stepfunction.toLower() == "plugin"){
        stepmode = PLUGIN;
        stepFunction = &evePosCalc::stepfuncDummy;
        doneFunction = &evePosCalc::donefuncAlwaysTrue;
    }
    else {
        stepmode = NONE;
        sendError(ERROR, QString("unknown Step-Function: %1").arg(stepfunction));
        stepFunction = &evePosCalc::stepfuncDummy;
        doneFunction = &evePosCalc::donefuncAlwaysTrue;
    }
}

evePosCalc::~evePosCalc() {
    // TODO Auto-generated destructor stub
}

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
        // relative: we accept an ISO duration format or obsolete HH:mm:ss
        QRegExp duration = QRegExp("^P(\\d+)Y(\\d+)M(\\d+)DT(\\d+)H(\\d+)M([\\d.]+)S$");
        if (position.contains(duration) && (duration.numCaptures() == 6)){
            bool ok;
            QStringList list = duration.capturedTexts();
            if ((list.at(1).toInt(&ok)!= 0) || (list.at(2).toInt(&ok)!= 0) || (list.at(3).toInt(&ok)!= 0))
                sendError(MINOR, QString("ISO Duration not implemented for Y/M/D (%1)").arg(position));
            double seconds = list.at(4).toDouble(&ok)*3600 + list.at(5).toDouble(&ok)*60 + list.at(6).toDouble(&ok);
            posVariant->setValue(seconds);
            return ;
        }
        // obsolete relative format accepts only a number for seconds or HH:mm:ss format
        else if (position.contains(QRegExp("^\\d{1,2}:\\d{1,2}:\\d{1,2}([.]\\d{1,3})?$"))){
            // obsolete relative format accepts only a number for seconds or HH:mm:ss format
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
        // stepwidth is a double, we accept a number or HH:mm:ss.mmm format
        QRegExp regex = QRegExp("^(\\d+):(\\d+):([\\d.]+)$");
        QRegExp duration = QRegExp("^P(\\d+)Y(\\d+)M(\\d+)DT(\\d+)H(\\d+)M([\\d.]+)S$");
        if (stepwidth.contains(regex) && (regex.numCaptures() == 3)){
            QStringList list = regex.capturedTexts();
            int hours = list.at(1).toInt(&ok);
            int minutes = list.at(2).toInt(&ok);
            double seconds = list.at(3).toDouble(&ok) + (double)(60*minutes + 3600*hours);
            stepWidth.setValue(seconds);
        }
        else if (stepwidth.contains(duration) && (duration.numCaptures() == 6)){
            QStringList list = duration.capturedTexts();
            if ((list.at(1).toInt(&ok)!= 0) || (list.at(2).toInt(&ok)!= 0) || (list.at(3).toInt(&ok)!= 0))
                sendError(MINOR, QString("ISO Duration not implemented for Y/M/D (%1)").arg(stepwidth));
            double seconds = list.at(4).toDouble(&ok)*3600 + list.at(5).toDouble(&ok)*60 + list.at(6).toDouble(&ok);
            stepWidth.setValue(seconds);
        }
        else {
            stepWidth.setValue(stepwidth.toDouble(&ok));
        }
        sendError(DEBUG, QString("set stepwidth to %1 (%2)").arg(stepWidth.toDouble(&ok)).arg(stepwidth));
    }
    else if ((stepmode == MULTIPLY) || (axisType == eveDOUBLE)){
        stepWidth.setValue(stepwidth.toDouble(&ok));
    }
    else if (axisType == eveINT){
        stepWidth.setValue(stepwidth.toInt(&ok));
    }
    else if (axisType == eveStringT){
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
                if (line.isEmpty()) continue;
                if (axisType == eveINT){
                    int value = line.toInt(&ok);
                    if (ok)
                        posIntList.append(value);
                    else
                        sendError(ERROR, QString("unable to convert >%1< to integer position from file %2").arg(line).arg(fileInfo.absoluteFilePath()));
                } else if (axisType == eveDOUBLE){
                    double value = line.toDouble(&ok);
                    if (ok)
                        posDoubleList.append(value);
                    else
                        sendError(ERROR, QString("unable to convert >%1< to double position from file %2").arg(line).arg(fileInfo.absoluteFilePath()));
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
        startPos = posIntList.first();
        endPos = posIntList.last();
        expectedPositions = posIntList.count();
    } else if ((axisType == eveDOUBLE) && (posDoubleList.count() > 0)){
        startPos = posDoubleList.first();
        endPos = posDoubleList.last();
        expectedPositions = posDoubleList.count();
    } else if ((axisType == eveSTRING) && (positionList.count() > 0)){
        startPos = positionList.first();
        endPos = positionList.last();
        expectedPositions = positionList.count();
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
            multiplyFactor = paraHash.value("factor").toDouble(&ok);
            if (!ok)sendError(ERROR, QString("unable to convert ReferenceMultiply factor %1 to double").arg(paraHash.value("factor")));
        }
        if (paraHash.contains("referenceaxis")){
            refAxisName=paraHash.value("referenceaxis");
        }
        stepFunction = &evePosCalc::stepfuncReferenceMultiply;
        doneFunction = &evePosCalc::donefuncReferenceAxis;
    }
    else if (pluginname == "ReferenceAdd"){
        if (paraHash.contains("summand")){
            bool ok;
            referenceOffset = paraHash.value("summand").toDouble(&ok);
            if (!ok)sendError(ERROR, QString("unable to convert ReferenceAdd summand %1 to double").arg(paraHash.value("summand")));
        }
        if (paraHash.contains("referenceaxis")){
            refAxisName=paraHash.value("referenceaxis");
        }
        stepFunction = &evePosCalc::stepfuncReferenceAdd;
        doneFunction = &evePosCalc::donefuncReferenceAxis;
    }
    else if (pluginname == "MotionDisabled"){
        doNotMove = true;
        startPos=0;
        endPos=0;
        stepFunction = &evePosCalc::stepfuncMotionDisabled;
        doneFunction = &evePosCalc::donefuncAlwaysTrue;
    }
    else {
        sendError(ERROR, QString("unknown step plugin %1").arg(pluginname));
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
        startPos = posIntList.first();
        endPos = posIntList.last();
        expectedPositions = posIntList.count();
    } else if ((axisType == eveDOUBLE) && (posDoubleList.count() > 0)){
        startPos = posDoubleList.first();
        endPos = posDoubleList.last();
        expectedPositions = posDoubleList.count();
    } else if ((axisType == eveSTRING) && (positionList.count() > 0)){
        startPos = positionList.first();
        endPos = positionList.last();
        expectedPositions = positionList.count();
    }
}

/**
 * \brief calculate and return the next position, increment internal values
 *
 * @return the next valid motorposition if any, else a NULL value
 */
eveVariant& evePosCalc::getNextPos(){

    if (!(this->*doneFunction)()){
        ++posCounter;
        currentPos = (this->*stepFunction)();
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
    if (offSet.getType() == offset.getType()) offSet = offset;
    return true;
}

/**
 *
 * @return reset the internal position counter and find startpos/endpos if relative
 */
void evePosCalc::reset(){

    if (stepmode == MULTIPLY){
        startPosAbs = offSet * startPos;
        multiplyFactor = startPos.toDouble();
    }
    else if (!refAxisName.isEmpty()) {
        startPosAbs = (this->*stepFunction)();
    }
    else {
        // here we add the current values to startpos/endpos if positionmode is relative
        // needs to be done here for Timer
        // CAUTION! addition is not commutative abs = abs + rel
        if (absolute){
            startPosAbs = startPos;
            endPosAbs = endPos;
        }
        else {
            if (axisType == eveDateTimeT){
                bool ok;
                startPosAbs.setValue(offSet.toDateTime().addMSecs((quint64)(startPos.toDouble(&ok)*1000)));
                endPosAbs.setValue(offSet.toDateTime().addMSecs((quint64)(endPos.toDouble(&ok)*1000)));
            }
            else {
                startPosAbs = offSet + startPos;
                endPosAbs = offSet + endPos;
            }
        }
        if (!startPosAbs.isValid() || !endPosAbs.isValid())
            sendError(ERROR, "startPos or endPos invalid");
    }

    currentPos = startPosAbs;
    posCounter = 0;

}

/**
 *
 * @brief set the next valid motorposition by adding stepwidth to current position
 */
eveVariant evePosCalc::stepfuncAdd(){

    eveVariant nextPos(currentPos);

    if (axisType == eveSTRING){
        sendError(ERROR, "stepfunction add may be used with integer, double or datetime values only");
    }
    else {
        if ((nextPos.getType() == eveDateTimeT) && (stepWidth.getType() == eveDOUBLE)){
            sendError(DEBUG, QString("adding stepwidth %1 to current position %2").arg(stepWidth.toDouble()).arg(nextPos.toDateTime().toString()));
            nextPos.setValue(currentPos.toDateTime().addMSecs((qint64)(stepWidth.toDouble(NULL)*1000.0)));
        }
        else {
            nextPos = currentPos + stepWidth;
        }
        if (donefuncAdd()) nextPos=endPosAbs;
    }
    return nextPos;
}
/**
 *
 * @brief check if we reached the end
 */
bool evePosCalc::donefuncAdd(){

    bool done = false;

    if (axisType == eveSTRING){
        sendError(ERROR, "stepfunction add may be used with integer, double or datetime values only");
        done = true;
    }
    else {
        // special treatment for double
        if ((currentPos.getType() == eveDOUBLE)){
            double currentDouble = currentPos.toDouble();
            if (((stepWidth >=0) && ((currentDouble + fabs(1.0e-12 * currentDouble)) >= endPosAbs.toDouble())) ||
                    ((stepWidth < 0) && ((currentDouble - fabs(1.0e-12 * currentDouble)) <= endPosAbs.toDouble()))){
                done = true;
            }
        }
        else if (((stepWidth >=0) && (currentPos >= endPosAbs)) || ((stepWidth < 0) && (currentPos <= endPosAbs))){
            if (currentPos.getType() == eveDateTimeT)
                sendError(DEBUG, QString("at end: current position %1 endPosAbs %2 (%3)").arg(currentPos.toDateTime().toString()).arg(endPosAbs.toDateTime().toString()).arg(stepWidth.toDouble()));
            else
                sendError(DEBUG, QString("at end: current position %1 endPosAbs %2 (%3)").arg(currentPos.toInt()).arg(endPosAbs.toInt()).arg(stepWidth.toDouble()));
            done = true;
        }
    }
    return done;
}

/**
 *
 * @brief set the next valid motorposition
 */
eveVariant evePosCalc::stepfuncMultiply(){

    eveVariant nextPos(currentPos);

    if ((axisType != eveINT) &&  (axisType != eveDOUBLE)){
        sendError(ERROR, "stepfunction Multiply may be used with integer or double values only");
    }
    else {
        multiplyFactor += stepWidth.toDouble();

        if (donefuncMultiply()) multiplyFactor = endPos.toDouble();
        nextPos.setValue(offSet.toDouble() * multiplyFactor);
    }
    return nextPos;
}
/**
 *
 * @brief set the next valid motorposition
 */
bool evePosCalc::donefuncMultiply(){

    if ((axisType != eveINT) &&  (axisType != eveDOUBLE)){
        sendError(ERROR, "stepfunction Multiply may be used with integer or double values only");
        return true;
    }
    else {
        if (((stepWidth.toDouble() >=0) && ((multiplyFactor + fabs(1.0e-12 * multiplyFactor)) >= endPos.toDouble())) ||
                ((stepWidth.toDouble() < 0) && ((multiplyFactor - fabs(1.0e-12 * multiplyFactor)) <= endPos.toDouble())))
            return true;
    }
    return false;
}

/**
 *
 * @brief read Motorpositions from File
 */
eveVariant evePosCalc::stepfuncList(){

    eveVariant nextPos(currentPos);

    // posCounter == 0 => start Position,
    if (axisType == eveSTRING){
        if (posCounter < positionList.count())
            nextPos.setValue(positionList.at(posCounter));
    }
    else if (axisType == eveINT){
        if (posCounter < posIntList.count()){
            if (absolute)
                nextPos.setValue(posIntList.at(posCounter));
            else
                nextPos.setValue(posIntList.at(posCounter) + offSet.toDouble());;
        }
    }
    else if (axisType == eveDOUBLE){
        if (posCounter < posDoubleList.count()){
            if (absolute)
                nextPos.setValue(posDoubleList.at(posCounter));
            else
                nextPos.setValue(posDoubleList.at(posCounter) + offSet.toDouble());;
        }
    }
    return nextPos;
}
/**
 *
 * @brief check if list has been done
 */
bool evePosCalc::donefuncList(){

    // posCounter == 0 => start Position,

    if (((axisType == eveSTRING) && (posCounter >= (positionList.count()-1)))
            || ((axisType == eveINT) && (posCounter >= (posIntList.count()-1)))
            || ((axisType == eveDOUBLE) && (posCounter >= (posDoubleList.count()-1))))
        return true;
    return false;
}

/**
 * @brief stepfunction moves the axis to a multiple of the reference axis
 *
 */
eveVariant evePosCalc::stepfuncReferenceMultiply(){
    eveVariant nextPos(currentPos);
    if (referencePosCalc != NULL)
        nextPos = referencePosCalc->getCurrentPos() * multiplyFactor;
    else
        sendError(ERROR, "ReferenceMultiply: invalid reference axis");
    return nextPos;
}

/**
 * @brief stepfunction moves the axis to the position offset plus reference axis
 *
 */
eveVariant evePosCalc::stepfuncReferenceAdd(){
    eveVariant nextPos(currentPos);
    if (referencePosCalc != NULL)
        nextPos = referencePosCalc->getCurrentPos() + referenceOffset;
    else
        sendError(ERROR, "ReferenceAdd: invalid reference axis");
    return nextPos;
}

/**
 * @brief donefunction if axis uses reference axis
 *
 */
bool evePosCalc::donefuncReferenceAxis(){
    if (referencePosCalc != NULL) {
        return (referencePosCalc->isAtEndPos() && (currentPos == (this->*stepFunction)()));
    }
    sendError(ERROR, "ReferenceAdd: invalid reference axis");
    return true;
}

/**
 * @brief stepfunction does not move anything
 *
 */
eveVariant evePosCalc::stepfuncMotionDisabled(){
    return eveVariant(currentPos);
}

/**
 * @brief dummy Function
 *
 */
eveVariant evePosCalc::stepfuncDummy(){

    sendError(ERROR, "called unknown (dummy) stepfunction");
    return eveVariant(currentPos);
}
/**
 * @brief dummy done check
 *
 */
bool evePosCalc::donefuncDummy(){

    sendError(ERROR, "called unknown (dummy) done check");
    return true;
}

void evePosCalc::checkValues()
{
    if ((stepmode == STARTSTOP) || (stepmode == MULTIPLY)){
        // TODO readyToGo is unused, start-/endPos are Null until after reset()
        readyToGo = false;
        bool ok1, ok2;

        if (startPos.isNull() || endPos.isNull() || stepWidth.isNull()) return;

        if (startPos.getType() == eveINT) {
            if (((startPos > endPos) && (stepWidth > 0)) || ((startPos < endPos) && (stepWidth < 0))){
                sendError(ERROR, "sign of stepwidth does not match startpos/endpos-values, using -1* stepwidth");
                stepWidth = stepWidth * eveVariant(-1);
            }
            int stepw = abs(stepWidth.toInt(&ok1));
            if (ok1 && (stepw > 0)){
                int distance = abs(endPos.toInt(&ok1)-startPos.toInt(&ok2));
                if (ok1 && ok2){
                    // add one for converting steps to positions
                    expectedPositions = distance/stepw +1;
                    if ((distance%stepw) != 0) ++expectedPositions;
                }
            }
        }
        else if (startPos.getType() == eveDOUBLE) {
            if (((startPos > endPos) && (stepWidth > 0)) || ((startPos < endPos) && (stepWidth < 0))){
                sendError(ERROR, "sign of stepwidth does not match startpos/endpos-values, using -1* stepwidth");
                stepWidth = stepWidth * eveVariant(-1);
            }
            double stepw = fabs(stepWidth.toDouble(&ok1));
            if (ok1 && (stepw > 0.0)){
                double distance = fabs(endPos.toDouble(&ok1)-startPos.toDouble(&ok2));
                if (ok1 && ok2){
                    expectedPositions = (int) (distance/stepw);
                    if ((((double)expectedPositions) * stepw - distance) < -1e-12) ++expectedPositions;
                    ++expectedPositions;   // add one for converting steps to positions
                }
            }
        }
        else if (startPos.getType() == eveDateTimeT) {
            // this is absolute DateTime
            if (startPos > endPos){
                eveVariant tmp = startPos;
                startPos = endPos;
                endPos = tmp;
                sendError(ERROR, "using absolute time and start > end, Exchanging start and end");
                if (stepWidth.toDouble(&ok1) < 0.0) stepWidth = stepWidth * eveVariant(-1);
            }
            double stepw = fabs(stepWidth.toDouble(&ok1));
            if (ok1 && (stepw > 0.0)){
                double distance = fabs((double)(startPos.toDateTime().msecsTo(endPos.toDateTime())))/1000.0;
                if (ok1 && distance > 0.0){
                    expectedPositions = (int) (distance/stepw);
                    if ((((double)expectedPositions) * stepw - distance) < -1e-12) ++expectedPositions;
                    ++expectedPositions;   // add one for converting steps to positions
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

