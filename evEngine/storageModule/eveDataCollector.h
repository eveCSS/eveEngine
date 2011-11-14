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
	eveDataCollector(eveStorageManager*, eveStorageMessage*, QByteArray*);
	virtual ~eveDataCollector();
	void addData(eveDataMessage*);
	void addDevice(eveDevInfoMessage *);
	void addMetaData(QString, QString&);

private:
	QString macroExpand(QString);
	bool fwInitDone;
	bool fwOpenDone;
	int chainId;
	QString fileName;
	QString comment;
	QString fileType;
	QString pluginName;
	QString pluginPath;
	QStringList deviceList;
	eveStorageManager* manager;
	eveFileWriter* fileWriter;
	QByteArray* xmlData;
};

#endif /* EVEDATACOLLECTOR_H_ */
