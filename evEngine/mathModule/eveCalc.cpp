/*
 * eveCalc.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveCalc.h"
#include <cmath>
#include <exception>

#define MATH EVEMESSAGEFACILITY_MATH

/**
 * eveCalc does calculations for one detector/normalize-detector/motor triple
 *
 * @param manag BaseManager, needed to send errors
 * @param calcAlgorithm select a specific algorithm or ALL for all algorithms
 * @param xAxis id of x-axis
 * @param yAxis id of y-axis
 * @param normAxis id of normalization axis
 *
 */
eveCalc::eveCalc(eveMessageChannel* manag, QString calcAlgorithm, QString xAxis, QString yAxis, QString normAxis) {
	manager = manag;
	xAxisId = xAxis;
	detectorId = yAxis;
	normalizeId = normAxis;
	algorithm = toMathAlgorithm(calcAlgorithm);
	if (normalizeId.isEmpty())
		doNormalize = false;
	else
		doNormalize = true;
	saveValues = true;
	// don't save all values for PEAK, MIN, MAX, SUM
	if ((algorithm == PEAK) || (algorithm == MIN) || (algorithm == MAX) || (algorithm == SUM)) saveValues = false;
	reset();

}

eveCalc::eveCalc(eveMathConfig mathConfig, eveMessageChannel* manag) {
	manager = manag;
	xAxisId = QString(mathConfig.getXAxis());
	detectorId = QString(mathConfig.getDetector());
	normalizeId = QString(mathConfig.getNormalizeDetector());
	algorithm = UNKNOWN;
	if (normalizeId.isEmpty())
		doNormalize = false;
	else
		doNormalize = true;
	saveValues = true;
	reset();

}

eveCalc::~eveCalc() {
	// TODO Auto-generated destructor stub
}

QList<MathAlgorithm> eveCalc::getAlgorithms(){
	QList<MathAlgorithm> list;
	list += PEAK;
	list += MIN;
	list += MAX;
	list += CENTER;
	list += EDGE;
	list += FWHM;
	list += STD_DEVIATION;
	list += MEAN;
	list += SUM;
	return list;
}

void eveCalc::reset(){
	xdataArray.clear();
	ydataArray.clear();
	xpos = -1;
	ypos = -1;
	zpos = -1;
	position = -1;
	count = -1;
	ymin = 1.0e300;
	ymax = 1.0e-300;
	sum = 0.0;
	arrayModified = false;
	minIndex = 0;
	maxIndex = 0;
}

/**
 * for now all values are converted to doubles
 *
 * @param deviceId device
 * @param dataVar positionCount of scan module
 * @param dataVar current data value
 * @return true if normalized value has been calculated
 */
bool eveCalc::addValue(QString deviceId, int pos, eveVariant dataVar){
	bool retval = false;

	manager->sendError(MINOR, MATH, 0, QString("Calc new value for %1, val %2 pos %3 (%4 / %5 / %6)").arg(deviceId).arg(dataVar.toDouble()).arg(pos).arg(xAxisId).arg(detectorId).arg(normalizeId));

	if (deviceId == xAxisId) {
		// set position at first invocation
		if (position == -1) position = pos -1;
		xpos = pos;
	}
	if (dataVar.canConvert(QVariant::Double)){
		bool ok = false;
		double data = dataVar.toDouble(&ok);
		if (ok){
			if (deviceId == xAxisId) {
				xdata = data;
			}
			else if (deviceId == detectorId) {
				ydata = data;
				ypos = pos;
			}
			else if (doNormalize && (deviceId == normalizeId)) {
				zdata = data;
				zpos = pos;
			}
			else
				return retval;

			if (((xpos == ypos ) && doNormalize && (xpos == zpos)) || ((xpos == ypos ) && !doNormalize )) {
				if (xpos == position){
					manager->sendError(MINOR, MATH, 0, QString("position  %1 has already been done").arg(position));
					return retval;
				}

				manager->sendError(MINOR, MATH, 0, QString("Calc values: %1 / %2 / %3 (%4 / %5 / %6)").arg(xdata).arg(ydata).arg(zdata).arg(xAxisId).arg(detectorId).arg(normalizeId));

				++position;
				if (xpos != position){
					//manager->sendError(MINOR, MATH, 0, QString("possibly missed calculation for position  %1").arg(position));
					position = xpos;
				}
				if (doNormalize) {
					if (fabs(zdata) > 0.0) {
						ydata /= zdata;
						retval = true;
					}
					else {
						ydata = 0.0;
						manager->sendError(MINOR, MATH, 0, QString("unable to normalize with normalize detector value 0.0"));
					}
				}
				++count;
				if ((saveValues)||(count == 0)){
					// we always save the start values
					xdataArray.append(xdata);
					ydataArray.append(ydata);
					arrayModified = true;
				}
				if (!saveValues){
					if ((algorithm == MIN)||(algorithm == MAX)||(algorithm == PEAK)){
						if (ydata < ymin) {
							ymin = ydata;
							xmin = xdata;
						}
						if (ydata > ymax) {
							ymax = ydata;
							xmax = xdata;
						}
					}
					else if (algorithm == SUM){
							yresult += ydata;
					}
				}
			}
		}
	}
	return retval;
}

