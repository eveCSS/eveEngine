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
class eveSMAxis;

/**
 * \brief calculate the next motor position with stepfunction
 *
 *
 */
class evePosCalc {

public:
	evePosCalc(eveScanModule*, QString , bool, eveType );
	virtual ~evePosCalc();
	void setStartPos(QString);
	void setEndPos(QString);
	void setStepWidth(QString);
	void setStepFile(QString);
	void setStepPlugin(QString, QHash<QString, QString>& );
	void setPositionList(QString);
	void reset();
	eveVariant& getNextPos();
	eveVariant& getStartPos(){return startPosAbs;};
	eveVariant& getCurrentPos(){return currentPos;};
	eveType getType(){return axisType;};
	bool isAtEndPos(){return (this->*doneFunction)();};
	bool isAbs(){return absolute;};
	bool setOffset(eveVariant);
	bool motionDisabled(){return doNotMove;};
	int getTotalSteps(){return totalSteps;};

private:
	enum {NONE, STARTSTOP, MULTIPLY, FILE, PLUGIN, LIST} stepmode;
	void stepfuncAdd();
	void stepfuncMultiply();
	void stepfuncList();
	void stepfuncDummy();
	void stepfuncReferenceMultiply();
	void stepfuncMotionDisabled();
	bool donefuncAdd();
	bool donefuncMultiply();
	bool donefuncList();
	bool donefuncDummy();
	bool donefuncAlwaysTrue(){return true;};
	void sendError(int, QString);
	void checkValues();
	void setPos(QString, eveVariant*);

	int posCounter;
	int totalSteps;
	void (evePosCalc::*stepFunction)();
	bool (evePosCalc::*doneFunction)();
	eveVariant startPosAbs, endPosAbs, startPos, endPos, currentPos, stepWidth, offSet, nullVal;
	eveType axisType;
	QString stepPlugin;
	QStringList positionList;
	QList<int> posIntList;
	QList<double> posDoubleList;
	eveScanModule* scanModule;
	double multiplyFactor;
	eveSMAxis* RefMultiplyAxis;
	bool absolute;
	bool readyToGo;
	bool doNotMove;
};

#endif /* EVEPOSCALC_H_ */
