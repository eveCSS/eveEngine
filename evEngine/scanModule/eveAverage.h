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
	eveVariant getResult();
	bool isDone();
	int getCount(){return dataArray.count();};
	void addValue(eveVariant);
	eveDataMessage* getResultMessage();

private:
	bool checkValue(double value);
	double last_value;
	bool lastTimeLevelReached;
	bool calcDone;
	int averageCount;
	int maxAttempt;
	int attempt;
	double lowLimit;
	double deviation;
	QVector<double> dataArray;
	double sum;
};

#endif /* EVEAVERAGE_H_ */