/**
 * \brief used only if dataArrays are not used
 */
bool eveCalc::setResult(MathAlgorithm algo){

	if (algo == MIN){
		if (count < 1) return false;
		xresult = xmin;
		yresult = ymin;
		return true;
	}
	else if (algo == MAX){
		if (count < 1) return false;
		xresult = xmax;
		yresult = ymax;
		return true;
	}
	else if (algo == PEAK){
		if (count < 3) return false;
		double maxpeak = 0.0;
		double minpeak = 0.0;
		// find the peak (either max or min)
		// we assume the higher peak to be the desired peak
		if ((ymax > ydataArray.at(0)) && (ymax > ydata)){
			maxpeak = ymax - (ydataArray.at(0) + ydata)/2.0;
		}
		if ((ymin < ydataArray.at(0)) && (ymin < ydata)){
			minpeak = (ydataArray.at(0) + ydata)/2.0 - ymin;
		}

		if (minpeak == maxpeak)
			return false;
		else if (minpeak > maxpeak){
			xresult = xmin;
			yresult = ymin;
			return true;
		}
		else {
			xresult = xmax;
			yresult = ymax;
			return true;
		}
	}
	else if (algo == SUM){
		xresult = 0.;
		yresult = sum;
		return true;
	}
	return false;
}

