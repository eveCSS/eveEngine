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
    bool addValue(double);
    bool replaceValue(double value, int pos);
    eveDataMessage* getResultMessage();
    bool limitMissed();
    int getAttempts(){return attempt + 1;};
    int getDataCount(){return dataArray.size();};
    double getUsedLimit(){return lowLimit;};
    double getUsedDeviation(){return deviation;};

private:
    bool allDone;
    bool doDeviationTest;
    int averageCount;
    int maxAttempt;
    int attempt;
    double lowLimit;
    double deviation;
    QVector<double> dataArray;
};

#endif /* EVEAVERAGE_H_ */
