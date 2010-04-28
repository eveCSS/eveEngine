/*
 * eveMath.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveMath.h"
#include <cmath>
#include <exception>

eveMath::eveMath(int type) {
	mathType = type;
	averageCount = 1;
	maxAttempt = 1;
	reset();
}

eveMath::eveMath(int average, int maxAttempt, double lowLimit, double maxDeviation) {
	mathType = EVEMATH_AVERAGEWINDOW;
	averageCount = average;
	this->maxAttempt = maxAttempt;
	this->lowLimit = lowLimit;
	this->deviation = fabs(maxDeviation);
	reset();
}

eveMath::~eveMath() {
	// TODO Auto-generated destructor stub
}

void eveMath::reset(int type){
	mathType = type;
	reset();
}

void eveMath::reset(){
	dataArray.clear();
	last_value = 0.0;
	attempt = 0;
}

bool eveMath::isDone(){

	if (dataArray.size() >= averageCount) return true;
	if (attempt >= maxAttempt) return true;

	return false;
}

/**
 * for now all values are converted to doubles
 *
 * @param dataVar add value to list of values for calculations
 */
void eveMath::addValue(eveVariant dataVar){
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
bool eveMath::checkValue(double value){
	if (mathType == EVEMATH_AVERAGEWINDOW){
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
	}
	return true;
}

eveVariant eveMath::getResult(int type){

	if (!calcDone){
		calcDone = true;
		minimum = 1.0e300;
		maximum = 1.0e-300;
		sum = 0.0;
		foreach(double value, dataArray){
			if (value > maximum) maximum = value;
			if (value < minimum) minimum = value;
			try {
				sum += value;
			} catch (std::exception& e) {
				printf("C++ Exception in eveMath > sum += value <  %s\n",e.what());
			}
		}
	}

	switch (type) {
		case EVEMATH_MIN:
			return QVariant(minimum);
			break;
		case EVEMATH_MAX:
			return QVariant(maximum);
			break;
		case EVEMATH_CENTER:
			return QVariant(QVariant::Double);
			break;
		case EVEMATH_EDGE:
			return QVariant(QVariant::Double);
			break;
		case EVEMATH_FWHM:
			return QVariant(QVariant::Double);
			break;
		case EVEMATH_DEVIATION:
			if (dataArray.size() > 1) {
				int count = dataArray.count();
				double psum = 0.;
				double mean = sum / count;
				foreach (double val, dataArray){
					psum += pow((val-mean),2.0);
				}
				return QVariant(sqrt(psum / (double)(count -1)));
			}
			break;
		case EVEMATH_AVERAGE:
		case EVEMATH_AVERAGEWINDOW:
			if (dataArray.size() > 0)
				return QVariant(sum / ((double)dataArray.size()));
			else
				return QVariant(last_value);
			break;
		case EVEMATH_SUM:
			return QVariant(sum);
			break;
		default:
			break;
	}
	return QVariant(QVariant::Double);
}

eveDataMessage* eveMath::getResultMessage(int type){
	QVector<double> data;
	eveDataStatus dstatus = {0,0,0};
	if ((mathType == EVEMATH_AVERAGEWINDOW) && (attempt >= maxAttempt)) dstatus.acqStatus = (eveAcqStatus)ACQSTATmaxattempt;
	data.append(getResult(type).toDouble());
	return new eveDataMessage(QString(), QString(), eveDataStatus(), toDataMod(type), epicsTime(), data);
}

eveDataModType eveMath::toDataMod(int type){
	switch (type) {
		case EVEMATH_MIN:
			return DMTmin;
			break;
		case EVEMATH_MAX:
			return DMTmax;
			break;
		case EVEMATH_CENTER:
			return DMTcenter;
			break;
		case EVEMATH_EDGE:
			return DMTedge;
			break;
		case EVEMATH_FWHM:
			return DMTfwhm;
			break;
		case EVEMATH_DEVIATION:
			return DMTstandarddev;
			break;
		case EVEMATH_AVERAGE:
			return DMTmean;
			break;
		case EVEMATH_SUM:
			return DMTsum;
			break;
		default:
			break;
	}
	return DMTunmodified;
}
