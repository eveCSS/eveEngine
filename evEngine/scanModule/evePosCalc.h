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
	void setStepPlugin(QString, QHash<QString, QString> );
	void setPositionList(QString);
	void reset();
	eveVariant& getNextPos();
	eveVariant& getStartPos();
	eveVariant& getCurrentPos(){return currentPos;};
	bool isAtEndPos(){return isAtEnd;};
	bool setOffset(eveVariant);

private:
	enum {NONE, STARTSTOP, FILE, PLUGIN, LIST} stepmode;
	void stepfuncAdd();
	void stepfuncList();
	void ReferenceMultiply();
	void stepfuncDummy();
	void sendError(int, QString);
	void checkValues();
	void setPos(QString, eveVariant*);

	int posCounter;
	void (evePosCalc::*stepFunction)();
	eveVariant startPosAbs, endPosAbs, startPos, endPos, currentPos, stepWidth, offSet, nullVal;
	eveType axisType;
	QHash<QString, QString> paraHash;
	QString stepPlugin;
	QStringList positionList;
	QList<int> posIntList;
	QList<double> posDoubleList;
	eveScanModule* scanModule;
	double RefMultiplyFactor;
	eveSMAxis* RefMultiplyAxis;
	bool absolute;
	bool isAtEnd;
	bool readyToGo;
};

#endif /* EVEPOSCALC_H_ */
