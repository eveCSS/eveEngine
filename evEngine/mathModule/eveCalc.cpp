/*
 * eveCalc.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveCalc.h"
#include <cmath>
#include <exception>
#include <values.h>
#include <float.h>
#include "eveError.h"

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
    normalizeExt = false;
        // don't save all values for MIN, MAX, SUM
        if ((algorithm == MIN) || (algorithm == MAX) || (algorithm == SUM)) saveValues = false;
	reset();

}

eveCalc::eveCalc(eveMathConfig mathConfig, eveMessageChannel* manag) {
	manager = manag;
	xAxisId = QString(mathConfig.getXAxis());
	detectorId = QString(mathConfig.getDetector());
	normalizeId = QString(mathConfig.getNormalizeDetector());
	algorithm = UNKNOWN;
    normalizeExt = mathConfig.getNormalizeExternal();
    doNormalize = false;
    if (!normalizeId.isEmpty() && !normalizeExt) doNormalize = true;
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
    points.clear();
    zdata = NAN;
    xdata = NAN;
    newValues = true;
    xpos = -1;
    ypos = -1;
    zpos = -1;
    position = -1;
    count = -1;
    curveMin.ry() = DBL_MAX;
    curveMax.ry() = DBL_MIN;
    ydata = NAN;
    curvePeak = curveCenter = curveEdge = QPointF();
    curveStdDev = QPointF();
    curveFwhm = QPointF();
    curveMean = QPointF();
    curveSum = QPointF();

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

    if (deviceId == xAxisId) {
        // set position at first invocation
        if (position == -1) position = pos -1;
    }
    // do calculations only if all three values may be converted to double
    // motor or detector as DateTime => no calculation
    if (dataVar.canConvert(QVariant::Double)){
        bool ok = false;
        double data = dataVar.toDouble(&ok);
        if (ok){
            if (deviceId == xAxisId) {
                xdata = data;
                xpos = pos;
            }
            else if (deviceId == detectorId) {
                ydata = data;
                ypos = pos;
            }
            else if (doNormalize && (deviceId == normalizeId)) {
                zdata = data;
                zpos = pos;
            }
            else if (normalizeExt && (deviceId == normalizeId)) {
                ydata = data;
                ypos = pos;
            }
            else
                return retval;

            if (((xpos == ypos ) && doNormalize && (xpos == zpos)) || ((xpos == ypos ) && !doNormalize )) {
                if (xpos == position){
                    manager->sendError(MINOR, MATH, 0, QString("position  %1 has already been done").arg(position));
                    return retval;
                }

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
                acceptPoint(xdata, ydata);
            }
        }
        else 	if (dataVar.canConvert(QVariant::DateTime)){
            manager->sendError(DEBUG, MATH, 0, QString("got DateTime as data value, skipping calculations"));
        }
        else {
            manager->sendError(MINOR, MATH, 0, QString("unable to convert dataformat to double, skipping calculations"));
        }
    }
    return retval;
}

/**
 * \brief used only if dataArrays are not used
 */
bool eveCalc::setResult(MathAlgorithm algo){


    switch (algo) {
    case MIN:
    case MAX:
    case SUM:
        if (points.size() > 0) return true;
        break;
    default:
        break;
    }
    return false;
}

