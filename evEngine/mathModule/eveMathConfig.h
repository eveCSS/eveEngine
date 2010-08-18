/*
 * eveMathConfig.h
 *
 *  Created on: 20.05.2010
 *      Author: eden
 */

#ifndef EVEMATHCONFIG_H_
#define EVEMATHCONFIG_H_

#include <QString>
#include <QList>

/*
 *
 */
class eveMathConfig {
public:
	eveMathConfig(int, int, bool, QString);
	virtual ~eveMathConfig();
	void addYAxis(QString, QString);
	void addScanModule(int);
	bool haveNormalize();
	QString& getXAxis(){return xAxisId;};
	QString& getDetector(){return detectorId;};
	QString& getNormalizeDetector(){return normalizeId;};
	bool containsSmId(int smid){return smidlist.contains(smid);};
	int getChainId(){return chid;};
	int getPlotWindowId(){return plotWindowId;};
	bool hasEqualDevices(eveMathConfig);
	bool hasInit(){return init;};
	int getFirstScanModuleId();
	QList<int> getAllScanModuleIds();

protected:
	int plotWindowId;
	QString xAxisId;
	QString detectorId;
	QString normalizeId;
	QList<int> smidlist;
	int chid;
	bool init;
};

#endif /* EVEMATHCONFIG_H_ */
