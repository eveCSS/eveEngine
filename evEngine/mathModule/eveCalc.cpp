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
    threshold = 0.5;
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
    threshold = 0.5;
    reset();

}

eveCalc::eveCalc(eveMessageChannel* manag, QHash<QString, QString>* mathparams) {
    manager = manag;
    algorithm = toMathAlgorithm(mathparams->value("pluginname", QString()));
    xAxisId = mathparams->value("axis_id", QString());
    detectorId = mathparams->value("channel_id", QString());
    normalizeId = mathparams->value("normalize_id", QString());
    doNormalize = true;
    if (normalizeId.isEmpty()) doNormalize = false;
    QString threshold_str = mathparams->value("threshold", QString());
    saveValues = true;
    normalizeExt = false;
    threshold = 0.5;
    bool ok = false;
    if (!threshold_str.isEmpty()){
        threshold = threshold_str.toDouble(&ok);
        if (ok) threshold *= 0.01;
    }
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
    curveMax.ry() = -1.0*DBL_MAX;
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

    bool retval = true;

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
                // position has already been done
                if (xpos == position) return false;

                ++position;
                if (xpos != position){
                    //manager->sendError(MINOR, MATH, 0, QString("possibly missed calculation for position  %1").arg(position));
                    position = xpos;
                }
                if (doNormalize) {
                    if (fabs(zdata) > 0.0) {
                        ydata /= zdata;
                    }
                    else {
                        retval = false;
                        ydata = 0.0;
                        manager->sendError(MINOR, MATH, 0, QString("unable to normalize with normalize detector value 0.0"));
                    }
                }
                ++count;
                if (retval) retval = acceptPoint(xdata, ydata);
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

bool eveCalc::acceptPoint(double xval, double yval) {
    if ((saveValues)||(count == 0)){
        // we always save the start values
        newValues = true;
        points.append(QPointF(xval, yval));
        manager->sendError(DEBUG, MATH, 0, QString("add point to calc %1 / %2").arg(xval).arg(yval));
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
        return false;
    }
    return true;
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

    if (points.size() < 3) {
        manager->sendError(DEBUG, MATH, 0, QString("unable to do calculations with less then 3 data points"));
        return false;
    }
    if (curveMin.ry() == curveMax.ry()) {
        manager->sendError(DEBUG, MATH, 0, QString("unable to do calculations Y_min = Y_max"));
        return false;
    }


    newValues = false;
    Calcresult mmresult = getPeakAndCenter(points);
    if (mmresult.peakIndex != 0) {
        curvePeak = mmresult.peak;
        curveFwhm.setY(mmresult.fwhm);
        curveCenter.setX(mmresult.center);
        return true;
    }
    else {
        manager->sendError(DEBUG, MATH, 0, QString("unable to do calculate PeakCenterFWHM peak: %1, Center %2, Fwhm %3")
                           .arg(mmresult.peak.ry()).arg(mmresult.center).arg(mmresult.fwhm));
    }

    return false;
}

QPointF eveCalc::getEdge(const QVector<QPointF>& curve){

    if (points.size() > 2) {
        QVector<QPointF> derivative = getDerivative(curve);
        if (derivative.size() > 2) {
            Calcresult center = getPeakAndCenter(derivative);
            if (center.peakIndex != 0) {
                return QPointF(center.center, 0.);
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
eveCalc::Calcresult eveCalc::getPeakAndCenter(const QVector<QPointF>& curve){

    QPointF max(0.0, -1.0*DBL_MAX);
    QPointF min(0.0, DBL_MAX);
    QPointF Peak(0.0, 0.0);
    double center = 0.0;
    double fwhm = 0.0;
    int peakIndex = 0;

    if (curve.size() > 2){

        QPointF first = curve.first();
        QPointF last = curve.last();
        int index = 0, minindex = 0, maxindex = 0;
        double threshMaxY;
        double threshDistY;
        bool haveMaxCenter = false, haveMinCenter = false;
        int rightMaxIndex = 0, leftMaxIndex = 0;
        int rightMinIndex = 0, leftMinIndex = 0;

        foreach (QPointF point, curve){
            if (point.ry() < min.ry()) {
                min = point;
                minindex = index;
            }
            if (point.ry() > max.y()) {
                max = point;
                maxindex = index;
            }
            ++index;
        }

        threshMaxY = (max.ry() + min.ry())*threshold;
        threshDistY = fabs((max.ry() - min.ry())*(1-threshold));

        // check if curve goes down to 50% left and right from max
        if ((max.ry() > first.ry()) && (max.ry() > last.ry())){
            for (int i = maxindex; i < curve.size(); ++i){
                if (fabs(max.ry() - curve.at(i).y()) > threshDistY) {
                    rightMaxIndex = i;
                    break;
                }
            }
            for (int i = maxindex; i >= 0; --i){
                if (fabs(max.ry() - curve.at(i).y()) > threshDistY){
                    leftMaxIndex = i;
                    break;
                }
            }
        }
        // check if curve goes up to 50% left and right from min
        if ((min.ry() < first.ry()) && (min.ry() < last.ry())){
            for (int i = minindex; i < curve.size(); ++i){
                if (fabs(min.ry() - curve.at(i).y()) > threshDistY) {
                    rightMinIndex = i;
                    break;
                }
            }
            for (int i = minindex; i >= 0; --i){
                if (fabs(min.ry() - curve.at(i).y()) > threshDistY){
                    leftMinIndex = i;
                    break;
                }
            }
        }
        // verify
        if ((rightMaxIndex != maxindex) && (leftMaxIndex != maxindex)
                && (rightMaxIndex != 0) && (leftMaxIndex != 0)) haveMaxCenter = true;
        if ((rightMinIndex != minindex) && (leftMinIndex != minindex)
                && (rightMinIndex != 0) && (leftMinIndex != 0)) haveMinCenter = true;

        int rightIndex = 0;
        int leftIndex = 0;

        // if we have two peaks use the max peak
        // TODO find a better way to select a peak
        if (haveMaxCenter && haveMinCenter) haveMinCenter = false;

        // use max or min
        if (haveMaxCenter) {
            Peak = max;
            peakIndex = maxindex;
            rightIndex = rightMaxIndex;
            leftIndex = leftMaxIndex;
        }
        if (haveMinCenter) {
            Peak = min;
            peakIndex = minindex;
            rightIndex = rightMinIndex;
            leftIndex = leftMinIndex;
        }

        if (haveMaxCenter || haveMinCenter) {
            // linear fit to find halfMaxX
            double halfMaxXright = (threshMaxY - curve.at(rightIndex).y()) * (curve.at(rightIndex-1).x() - curve.at(rightIndex).x()) /
                    (curve.at(rightIndex-1).y() - curve.at(rightIndex).y()) + curve.at(rightIndex).x();
            double halfMaxXleft = (threshMaxY - curve.at(leftIndex).y()) * (curve.at(leftIndex+1).x() - curve.at(leftIndex).x()) /
                    (curve.at(leftIndex+1).y() - curve.at(leftIndex).y()) + curve.at(leftIndex).x();

            center = (halfMaxXright + halfMaxXleft) * 0.5;
            fwhm = fabs(halfMaxXright - halfMaxXleft);
        }
    }
    return Calcresult(max, min, Peak, peakIndex, fwhm, center);
}

eveCalc::Calcresult::Calcresult(QPointF max, QPointF min, QPointF cpeak, int pindex, double fullwidthhm, double cent){
    maximum = max;
    minimum = min;
    peak = cpeak;
    peakIndex = pindex;
    fwhm = fullwidthhm;
    center = cent;
}
