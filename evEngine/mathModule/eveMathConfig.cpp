/*
 * eveMathConfig.cpp
 *
 *  Created on: 20.05.2010
 *      Author: eden
 */

#include "eveMathConfig.h"

eveMathConfig::eveMathConfig(int chid, int plotWindowId, bool init, QString xAxisId) {
	this->chid = chid;
	this->plotWindowId = plotWindowId;
	this->xAxisId = xAxisId;
	this->init = init;
}

eveMathConfig::~eveMathConfig() {
	// TODO Auto-generated destructor stub
}

void eveMathConfig::addYAxis(QString detectorId, QString normalizeId) {
	this->detectorId = detectorId;
	this->normalizeId = normalizeId;
}

void eveMathConfig::addScanModule(int smid) {
	if (!smidlist.contains(smid)) smidlist.append(smid);
}

bool eveMathConfig::haveNormalize() {
	if (normalizeId.length() > 0)
		return true;
	else
		return false;
}

int eveMathConfig::getFirstScanModuleId() {
	if (!smidlist.isEmpty())
		return smidlist.first();
	else
		return 0;
}

QList<int> eveMathConfig::getAllScanModuleIds() {
	return smidlist;
}

bool eveMathConfig::hasEqualDevices(eveMathConfig other){
	if ((xAxisId == other.getXAxis()) && (detectorId == other.getDetector()) && (normalizeId == other.getNormalizeDetector()))
		return true;

	return false;
}
