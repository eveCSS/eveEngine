/*
 * eveCalc.h
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#ifndef EVECALC_H_
#define EVECALC_H_

#include <QVector>
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
	virtual ~eveCalc();
	void reset();
	bool addValue(QString, int pos, eveVariant);
	bool isModified(){return arrayModified;};
	bool calculate(){return calculate(algorithm);};
	bool calculate(MathAlgorithm);
	QString getAxisId(){return xAxisId;};
	QString getChannelId(){return detectorId;};
	QString getNormalizeId(){return normalizeId;};
	eveVariant getXResult(){return eveVariant(xresult);};
	eveVariant getYResult(){return eveVariant(yresult);};

protected:
	eveDataModType toDataMod(MathAlgorithm);
	QString xAxisId;
	QString detectorId;
	int xpos, ypos, zpos, count;
	int position;
	double ydata;
	double zdata;
	double xresult;
	double yresult;

private:
	void execValues();
	bool setResult(MathAlgorithm);
	bool checkValue(double value);
	MathAlgorithm toMathAlgorithm(QString);
	MathAlgorithm algorithm;
	QString normalizeId;
	bool arrayModified;
	bool doNormalize;
	bool saveValues;
	eveMessageChannel* manager;
	QVector<double> xdataArray;
	QVector<double> ydataArray;
	int minIndex, maxIndex;
	double fwhm;
	double xdata;
	double ymin, xmin, xmax;
	double ymax;
	double sum;
};

#endif /* EVECALC_H_ */
