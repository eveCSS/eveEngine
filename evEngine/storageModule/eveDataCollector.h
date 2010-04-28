/*
 * eveDataCollector.h
 *
 *  Created on: 03.09.2009
 *      Author: eden
 */

#ifndef EVEDATACOLLECTOR_H_
#define EVEDATACOLLECTOR_H_

#include "eveMessage.h"
#include "eveFileWriter.h"

class eveStorageManager;

/*
 *
 */
class eveDataCollector {
public:
	eveDataCollector(eveStorageManager*, eveStorageMessage*);
	virtual ~eveDataCollector();
	void addData(eveDataMessage*);
	void addDevice(eveDevInfoMessage *);

private:
	bool fwInitDone;
	bool fwOpenDone;
	int chainId;
	QString fileName;
	QString fileType;
	QString pluginName;
	QString pluginPath;
	QStringList deviceList;
	eveStorageManager* manager;
	eveFileWriter* fileWriter;

};

#endif /* EVEDATACOLLECTOR_H_ */