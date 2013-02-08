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
	QString& getXAxis(){return xAxisId;};
	QString& getDetector(){return detectorId;};
	QString& getNormalizeDetector(){return normalizeId;};
	int getPlotWindowId(){return plotWindowId;};
	bool hasEqualDevices(eveMathConfig);
	bool hasInit(){return init;};
	int getFirstScanModuleId();
	QList<int> getAllScanModuleIds(){return smidlist;};
    void setNormalizeExternal(bool normExt){normalizeExt = normExt;};
    bool getNormalizeExternal(){return normalizeExt;};

protected:
	QString xAxisId;
	QString detectorId;
	QString normalizeId;

private:
	bool init;
	int plotWindowId;
	QList<int> smidlist;
    bool normalizeExt;

};

#endif /* EVEMATHCONFIG_H_ */