bool eveCalc::calculate(MathAlgorithm algo){

	bool retval = false;

	if (!saveValues) return setResult(algo);

	if (!getAlgorithms().contains(algo)) return false;

	if (arrayModified){
		arrayModified = false;
		// we have at least 1 value in dataArray
		// calculate MIN MAX PEAK
		xmin=0.0;
		xmax=0.0;
		ymin = ydataArray.at(0);
		ymax = ydataArray.at(0);
		sum = 0.0;
		int index = 0;
		minIndex = index;
		maxIndex = index;
		foreach(double value, ydataArray){
			if (value > ymax) {
				ymax = value;
				maxIndex = index;
			}
			if (value < ymin) {
				ymin = value;
				minIndex = index;
			}
			try {
				sum += value;
			} catch (std::exception& e) {
				printf("C++ Exception in eveCalc > sum += value <  %s\n",e.what());
			}
			++index;
		}
		xmax = xdataArray.at(maxIndex);
		xmin = xdataArray.at(minIndex);
	}
	xresult = 0.0;
	yresult = 0.0;
	switch (algo) {
		case PEAK:
			if (ydataArray.size() > 1){
				double first = ydataArray.at(0);
				double last = ydataArray.at(ydataArray.size()-1);
				double maxpeak = 0.0;
				double minpeak = 0.0;
				if ((ymax > first) && (ymax > last)){
					maxpeak = ymax - (first + last)/2.0;
				}
				if ((ymin < first) && (ymin < last)){
					minpeak = (first + last)/2.0 - ymin;
				}
				if (minpeak > maxpeak){
					xresult = xmin;
					yresult = ymin;
					retval = true;
				}
				else if (minpeak < maxpeak) {
					xresult = xmax;
					yresult = ymax;
					retval = true;
				}
			}
			break;
		case MIN:
			if (ydataArray.size() > 0){
				yresult = ymin;
				xresult = xmin;
				retval = true;
			}
			break;
		case MAX:
			if (ydataArray.size() > 0){
				yresult = ymax;
				xresult = xmax;
				retval = true;
			}
			break;
		case CENTER:
			if (calculate(PEAK)){
				int index;
				if (xresult == xmin)
					index = minIndex;
				else
					index = maxIndex;
				//TODO not yet ready
				double halfMax = (ymax-ymin)/2;
				int upperIndex, lowerIndex;
				for (int i = index; i < ydataArray.size(); ++i){
					upperIndex = i;
					if (fabs(yresult-ydataArray.at(i)) > halfMax) break;
				}
				for (int i = index; i >= 0; --i){
					lowerIndex = i;
					if (fabs(yresult-ydataArray.at(i)) > halfMax) break;
				}
				xresult = xdataArray.at(lowerIndex) + (xdataArray.at(upperIndex) - xdataArray.at(lowerIndex))/2.0;
				yresult = 0.0;
				fwhm = fabs(xdataArray.at(upperIndex) - xdataArray.at(lowerIndex));
				retval = true;
			}
			break;
		case EDGE:
			if (ydataArray.size() > 1) {
				double mmax = 0.0;
				int index = -1;
				for (int i=0; i < ydataArray.count()-1; ++i){
					if (xdataArray.at(i+1)-xdataArray.at(i) == 0.0) break;
					double m = fabs((ydataArray.at(i+1)-ydataArray.at(i))/(xdataArray.at(i+1)-xdataArray.at(i)));
					if (m > mmax) {
						mmax = m;
						index = i;
					}
				}
				if (index >= 0){
					xresult = (xdataArray.at(index+1) + xdataArray.at(index))/2.0;
					yresult = 0.0;
					retval = true;
				}
			}
			break;
		case FWHM:
			if (calculate(CENTER)){
				yresult = fwhm;
				xresult = 0.0;
				retval = true;
			}
			break;
		case STD_DEVIATION:
			if (ydataArray.size() > 1) {
				int count = ydataArray.count();
				double psum = 0.;
				double mean = sum / count;
				foreach (double val, ydataArray){
					psum += pow((val-mean),2.0);
				}
				xresult = 0.0;
				yresult = sqrt(psum / (double)(count -1));
				retval = true;
			}
			break;
		case MEAN:
			if (ydataArray.size() > 0) {
				xresult = 0.0;
				yresult = (sum / ((double)ydataArray.size()));
				retval = true;
			}
			break;
		case SUM:
			if (ydataArray.size() > 0) {
				xresult = 0.0;
				yresult = sum;
				retval = true;
			}
			break;
		default:
			break;
	}
	return retval;
}

eveDataModType eveCalc::toDataMod(MathAlgorithm algo){
	switch (algo) {
		case PEAK:
			return DMTpeak;
			break;
		case MIN:
			return DMTmin;
			break;
		case MAX:
			return DMTmax;
			break;
		case CENTER:
			return DMTcenter;
			break;
		case EDGE:
			return DMTedge;
			break;
		case FWHM:
			return DMTfwhm;
			break;
		case STD_DEVIATION:
			return DMTstandarddev;
			break;
		case MEAN:
			return DMTmean;
			break;
		case SUM:
			return DMTsum;
			break;
		default:
			return DMTunknown;
			break;
	}
	// never reached
	return DMTunmodified;
}

MathAlgorithm eveCalc::toMathAlgorithm(QString AlgoString){

	MathAlgorithm returnAlgo = UNKNOWN;
	if (AlgoString == "ALL") returnAlgo = ALL;
	else if (AlgoString == "PEAK") returnAlgo = PEAK;
	else if (AlgoString == "MIN") returnAlgo = MIN;
	else if (AlgoString == "MAX") returnAlgo = MAX;
	else if (AlgoString == "CENTER") returnAlgo = CENTER;
	else if (AlgoString == "EDGE") returnAlgo = EDGE;
	else if (AlgoString == "FWHM") returnAlgo = FWHM;
	else if (AlgoString == "STD_DEVIATION") returnAlgo = STD_DEVIATION;
	else if (AlgoString == "MEAN") returnAlgo = MEAN;
	else if (AlgoString == "SUM") returnAlgo = SUM;

	return returnAlgo;
}