bool eveCalc::calculate(MathAlgorithm algo){

    bool retval = false;
    
    if (!saveValues) return setResult(algo);
    
    if (!getAlgorithms().contains(algo)) return false;

    switch (algo) {
    case PEAK:
    case CENTER:
    case FWHM:
        if (newValues) centerOK = calculatePeakCenterFWHM();
        if (centerOK) retval = true;
        break;
    case MIN:
    case MAX:
    case SUM:
        if (points.size() > 0) retval = true;
        break;
    case EDGE:
        curveEdge = getEdge(points);
        retval = true;
        // check for nan, might not work with every compiler
        if (curveEdge.rx() != curveEdge.rx()) retval = false;
        break;
    case STD_DEVIATION:
        if (points.size() > 1) {
            double cnt = (double) points.size();
            double psum = 0.;
            double mean = curveSum.ry() / cnt;
            foreach (QPointF val, points){
                psum += pow((val.ry() - mean),2.0);
            }
            curveStdDev.setX(NAN);
            curveStdDev.setY(sqrt(psum / (cnt -1.0)));
            retval = true;
        }
        break;
    case MEAN:
        if (points.size() > 0) {
            curveMean.setX(NAN);
            curveMean.setY(curveSum.ry() / ((double)points.size()));
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

void eveCalc::acceptPoint(double xval, double yval) {
    if ((saveValues)||(count == 0)){
        // we always save the start values
        newValues = true;
        points.append(QPointF(xval, yval));
    }
    if (yval < curveMin.ry()) {
        curveMin.setY(yval);
        curveMin.setX(xval);
    }
    if (yval > curveMax.ry()) {
        curveMax.setY(yval);
        curveMax.setX(xval);
    }
    try {
        curveSum += QPointF(0.0, yval);
    } catch (std::exception& e) {
        eveError::log(ERROR, QString("C++ Exception in eveCalc > sum += value <  %1").arg(e.what()), MATH);
    }
}

QPointF eveCalc::getResult(MathAlgorithm algo){
    switch (algo) {
    case PEAK:
        return curvePeak;
        break;
    case MIN:
        return curveMin;
        break;
    case MAX:
        return curveMax;
        break;
    case CENTER:
        return curveCenter;
        break;
    case EDGE:
        return curveEdge;
        break;
    case FWHM:
        return curveFwhm;
        break;
    case STD_DEVIATION:
        return curveStdDev;
        break;
    case MEAN:
        return curveMean;
        break;
    case SUM:
        return curveSum;
        break;
    default:
        return QPointF();
        break;
    }
}

bool eveCalc::calculatePeakCenterFWHM(){

    if ((points.size() < 3) || (curveMin.ry() == curveMax.ry())) return false;

    newValues = false;
    Calcresult mmresult = getPeak(points);
    if (mmresult.peakIndex != 0) {
        mmresult = getCenter(mmresult, points);
        if (mmresult.peakIndex != 0) {
            curvePeak = mmresult.peak;
            curveFwhm.setY(mmresult.fwhm);
            curveCenter.setX(mmresult.center);
            return true;
        }
    }
    return false;
}

QPointF eveCalc::getEdge(const QVector<QPointF>& curve){

    if (points.size() > 2) {
        QVector<QPointF> derivative = getDerivative(curve);
        if (derivative.size() > 2) {
            Calcresult mmresult = getPeak(derivative);
            if (mmresult.peakIndex != 0) {
                Calcresult center = getCenter(mmresult, derivative);
                if (center.peakIndex != 0) {
                    return QPointF(center.center, 0.);
                }
            }
        }
    }
    return QPointF(NAN, NAN);
}

// numerical differentiation using 3-point, Lagrangian interpolation
QVector<QPointF> eveCalc::getDerivative(const QVector<QPointF>& curve){

    if (curve.size() < 3) return QVector<QPointF>();

    //    x12 = x - shift(x,-1)   ;x1 - x2
    //    x01 = shift(x,1) - x    ;x0 - x1
    //    x02 = shift(x,1) - shift(x,-1) ;x0 - x2
    //
    //    d = shift(y,1) * (x12 / (x01*x02)) + $ ;Middle points
    //    y * (1./x12 - 1./x01) - $
    //    shift(y,-1) * (x01 / (x02 * x12))
    //
    //    // Formulae for the first and last points:
    //    d[0] = y[0] * (x01[1]+x02[1])/(x01[1]*x02[1]) - $ ;First point
    //    y[1] * x02[1]/(x01[1]*x12[1]) + $
    //    y[2] * x01[1]/(x02[1]*x12[1])
    //    n2 = n-2
    //
    //    d[n-1] = -y[n-3] * x12[n2]/(x01[n2]*x02[n2]) + $ ;Last point
    //    y[n-2] * x02[n2]/(x01[n2]*x12[n2]) - $
    //    y[n-1] * (x02[n2]+x12[n2]) / (x02[n2]*x12[n2])


    int n = curve.size();
    QVector<QPointF> derivative(n);
    double result, x12, x01, x02;

    x12 = curve.at(1).x() - curve.at(2).x();
    x01 = curve.at(0).x() - curve.at(1).x();
    x02 = curve.at(0).x() - curve.at(2).x();
    result = curve.at(0).y() * (x01 + x02) / (x01 * x02) -
            curve.at(1).y() * x02/(x01*x12) + curve.at(2).y() * x01/(x02*x12);
    derivative[0] = QPointF(curve.at(0).x(), result);

    for(int i=1; i < n-1; ++i){
        x12 = curve.at(i).x() - curve.at(i+1).x();
        x01 = curve.at(i-1).x() - curve.at(i).x();
        x02 = curve.at(i-1).x() - curve.at(i+1).x();
        result = curve.at(i-1).y() * x12/(x01*x02) + curve.at(i).y() * (1.0/x12 - 1.0/x01) -
                 curve.at(i+1).y() * (x01 / (x02 * x12));
        derivative[i] = QPointF(curve.at(i).x(), result);
    }

    x12 = curve.at(n-2).x() - curve.at(n-1).x();
    x01 = curve.at(n-3).x() - curve.at(n-2).x();
    x02 = curve.at(n-3).x() - curve.at(n-1).x();
    result = -curve.at(n-3).y() * x12 / (x01*x02) +
         curve.at(n-2).y() * x02/(x01*x12) - curve.at(n-1).y() * (x02+x12) / (x02*x12);
    derivative[n-1] = QPointF(curve.at(n-1).x(), result);

    return derivative;
}
/**
 * @param curve
 * @return
 */
eveCalc::Calcresult eveCalc::getPeak(const QVector<QPointF>& curve){

    QPointF max(0.0, DBL_MIN);
    QPointF min(0.0, DBL_MAX);
    QPointF Peak;
    int pindex = 0;

    if (curve.size() > 2){

        QPointF first = curve.first();
        QPointF last = curve.last();
        int index = 0, minindex = 0, maxindex = 0;
        double sum = 0.0, mean;
        double maxpeak = 0.0, minpeak = 0.0;

        foreach (QPointF point, curve){
            if (point.ry() < min.ry()) {
                min = point;
                minindex = index;
            }
            if (point.ry() > max.y()) {
                max = point;
                maxindex = index;
            }
            sum += point.ry();
            ++index;
        }
        mean = sum / ((double) index);

        if ((max.ry() > first.ry()) && (max.ry() > last.ry())){
            maxpeak = fabs(max.ry() - mean);
        }
        if ((min.y() < first.ry()) && (min.y() < last.ry())){
            minpeak = fabs(min.ry() - mean);
        }
        if (minpeak > maxpeak){
            Peak = min;
            pindex = minindex;
        }
        else if (minpeak < maxpeak) {
            Peak = max;
            pindex=maxindex;
        }
    }
    return Calcresult(max, min, Peak, pindex, 0., 0.);
}

/**
 * @param curve
 * @return
 */
eveCalc::Calcresult eveCalc::getCenter(eveCalc::Calcresult minmax, const QVector<QPointF>& curve){

    // CENTER:
    double halfMaxY = (minmax.maximum.ry()+minmax.minimum.ry())/2;
    double halfDistY = fabs((minmax.maximum.ry()-minmax.minimum.ry())/2.);
    int peakIndex = minmax.peakIndex;
    int rightIndex = 0, leftIndex = 0;
    QPointF Peak = minmax.peak;
    Calcresult result = minmax;
    result.center = 0.0;
    result.fwhm = 0.0;
    result.peakIndex = 0;

    if (curve.size() > 2){

        for (int i = peakIndex; i < curve.size(); ++i){
            if (fabs(Peak.ry() - curve.at(i).y()) > halfDistY) {
                rightIndex = i;
                break;
            }
        }
        for (int i = peakIndex; i >= 0; --i){
            if (fabs(Peak.ry() - curve.at(i).y()) > halfDistY){
                leftIndex = i;
                break;
            }
        }
        // verify
        if ((rightIndex != peakIndex) && (leftIndex != peakIndex)
                && (rightIndex != 0) && (leftIndex != 0)) {

            // linear fit to find halfMaxX
            double halfMaxXright = (halfMaxY - curve.at(rightIndex).y()) * (curve.at(rightIndex-1).x() - curve.at(rightIndex).x()) /
                    (curve.at(rightIndex-1).y() - curve.at(rightIndex).y()) + curve.at(rightIndex).x();
            double halfMaxXleft = (halfMaxY - curve.at(leftIndex).y()) * (curve.at(leftIndex+1).x() - curve.at(leftIndex).x()) /
                    (curve.at(leftIndex+1).y() - curve.at(leftIndex).y()) + curve.at(leftIndex).x();

            result.center = (halfMaxXright + halfMaxXleft) * 0.5;
            result.fwhm = fabs(halfMaxXright - halfMaxXleft);
            result.peakIndex = peakIndex;
        }
    }
    return result;
}

eveCalc::Calcresult::Calcresult(QPointF max, QPointF min, QPointF cpeak, int pindex, double fullwidthhm, double cent){
    maximum = max;
    minimum = min;
    peak = cpeak;
    peakIndex = pindex;
    fwhm = fullwidthhm;
    center = cent;
}
