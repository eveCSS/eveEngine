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
    prefered = false;
    manager = manag;
    plotWindow = mathConfig.getPlotWindowId();
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
    if (prefered) {
        prefered = false;
        QStringList slist;
        if (normalizeId.size() > 0)  slist << "PreferedNormalization" << normalizeId;
        slist << "PreferedYAxis" << detectorId << "PreferedXAxis" << xAxisId;
        eveMessageTextList* metaDataMessage = new eveMessageTextList(EVEMESSAGETYPE_METADATA, slist);
        metaDataMessage->setDestinationFacility(EVECHANNEL_STORAGE);
        manager->sendMessage(metaDataMessage);
    }
	if (((eveCalc*)this)->addValue(deviceId, pos, dataVar)) {
        eveDataMessage *normalizedMessage = new eveDataMessage(detectorId, QString(),
				eveDataStatus(), DMTnormalized, eveTime::getCurrent(), QVector<double>(1,ydata));
		// TODO remove
		manager->sendError(DEBUG, 0, QString("normalized value: %1").arg(ydata));
        normalizedMessage->setNormalizeId(normalizeId);
		normalizedMessage->setPositionCount(ypos);
		normalizedMessage->setSmId(smid);
        normalizedMessage->setDestinationFacility(EVECHANNEL_NET);
        manager->sendMessage(normalizedMessage);
	}
}

QList<eveDataMessage*> eveMath::getResultMessage(MathAlgorithm algo, int chid, int smid){
    QList<eveDataMessage*> messageList;

    messageList.clear();
    QVector<double> data;
    if (calculate(algo)) {
        eveDataMessage *message;
        QPointF result = getResult(algo);
        data.append(result.rx());
        data.append(result.ry());

        message = new eveDataMessage(detectorId, QString(), eveDataStatus(), toDataMod(algo), epicsTime(), data);
        message->setAuxString(xAxisId);
        if (!normalizeId.isEmpty())message->setNormalizeId(normalizeId);
        message->setChainId(chid);
        message->setSmId(smid);
        message->setPositionCount(plotWindow);
        messageList.append(message);
    }

    return messageList;
}
