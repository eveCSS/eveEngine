/*
 * eveDataCollector.h
 *
 *  Created on: 03.09.2009
 *      Author: eden
 */

#ifndef EVEDATACOLLECTOR_H_
#define EVEDATACOLLECTOR_H_

#include <QList>
#include "eveMessage.h"
#include "eveFileWriter.h"

class eveStorageManager;

/*
 *
 */
class eveDataCollector {
public:
	eveDataCollector(eveStorageManager*, QHash<QString, QString>&, QByteArray*);
	virtual ~eveDataCollector();
	void addData(eveDataMessage*);
	void addChain(eveStorageMessage*);
	void addDevice(eveDevInfoMessage *);
	void addMetaData(int, QString, QString&);
        void setKeepFile(bool keep){keepFile = keep;};
        bool isConfirmSave(){return doConfirmSave;};

private:
	QString macroExpand(QString);
	bool fwInitDone;
	bool fwOpenDone;
	bool StartTimeDone;
	bool StartDateDone;
        bool doConfirmSave;
        bool keepFile;
	QList<int> chainIdList;
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
