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
}

/**
 *
 * @param dataVar add value to list of values for calculations if it passes the tests
 */
bool eveAverage::addValue(double value){

    if (allDone) return true;

    if ((attempt < maxAttempt) && (deviation > 0.0)){
        if ((lowLimit != 0.0) && (dataArray.size() == 0) && (fabs(value) < lowLimit)) {
            ++attempt;
            return false;
        }
        if ((dataArray.size() == 1) && (fabs(dataArray.at(0)*deviation/100.0) < fabs(dataArray.at(0)-value))) {
            ++attempt;
            return false;
        }
    }
    dataArray.append(value);

    if ((dataArray.size() >= averageCount) || (attempt > maxAttempt)) allDone = true;
    return true;
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

