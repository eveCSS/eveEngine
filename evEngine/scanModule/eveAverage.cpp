/*
 * eveAverage.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveAverage.h"
#include <cmath>
#include <exception>

/**
 * wait until average values have been taken before signaling ready
 * if lowLimit > 0.0 do average measurement only if first value > lowLimit
 * if lowLimit > 0.0 and deviation > 0.0 do average measurement only if first value > lowLimit
 * and start average accumulation if the deviation in percent of two consecutive values
 * is not more than maxDeviation.
 * If average count is not reached after maxattempt attempts, only the values
 * accumulated so far will be used for average calculation.
 * Nothing will be done, if value cannot be converted to double
 *
 * @param average desired number of average measurements
 * @param maxAttempt max number of allowed attempts
 * @param lowLimit minimum value to be used for average
 * @param maxDeviation in percent
 */
eveAverage::eveAverage(int average, int maxAttempt, double lowLimit, double maxDeviation) {
	averageCount = average;
	this->maxAttempt = maxAttempt;
	this->lowLimit = lowLimit;
	this->deviation = fabs(maxDeviation);
	if (lowLimit > 0.0)
		useLimit = true;
	else
		useLimit = false;
	if (useLimit && (maxDeviation > 0.0))
		useDeviation = true;
	else
		useDeviation = false;
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
	last_value = 0.0;
	attempt = 0;
	calcDone = false;
	lastTimeLevelReached = false;
	allDone = false;
	deviationCount = 0;
}

/**
 *
 * @param dataVar add value to list of values for calculations if it passes the tests
 */
void eveAverage::addValue(eveVariant dataVar){

	if (allDone) return;

	++attempt;
	if (dataVar.canConvert(QVariant::Double)){
		bool ok = false;
		double data = dataVar.toDouble(&ok);
		if (ok){
			last_value = data;
			calcDone = false;
			if (useLimit && (attempt == 1)){
				// if the first value doesn't hit the limit, we abort all checks
				if (checkLimit(data)){
					if (useDeviation) {
						checkDeviation(data);
					}
					else{
						dataArray.append(data);
					}
				}
				else{
					allDone = true;
				}
			}
			else if (useDeviation && (deviationCount < 2)){
				checkDeviation(data);
				// increase maxAttempt so we can collect averageCount messages
				if (deviationCount == 2) maxAttempt += averageCount-2;
			}
			else{
				dataArray.append(data);
			}
		}
	}
	else {
		allDone = true;
	}

	if (dataArray.size() >= averageCount) allDone = true;
	if (attempt >= maxAttempt) allDone = true;
}

/**
 * \brief check if value is larger than lowLimit
 *
 * @param value the value to test
 * @return true if value is not smaller than lowLimit
 */
bool eveAverage::checkLimit(double value){
	if (value < lowLimit) return false;
	return true;
}

/**
 *
 * \brief check if deviation <= allowed deviation; if true, accept this
 * and all following values for average calculation
 *
 * @param value the value to test
 */
void eveAverage::checkDeviation(double value){

	if (deviationCount == 0) {
		++ deviationCount;
		dataArray.append(value);
	}
	else if (deviationCount == 1){
		if (fabs(dataArray.at(0)*deviation/100.0) >= fabs(dataArray.at(0)-value)){
			++ deviationCount;
			dataArray.append(value);
		}
		else {
			dataArray.clear();
			dataArray.append(value);
		}
	}
	return;
}

eveVariant eveAverage::getResult(){

	if (!calcDone){
		calcDone = true;
		sum = 0.0;
		foreach(double value, dataArray){
			try {
				sum += value;
			} catch (std::exception& e) {
				printf("C++ Exception in eveAverage > sum += value <  %s\n",e.what());
			}
		}
	}

	if (dataArray.size() > 0)
		return QVariant(sum / ((double)dataArray.size()));
	else
		return QVariant(last_value);

}

eveDataMessage* eveAverage::getResultMessage(){
	QVector<double> data;
	eveDataStatus dstatus = {0,0,0};
	// TODO do we need an additional status for averageSuccess, averageAbort
	if (attempt >= maxAttempt) dstatus.acqStatus = (eveAcqStatus)ACQSTATmaxattempt;
	data.append(getResult().toDouble());
	return new eveDataMessage(QString(), QString(), dstatus, DMTunmodified, eveTime(), data);
}

