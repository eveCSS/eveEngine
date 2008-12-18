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
#include "eveTypes.h"
#include "eveMotorPosition.h"

enum eveSTEPFUNCTIONT {eveSTEPFUNCTION_Add, eveSTEPFUNCTION_Multiply, eveSTEPFUNCTION_Double, eveSTEPFUNCTION_File, eveSTEPFUNCTION_Plugin, eveSTEPFUNCTION_Positionlist};

/**
 * \brief calculate the next motor position with stepfunction
 */
class evePosCalc {

public:
	evePosCalc(QString , eveType);
	virtual ~evePosCalc();
	void setStartPos(QString);
	void setEndPos(QString);
	void setStepWidth(QString);
	void setStepPara(QString);
	void setStepFile(QString);
	void setStepPlugin(QString);
	void setPositionList(QString);
	eveMotorPosition* getNextPos();
	eveMotorPosition* getStartPos();
	bool isAtEndPos(){return isAtEnd;};


private:
	eveMotorPosition* funcAdd();

	int posCounter;
	eveSTEPFUNCTIONT stepFunction;
	eveMotorPosition *startPos, *endPos, *currentPos, *stepWidth;
	eveType axisType;
	QString stepPara;
	QString stepFile;
	QStringList positionList;
	QList<int> posIntList;
	QList<double> posDoubleList;
	bool isAtEnd;

};

#endif /* EVEPOSCALC_H_ */
