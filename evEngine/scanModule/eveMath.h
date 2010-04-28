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

#define EVEMATH_MIN       0x01
#define EVEMATH_MAX       0x02
#define EVEMATH_CENTER    0x04
#define EVEMATH_EDGE      0x08
#define EVEMATH_FWHM      0x10
#define EVEMATH_DEVIATION 0x20
#define EVEMATH_AVERAGE   0x40
#define EVEMATH_SUM       0x80
// Caution adjust this if adding algorithms
// EVEMATH_ALL = MIN & MAX & ... & AVERAGE
#define EVEMATH_ALL       0xff

// not part of EVEMATH_ALL
#define EVEMATH_AVERAGEWINDOW 0x0800


class eveMath {
public:
	eveMath(int);
	eveMath(int, int, double, double);
	virtual ~eveMath();
	void reset(int);
	void reset();
	eveVariant getResult(int );
	bool isDone();
	int getCount(){return dataArray.count();};
	void addValue(eveVariant);
	eveDataMessage* getResultMessage(int);
	eveDataMessage* getResultMessage(){return getResultMessage(EVEMATH_AVERAGEWINDOW);};
	bool haveAlgorithm(int type){return (type & mathType);};



private:
	bool checkValue(double value);
	eveDataModType toDataMod(int type);
	double last_value;
	bool lastTimeLevelReached;
	bool calcDone;
	int mathType;
	int averageCount;
	int maxAttempt;
	int attempt;
	double lowLimit;
	double deviation;
	QVector<double> dataArray;
	double minimum;
	double maximum;
	double sum;
};

#endif /* EVEMATH_H_ */
