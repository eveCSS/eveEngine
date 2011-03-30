/*
 * eveMath.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveMath.h"
#include "eveMathManager.h"
#include <cmath>
#include <exception>

/**
 * eveMath does calculations for one detector/normalize-detector/motor triple
 *
 * @param mathConfig configuration data for math
 * @return
 */
eveMath::eveMath(eveMathConfig mathConfig, eveMathManager* manag) : eveMathConfig(mathConfig){
	mmanager = manag;
	if (normalizeId.length() > 0)
		doNormalize = true;
	else
		doNormalize = false;
	reset();
	modified = false;
	minimum = 0.0;
	maximum = 0.0;
	sum = 0.0;
	minIndex = 0;
	maxIndex = 0;

}

eveMath::~eveMath() {
	// TODO Auto-generated destructor stub
}

QList<MathAlgorithm> eveMath::getAlgorithms(){
	QList<MathAlgorithm> list;
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

void eveMath::reset(){
	xdataArray.clear();
	ydataArray.clear();
	xpos = -1;
	ypos = -2;
	doYNorm = false;
}

/**
 * for now all values are converted to doubles
 *
 * @param dataVar add value to list of values for calculations
 */
void eveMath::addValue(QString deviceId, int smid, int pos, eveVariant dataVar){
	if (deviceId == xAxisId) {
		xpos = pos;
		// TODO remove
		//mmanager->sendError(DEBUG, 0, QString("xpos: %1").arg(xpos));

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
				doYNorm = true;
				// TODO remove
				//mmanager->sendError(DEBUG, 0, QString("ypos: %1, value: %2").arg(ypos).arg(data));
			}
			else if (doNormalize && (deviceId == normalizeId)) {
				zdata = data;
				zpos = pos;
				// TODO remove
				//mmanager->sendError(DEBUG, 0, QString("zpos: %1, value: %2").arg(zpos).arg(data));
			}
			else
				return;

			if (((xpos == ypos ) && doNormalize && (xpos == zpos)) || ((xpos == ypos ) && !doNormalize )) {
				if (doNormalize && doYNorm) {
					doYNorm = false;
					eveDataStatus status = {1,1,1};
					if (fabs(zdata) > 0.0) {
						ydata /= zdata;
					}
					else {
						ydata = 0.0;
						status.condition = 3;
						status.severity = 4;
					}
					eveDataMessage *normalizedMessage = new eveDataMessage(detectorId, QString(), status, DMTnormalized, eveTime::getCurrent(), QVector<double>(1,ydata));
					// TODO remove
					//mmanager->sendError(DEBUG, 0, QString("normalized value: %1").arg(ydata));
					normalizedMessage->setPositionCount(ypos);
					normalizedMessage->setSmId(smid);
					mmanager->sendMessage(normalizedMessage);
				}
				xdataArray.append(xdata);
				ydataArray.append(ydata);
				modified = true;
				position = xpos;
			}
		}
	}
}

void eveMath::calculate(MathAlgorithm type){

	if (modified){
		modified = false;
		minimum = 1.0e300;
		maximum = 1.0e-300;
		sum = 0.0;
		int index = 0;
		minIndex = index;
		maxIndex = index;
		foreach(double value, ydataArray){
			if (value > maximum) {
				maximum = value;
				maxIndex = index;
			}
			if (value < minimum) {
				minimum = value;
				minIndex = index;
			}
			try {
				sum += value;
			} catch (std::exception& e) {
				printf("C++ Exception in eveMath > sum += value <  %s\n",e.what());
			}
			++index;
		}
		if (ydataArray.size() > 1) {
			int count = ydataArray.count();
			double psum = 0.;
			double mean = sum / count;
			foreach (double val, ydataArray){
				psum += pow((val-mean),2.0);
			}
			std_deviation = sqrt(psum / (double)(count -1));
		}
	}
	xresult = 0.0;
	yresult = 0.0;
	switch (type) {
		case MIN:
			yresult = minimum;
			if (minIndex < xdataArray.size()) xresult = xdataArray.at(minIndex);
			break;
		case MAX:
			yresult = maximum;
			if (maxIndex < xdataArray.size()) xresult = xdataArray.at(maxIndex);
			break;
		case CENTER:
			break;
		case EDGE:
			break;
		case FWHM:
			break;
		case STD_DEVIATION:
			yresult = std_deviation;
			break;
		case MEAN:
			if (ydataArray.size() > 0) yresult = (sum / ((double)ydataArray.size()));
			break;
		case SUM:
			yresult = sum;
			break;
		default:
			break;
	}
}

QList<eveDataMessage*> eveMath::getResultMessage(MathAlgorithm type, int smid){
	QList<eveDataMessage*> messageList;

	QVector<double> data;
	calculate(type);

	data.append(xresult);
	eveDataMessage *message = new eveDataMessage(xAxisId, QString(), eveDataStatus(), toDataMod(type), epicsTime(), data);
	message->setChainId(chid);
	message->setSmId(smid);
	message->setPositionCount(position);
	messageList.append(message);

	data.clear();
	data.append(yresult);
	message = new eveDataMessage(detectorId, QString(), eveDataStatus(), toDataMod(type), epicsTime(), data);
	message->setChainId(chid);
	message->setSmId(smid);
	message->setPositionCount(position);
	messageList.append(message);

	return messageList;
}

eveDataModType eveMath::toDataMod(MathAlgorithm type){
	switch (type) {
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
			break;
	}
	return DMTunmodified;
}
