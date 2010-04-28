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

typedef struct sUInt8_t {
	quint8 a;
	quint32 b;
} sUInt8_t;
typedef struct sInt8_t {
	qint8 a;
	quint32 b;
} sInt8_t;
typedef struct sUInt16_t {
	quint16 a;
	quint32 b;
} sUInt16_t;
typedef struct sInt16_t {
	qint16 a;
	quint32 b;
} sInt16_t;
typedef struct sUInt32_t {
	quint32 a;
	quint32 b;
} sUInt32_t;
typedef struct sInt32_t {
	quint32 a;
	quint32 b;
} sInt32_t;
typedef struct sUInt64_t {
	quint64 a;
	quint32 b;
} sUInt64_t;
typedef struct sInt64_t {
	qint64 a;
	quint32 b;
} sInt64_t;
typedef struct sFloat_t {
	float a;
	quint32 b;
} sFloat_t;
typedef struct sDouble_t {
	double a;
	quint32 b;
} sDouble_t;
typedef struct sString_t {
	char a[STANDARD_STRINGSIZE +1];
	quint32 b;
} sString_t;
typedef struct sEumString_t {
	char a[STANDARD_ENUM_STRINGSIZE +1];
	quint32 b;
} sEumString_t;

/*
 *
 */
class hdf5Plugin : public QObject, eveFileWriter{

	Q_OBJECT
	Q_INTERFACES(eveFileWriter)

public:
	hdf5Plugin();
	virtual ~hdf5Plugin();
	int init(int, QString, QString, QHash<QString, QString>*);
	int setCols(int, QString, QString, QStringList);
	int open(int);
	int close(int);
	int addData(int, eveDataMessage*);
	int setXMLData(QByteArray*);

	QString errorText();

private:
	static CompType createStandardDataType(QString, QString, eveType);
	static PredType convertToHdf5Type(eveType);
	void* getBufferAddress(eveDataMessage*);
	bool isFileOpen;
	class columnInfo {
		public:
		columnInfo(QString, QStringList);
		bool isNotInit;
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
	QHash<int, QHash<QString, columnInfo* >* > idHash;
    sInt8_t		sInt8;
    sUInt8_t	sUInt8;
    sInt16_t	sInt16;
    sUInt16_t	sUInt16;
    sInt32_t	sInt32;
    sUInt32_t	sUInt32;
    sInt64_t	sInt64;
    sUInt64_t	sUInt64;
    sFloat_t	sFloat;
    sDouble_t	sDouble;
    sString_t	sString;
    sEumString_t sEumString;

};

#endif /* HDF5PLUGIN_H_ */
