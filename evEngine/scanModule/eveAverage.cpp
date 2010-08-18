/*
 * eveAverage.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveAverage.h"
#include <cmath>
#include <exception>

eveAverage::eveAverage(int average, int maxAttempt, double lowLimit, double maxDeviation) {
	averageCount = average;
	this->maxAttempt = maxAttempt;
	this->lowLimit = lowLimit;
	this->deviation = fabs(maxDeviation);
	reset();
}

eveAverage::~eveAverage() {
	// TODO Auto-generated destructor stub
}

void eveAverage::reset(){
	dataArray.clear();
	last_value = 0.0;
	attempt = 0;
}

bool eveAverage::isDone(){

	if (dataArray.size() >= averageCount) return true;
	if (attempt >= maxAttempt) return true;

	return false;
}

/**
 * for now all values are converted to doubles
 *
 * @param dataVar add value to list of values for calculations
 */
void eveAverage::addValue(eveVariant dataVar){
	++attempt;
	if (dataVar.canConvert(QVariant::Double)){
		bool ok = false;
		double data = dataVar.toDouble(&ok);
		if (ok){
			if (checkValue(data)){
				dataArray.append(data);
				calcDone = false;
			}
		}
	}
}

/**
 *
 * @param value the value to test
 * @return true if value is larger than lowLimit and deviation lower than maxDeviation
 */
bool eveAverage::checkValue(double value){
	lastTimeLevelReached = true;
	if ((lowLimit > 0.0) && (value <= lowLimit)) {
		lastTimeLevelReached = false;
		last_value = value;
		return false;
	}
	// deviation check is skipped if the first two consecutive values have been found
	if ((deviation > 0.0) && (dataArray.size() == 1 ) && (!lastTimeLevelReached ||
					(fabs(dataArray.last() - value) > deviation))){
		dataArray.clear();
		return true;
	}
	return true;
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
	if (attempt >= maxAttempt) dstatus.acqStatus = (eveAcqStatus)ACQSTATmaxattempt;
	data.append(getResult().toDouble());
	return new eveDataMessage(QString(), QString(), eveDataStatus(), DMTunmodified, eveTime(), data);
}

