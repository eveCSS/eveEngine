/*
 * eveCalc.h
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#ifndef EVECALC_H_
#define EVECALC_H_

#include <QVector>
#include <QPointF>
#include "eveTypes.h"
#include "eveVariant.h"
#include "eveMessage.h"
#include "eveMessageChannel.h"
#include "eveMathConfig.h"


enum MathAlgorithm {ALL, UNKNOWN, PEAK, MIN, MAX, CENTER, EDGE, FWHM, STD_DEVIATION, MEAN, SUM};

class eveCalc{
public:
    static QList<MathAlgorithm> getAlgorithms();
    eveCalc(eveMessageChannel*, QString, QString, QString, QString);
    eveCalc(eveMathConfig mathConfig, eveMessageChannel* manag);
    eveCalc(eveMessageChannel* manag, QHash<QString, QString>* mathparams);
    virtual ~eveCalc();
    void reset();
    bool addValue(QString, int pos, eveVariant);
    bool calculate(){return calculate(algorithm);};
    bool calculate(MathAlgorithm);
    QString getAxisId(){return xAxisId;};
    QString getChannelId(){return detectorId;};
    QString getNormalizeId(){return normalizeId;};
    void setNormalizeExternal(bool normExt){normalizeExt = normExt;};
    bool getNormalizeExternal(){return normalizeExt;};
    eveVariant getXResult(){return eveVariant(getResult(algorithm).rx());};
    QPointF getResult(MathAlgorithm);
    static MathAlgorithm toMathAlgorithm(QString);

protected:
    eveDataModType toDataMod(MathAlgorithm);
    QString xAxisId;
    QString detectorId;
    QString normalizeId;
    int ypos;
    double threshold;
    int position;
    double ydata;

private:
    class Calcresult {
    public:
        Calcresult(QPointF, QPointF, QPointF, int, double, double);
        QPointF minimum;
        QPointF maximum;
        QPointF peak;
        int peakIndex;
        double fwhm;
        double center;
    };
    int xpos, zpos, count;
    double zdata;
    double xdata;
    QPointF getEdge(const QVector<QPointF>& curve);
    void acceptPoint(double, double);
    bool calculatePeakCenterFWHM();
    QVector<QPointF> getDerivative(const QVector<QPointF>& curve);
    Calcresult getPeakAndCenter(const QVector<QPointF>& curve);
    QVector<QPointF> points;
    QVector<QPointF> getGradients(QVector<QPointF>);
    void execValues();
    bool setResult(MathAlgorithm);
    bool checkValue(double value);
    bool newValues;
    bool centerOK;
    bool normalizeExt;
    MathAlgorithm algorithm;
    bool doNormalize;
    bool saveValues;
    eveMessageChannel* manager;
    //        int minIndex, maxIndex, peakIndex;
    //        double xresult;
    //        double yresult;
    QPointF curveMin, curveMax, curvePeak, curveCenter, curveEdge;
    QPointF curveStdDev, curveFwhm, curveMean, curveSum;
};

#endif /* EVECALC_H_ */
