/*
 * eveAverage.h
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#ifndef EVEAVERAGE_H_
#define EVEAVERAGE_H_

#include <QVector>
#include "eveVariant.h"
#include "eveMessage.h"

class eveAverage {
public:
	eveAverage(int, int, double, double);
	virtual ~eveAverage();
	void reset();
	bool isDone(){return allDone;};
	void addValue(eveVariant);
	eveDataMessage* getResultMessage();

private:
	eveVariant getResult();
	void checkDeviation(double value);
	bool checkLimit(double value);
	double last_value;
	bool lastTimeLevelReached;
	bool calcDone;
	bool allDone;
	bool useLimit;
	bool useDeviation;
	int deviationCount;
	int averageCount;
	int maxAttempt;
	int attempt;
	double lowLimit;
	double deviation;
	QVector<double> dataArray;
	double sum;
};

#endif /* EVEAVERAGE_H_ */
