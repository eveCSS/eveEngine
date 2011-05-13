/*
 * eveMath.cpp
 *
 *  Created on: 14.04.2010
 *      Author: eden
 */

#include "eveMath.h"
#include "eveMathManager.h"
#include <cmath>
#include <exception>

/**
 * eveMath does calculations for one detector/normalize-detector/motor triple
 *
 * @param mathConfig configuration data for math
 * @return
 */
eveMath::eveMath(eveMathConfig mathConfig, eveMathManager* manag) : eveCalc(mathConfig, manag){
	smidlist = QList<int>(mathConfig.getAllScanModuleIds());
	initBeforeStart = mathConfig.hasInit();
	manager = manag;
}

eveMath::~eveMath() {
	// TODO Auto-generated destructor stub
}


/**
 * for now all values are converted to doubles
 *
 * @param dataVar add value to list of values for calculations
 */
void eveMath::addValue(QString deviceId, int smid, int pos, eveVariant dataVar){
	if (((eveCalc*)this)->addValue(deviceId, pos, dataVar)) {
		eveDataStatus status = {1,1,1};
		eveDataMessage *normalizedMessage = new eveDataMessage(detectorId, QString(),
				status, DMTnormalized, eveTime::getCurrent(), QVector<double>(1,ydata));
		// TODO remove
		manager->sendError(DEBUG, 0, QString("normalized value: %1").arg(ydata));
		normalizedMessage->setPositionCount(ypos);
		normalizedMessage->setSmId(smid);
		manager->sendMessage(normalizedMessage);

	}

}

QList<eveDataMessage*> eveMath::getResultMessage(MathAlgorithm algo, int chid, int smid){
	QList<eveDataMessage*> messageList;

	QVector<double> data;
	if (calculate(algo)) {
		eveDataMessage *message;
		data.append(xresult);
		data.append(yresult);
		message = new eveDataMessage(detectorId, QString(), eveDataStatus(), toDataMod(algo), epicsTime(), data);
		message->setChainId(chid);
		message->setSmId(smid);
		message->setPositionCount(position);
		messageList.append(message);
	}

	return messageList;
}
