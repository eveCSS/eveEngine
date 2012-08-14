/*
 * eveMath.h
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#ifndef EVEMATH_H_
#define EVEMATH_H_

#include "eveVariant.h"
#include "eveMessage.h"
#include "eveMathConfig.h"
#include "eveMathManager.h"
#include "eveCalc.h"

class eveMathManager;

class eveMath  : public eveCalc {
public:
	eveMath(eveMathConfig mathConfig, eveMathManager *);
	virtual ~eveMath();
	void addValue(QString, int smid, int pos, eveVariant);
	QList<eveDataMessage*> getResultMessage(MathAlgorithm, int, int, int);
	QList<int> getAllScanModuleIds(){return smidlist;};
	bool hasInit(){return initBeforeStart;};

private:
	QList<int> smidlist;
	bool initBeforeStart;
	eveMathManager* manager;
}
;

#endif /* EVEMATH_H_ */
