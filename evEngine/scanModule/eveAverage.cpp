/*
 * eveAverage.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveAverage.h"
#include <cmath>
#include <exception>
#include "eveError.h"



/**
 *  Do average measurements and calculate the average
 *  If maxDeviation > 0 and/or limit != 0 filter accordingly
 *
 * @param average desired number of average measurements
 * @param maxAttempt max number of allowed attempts
 * @param lowLimit minimum of first value to be used if maxDeviation != 0
 * @param maxDeviation in percent
 */
eveAverage::eveAverage(int average, int maxAttempt, double lowLimit, double maxDeviation) {
    averageCount = abs(average);
    this->maxAttempt = abs(maxAttempt);
    this->lowLimit = fabs(lowLimit);
    this->deviation = fabs(maxDeviation);
    reset();
}

eveAverage::~eveAverage() {
    // Auto-generated destructor stub
}

/**
 * \brief reset and start a new calculation
 *
 */
void eveAverage::reset(){
    dataArray.clear();
    attempt = 0;
    allDone = false;
    doDeviationTest = true;
}

/**
 * check values:
 * if deviation > 0.0 enable deviationCheck
 * if limit > 0.0 and first value is below limit => disable deviationCheck
 * deviationCheck: start average measurement with the two values whith a value difference < deviation
 *
 * @param value add value to list of values for calculations if it passes the tests
 * @return false if value missed deviation test or value is NAN
 */
bool eveAverage::addValue(double value){

    if (allDone) return true;

    if ((attempt < maxAttempt) && (deviation > 0.0)){
        if ((lowLimit != 0.0) && (dataArray.size() == 0) && (fabs(value) < lowLimit)) {
            doDeviationTest = false;
        }
        if (doDeviationTest && (dataArray.size() == 1) && (fabs(dataArray.at(0)*deviation/100.0) < fabs(dataArray.at(0)-value))) {
            dataArray.replace(0, value);
            ++attempt;
            return false;
        }
    }
    dataArray.append(value);

    if (dataArray.size() >= averageCount) allDone = true;

    return true;
}

/**
 *replace value at specified array position
 *
 * @param value add value to list of values for calculations if it passes the tests
 * @return false if array position doesn't exist
 */
bool eveAverage::replaceValue(double value, int pos){

    if (dataArray.size() > pos) {
        dataArray.replace(pos, value);
        ++attempt;
        return true;
    }

    return false;
}

eveDataMessage* eveAverage::getResultMessage(){

    QVector<double> data;
    double result = NAN;
    double sum = 0.0;

    eveDataStatus dstatus(0,0,0);
    if (attempt >= maxAttempt) dstatus.setAcquisitionStatus(ACQSTATmaxattempt);

    if (dataArray.size() > 0){
        foreach(double value, dataArray){
            try {
                sum += value;
            } catch (std::exception& e) {
                eveError::log(ERROR, QString("C++ Exception eveAverage > sum += value <  %1").arg(e.what()), EVEMESSAGEFACILITY_MATH);
            }
        }
        result = sum / ((double)dataArray.size());
    }
    data.append(result);
    return new eveDataMessage(QString(), QString(), dstatus, DMTunmodified, eveTime(), data);
}

