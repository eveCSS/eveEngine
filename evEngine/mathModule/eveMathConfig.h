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
	eveMathConfig(int, bool, QString);
	virtual ~eveMathConfig();
	void addYAxis(QString, QString);
	void addScanModule(int);
	// bool haveNormalize();
	QString& getXAxis(){return xAxisId;};
	QString& getDetector(){return detectorId;};
	QString& getNormalizeDetector(){return normalizeId;};
	//bool containsSmId(int smid){return smidlist.contains(smid);};
	//int getChainId(){return chId;};
	int getPlotWindowId(){return plotWindowId;};
	bool hasEqualDevices(eveMathConfig);
	bool hasInit(){return init;};
	int getFirstScanModuleId();
	QList<int> getAllScanModuleIds(){return smidlist;};

protected:
	QString xAxisId;
	QString detectorId;
	QString normalizeId;

private:
	//int chId;
	bool init;
	int plotWindowId;
	QList<int> smidlist;


};

#endif /* EVEMATHCONFIG_H_ */
