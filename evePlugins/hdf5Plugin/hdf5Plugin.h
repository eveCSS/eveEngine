/*
 * hdf5Plugin.h
 *
 *  Created on: 13.11.2009
 *      Author: eden
 */

#ifndef HDF5PLUGIN_H_
#define HDF5PLUGIN_H_

#include <QObject>
#include <QMultiHash>
#include <QStringList>
#include <QString>
#include "eveMessage.h"
#include "eveFileWriter.h"
#include "eveTypes.h"
#include "H5Cpp.h"
#include "hdf5DataSet.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif
using namespace std;

#define SAVEPLUGIN_VERSION "1.3"
#define EVEH5_VERSION "2.0"

class hdf5Plugin : public QObject, eveFileWriter{

	Q_OBJECT
	Q_INTERFACES(eveFileWriter)

public:
	hdf5Plugin();
	virtual ~hdf5Plugin();
	int init(QString, QString, QHash<QString, QString>&);
	QString getVersionString(){return SAVEPLUGIN_VERSION;};
	int addColumn(eveDevInfoMessage* message);
	int open();
	int close();
    int flush();
	int addData(int, eveDataMessage*);
	int addMetaData(int, QString, QString);
	int setXMLData(QByteArray*);
	QString errorText(){return errorString;};

private:
	static void compareNames(H5Object&, std::string, void*);
	int addSingleData(int, eveDataMessage*);
	int addArrayData(int, eveDataMessage*);
    QString getDSName(int, QString, QString, eveDataModType, QString, QString);
	QString createGroup(QString);
	QString getGroupName(int pathId);
	bool isFileOpen;
	QStringList groupList;
	QString fileName;
	QString fileFormat;
	QString errorString;
	H5File* dataFile;
	int defaultSizeIncrement;
	QHash<QString, hdf5DataSet* > dsNameHash;
	QHash<eveDataModType, QString>modificationHash;
};

#endif /* HDF5PLUGIN_H_ */
