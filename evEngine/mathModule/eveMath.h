/*
 * eveMath.h
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#ifndef EVEMATH_H_
#define EVEMATH_H_

#include <QVector>
#include "eveVariant.h"
#include "eveMessage.h"
#include "eveMathConfig.h"

enum MathAlgorithm {MIN, MAX, CENTER, EDGE, FWHM, STD_DEVIATION, MEAN, SUM};

class eveMath  : public eveMathConfig {
public:
	static QList<MathAlgorithm> getAlgorithms();
	eveMath(int);
	eveMath(eveMathConfig mathConfig);
	eveMath(int, int, double, double);
	virtual ~eveMath();
	void reset();
	void addValue(QString, int pos, eveVariant);
	QList<eveDataMessage*> getResultMessage(MathAlgorithm, int);
	void setAlgorithm(MathAlgorithm alg){if (!usedAlgorithm.contains(alg))usedAlgorithm.append(alg);};
	bool haveAlgorithm(MathAlgorithm alg){return usedAlgorithm.contains(alg);};
	bool isModified(){return modified;};


private:
	void calculate(MathAlgorithm );
	bool checkValue(double value);
	eveDataModType toDataMod(MathAlgorithm);
	MathAlgorithm algorithm;
	bool modified;
	bool doNormalize;
	QList<MathAlgorithm> usedAlgorithm;
	QVector<double> xdataArray;
	QVector<double> ydataArray;
	int xpos, ypos, zpos;
	double position;
	double xdata;
	double ydata;
	double zdata;
	double xresult;
	double yresult;
	double minimum;
	double maximum;
	double sum;
	double std_deviation;
	int maxIndex;
	int minIndex;
};

#endif /* EVEMATH_H_ */
