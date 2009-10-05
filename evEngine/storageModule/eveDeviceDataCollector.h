/*
 * eveDeviceDataCollector.h
 *
 *  Created on: 03.09.2009
 *      Author: eden
 */

#ifndef EVEDEVICEDATACOLLECTOR_H_
#define EVEDEVICEDATACOLLECTOR_H_

#include "eveMessage.h"

class eveStorageManager;

/*
 *
 */
class eveDeviceDataCollector {
public:
	eveDeviceDataCollector(eveStorageManager*, eveDevInfoMessage*);
	virtual ~eveDeviceDataCollector();
	void addData(eveDataMessage*);

private:
	int counter;
	QString xmlId;
	QString name;
	QStringList *transInfo;
	bool initialized;
	eveStorageManager* manager;

	eveType dataType;
	eveDataModType dataModifier;
	QVector<int> sequenceCounter;
	QVector<int> dataArrayInt;
	QVector<short> dataArrayShort;
	QVector<signed char> dataArrayChar;
	QVector<float> dataArrayFloat;
	QVector<double> dataArrayDouble;
	QStringList dataStrings;

};

#endif /* EVEDEVICEDATACOLLECTOR_H_ */
