/*
 * hdf5DataSet.h
 *
 *  Created on: 07.05.2012
 *      Author: eden
 */

#ifndef HDF5DATASET_H_
#define HDF5DATASET_H_

#include <QString>
#include "eveTypes.h"
#include "eveMessage.h"
#include "H5Cpp.h"

using namespace H5;
using namespace std;

#define STANDARD_STRINGSIZE 40
#define STANDARD_ENUM_STRINGSIZE 16
#define DATETIME_STRINGSIZE 40

typedef struct {
	qint32 positionCount;
	void *aPtr;
} memSpace_t;

class hdf5DataSet {
public:
	hdf5DataSet(QString, QString, QString, QStringList, H5File*);
	virtual ~hdf5DataSet();
	void close();
	int addData(eveDataMessage*);
	void setSizeIncrement(int inc){sizeIncrement=inc;};
	QString getError(){return errorString;};

private:
	static CompType createDataType(QString, QString, eveType, int);
	static PredType convertToHdf5Type(eveType);
	void* getDataBufferAddress(eveDataMessage*);
	int getMinimumDataBufferLength(eveDataMessage*, int);
	void init(eveDataMessage*);
	void addAttributes(H5Object*);
	void addLink(QString, QString, QString);
	bool isInit;
	int arraySize;
	int posCounter;
	int status;
	H5File* dataFile;
	QString errorString;
	eveType dataType;
	memSpace_t *memBuffer;
	QString dspath;			// e.g. /1/XMLID
	QString name;
	QString basename;
	QStringList params;
	DataSpace dspace;
	DataSpace memspace;
	DataSet dset;
	Group targetGroup;
	int rank;
	hsize_t maxdim[2];
	hsize_t currentDim[2];
	hsize_t currentOffset[2];
	hsize_t chunk_dims[2];
	CompType compoundType;
	DSetCreatPropList createProps;
	bool dsetOpen;
	int sizeIncrement;

};

#endif /* HDF5DATASET_H_ */
