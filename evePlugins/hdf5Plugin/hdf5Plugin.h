/*
 * hdf5Plugin.h
 *
 *  Created on: 13.11.2009
 *      Author: eden
 */

#ifndef HDF5PLUGIN_H_
#define HDF5PLUGIN_H_

#include <QObject>
#include "eveFileWriter.h"
#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
     using namespace H5;
#endif

#define STANDARD_STRINGSIZE 40
#define STANDARD_ENUM_STRINGSIZE 16
#define DATETIME_STRINGSIZE 40


typedef struct {
	qint32 positionCount;
	void *aPtr;
} memSpace_t;

/*
 *
 */
class hdf5Plugin : public QObject, eveFileWriter{

	Q_OBJECT
	Q_INTERFACES(eveFileWriter)

public:
	hdf5Plugin();
	virtual ~hdf5Plugin();
	int init(int, QString, QString, QHash<QString, QString>&);
	int setCols(int, QString, QString, QStringList);
	int open(int);
	int close(int);
	int addData(int, eveDataMessage*);
	int addComment(int, QString);
	int setXMLData(QByteArray*);

	QString errorText();

private:
	static CompType createDataType(QString, QString, eveType, int);
	static PredType convertToHdf5Type(eveType);
	void* getDataBufferAddress(eveDataMessage*);
	int getMinimumDataBufferLength(eveDataMessage*, int);
	bool isFileOpen;
	class columnInfo {
		public:
		columnInfo(QString, QStringList);
		~columnInfo();
		bool isNotInit;
		int arraySize;
		memSpace_t *memBuffer;
		size_t memSize;
		QString name;
		QStringList info;
		DataSpace dspace;
		DataSpace memspace;
		DataSet dset;
		hsize_t currentDim[1];
		hsize_t currentOffset[1];
		CompType compoundType;
	};
	QList<int> setIdList;
	QString fileName;
	QString fileFormat;
	QString errorString;
	H5File* dataFile;
	int sizeIncrement;
	QString comment;
	QHash<int, QHash<QString, columnInfo* >* > idHash;

};

#endif /* HDF5PLUGIN_H_ */
