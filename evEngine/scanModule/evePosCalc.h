/*
 * evePosCalc.h
 *
 *  Created on: 16.12.2008
 *      Author: eden
 */

#ifndef EVEPOSCALC_H_
#define EVEPOSCALC_H_

#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include "eveTypes.h"
#include "eveVariant.h"
//#include "eveScanModule.h"

//class eveScanModule;
class eveScanModule;

/**
 * \brief calculate the next motor position with stepfunction
 */
class evePosCalc {

public:
	evePosCalc(eveScanModule*, QString , eveType );
	virtual ~evePosCalc();
	void setStartPos(QString);
	void setEndPos(QString);
	void setStepWidth(QString);
	void setStepPara(QString, QString);
	void setStepFile(QString);
	void setStepPlugin(QString);
	void setPositionList(QString);
	void reset();
	eveVariant& getNextPos();
	eveVariant& getStartPos(){return startPos;};
	eveVariant& getCurrentPos(){return currentPos;};
	bool isAtEndPos(){return isAtEnd;};

private:
	void stepfuncAdd();
	void stepfuncDummy();
	void sendMessage(int, QString);
	void checkValues();

	int posCounter;
	void (evePosCalc::*stepFunction)();
	eveVariant startPos, endPos, currentPos, stepWidth, nullVal;
	eveType axisType;
	QHash<QString, QString> conParaHash;
	QString stepFile, stepPlugin;
	QStringList positionList;
	QList<int> posIntList;
	QList<double> posDoubleList;
	eveScanModule* scanModule;
	bool isAtEnd;
	bool readyToGo;
};

#endif /* EVEPOSCALC_H_ */
