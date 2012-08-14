/*
 * hdf5DataSet.cpp
 *
 *  Created on: 07.05.2012
 *      Author: eden
 */

#include "hdf5DataSet.h"
#include <QDateTime>
#include "eveStartTime.h"

hdf5DataSet::hdf5DataSet(QString path, QString colid, QString devicename, QStringList info, H5File* file) {

	dspath = path;
	name = devicename;
	basename = colid;
	params = QStringList(info);
	memBuffer = NULL;
	dataFile = file;
	posCounter = 0;
	arraySize = 1;
	isInit = false;
	dsetOpen = false;
	sizeIncrement = 50;
	isModifiedData = false;
}

hdf5DataSet::~hdf5DataSet() {

	close();
	if (arraySize == 1) {
		if (memBuffer != NULL) free(memBuffer);
	}
	else {
		if (isInit) targetGroup.close();
	}
}

void hdf5DataSet::close() {

	if (dsetOpen) {
		dset.extend( currentOffset );
		dset.close();
		dsetOpen = false;
	}
}

int hdf5DataSet::addData(eveDataMessage* data){

	status = DEBUG;
	errorString.clear();
	if (!isInit) init(data);

	QString dsname = dspath;
	if (arraySize == 1){
		DataSpace filespace;
		try {
			filespace = dset.getSpace();
			filespace.selectHyperslab( H5S_SELECT_SET, chunk_dims, currentOffset );

		}
		catch (...) {
			errorString += QString("hdf5DataSet:addData: error opening dataset %1").arg(dsname);
			return ERROR;
		}

		// zero the memory buffer
		memset(memBuffer, 0, compoundType.getSize());

		if (data->getChainId() == 0)
			memBuffer->positionCount = data->getMSecsSinceStart();
		else
			memBuffer->positionCount = data->getPositionCount();

		if ((data->getDataType() == eveEnum16T) || (data->getDataType() == eveStringT) || (data->getDataType() == eveDateTimeT)){
			int stringLength;
			if (data->getDataType() == eveStringT) {
				stringLength = STANDARD_STRINGSIZE+1;
			}
			else if (data->getDataType() == eveDateTimeT){
				stringLength = DATETIME_STRINGSIZE+1;
			}
			else {
				stringLength = STANDARD_ENUM_STRINGSIZE+1;
			}

			char *start = (char*) &memBuffer->aPtr;
			foreach(QString dataString, data->getStringArray()){
				if((start + stringLength) <= (((char*) &memBuffer->aPtr) + compoundType.getSize() - sizeof(qint32))){
					strncpy(start, dataString.toLocal8Bit().constData(), stringLength);
					start += stringLength;
				}
			}
		}
		else {
			void *buffer = getDataBufferAddress(data);
			memcpy(&memBuffer->aPtr, buffer, getMinimumDataBufferLength(data, compoundType.getSize() - sizeof(qint32)));
		}
		dset.write( (void*)memBuffer, compoundType, memspace, filespace );

		++currentOffset[0];
		if (currentOffset[0] >= currentDim[0]){
			currentDim[0] += sizeIncrement;
			dset.extend( currentDim );
		}
	}
	else if (isModifiedData){
		DataSpace filespace;
		try {
			filespace = dset.getSpace();
			filespace.selectHyperslab( H5S_SELECT_SET, chunk_dims, currentOffset );

		}
		catch (...) {
			errorString += QString("hdf5DataSet:addData: error opening dataset %1").arg(dsname);
			return ERROR;
		}

		modBuffer.xval = data->getDoubleArray().at(0);
		modBuffer.yval = data->getDoubleArray().at(1);
		modBuffer.positionCount = data->getPositionCount();
		dset.write( (void*)&modBuffer, compoundType, memspace, filespace );

		++currentOffset[0];
		if (currentOffset[0] >= currentDim[0]){
			currentDim[0] += sizeIncrement;
			dset.extend( currentDim );
		}
	}
	else {
		if (posCounter < data->getPositionCount()) {
			if (dsetOpen) {
				try {
					currentDim[1] = currentOffset[1];
					dset.extend( currentDim );
					dset.close();
					dsetOpen = false;
				}
				catch (...) {
					errorString += QString("hdf5DataSet:addData: error closing dataset posCount %1").arg(posCounter);
					return ERROR;
				}
			}
			posCounter = data->getPositionCount();
		}
		else if (posCounter > data->getPositionCount()) {
			// TODO
			// reopen already closed dataset
			errorString += QString("hdf5DataSet:addData: posCounter must be monotonically increasing");
			return ERROR;
		}
		dsname = QString("%1/%2").arg(dspath).arg(posCounter);
		if (!dsetOpen){
			try {
				currentDim[0] = arraySize;		// not constant
				currentDim[1] = 1; 				// default array sizeIncrement
				currentOffset[0] = 0;
				currentOffset[1] = 0;
				dset = dataFile->createDataSet(qPrintable(dsname), convertToHdf5Type(dataType), dspace, createProps);
				dset.extend( currentDim );
				dsetOpen = true;
			}
			catch (...) {
				errorString += QString("hdf5DataSet:addData: error creating dataset %1").arg(dsname);
				return ERROR;
			}
			errorString += QString("successfully created dataset %1").arg(dsname);
		}

		try {
			DataSpace filespace = dset.getSpace();
			filespace.selectHyperslab( H5S_SELECT_SET, chunk_dims, currentOffset );

			++currentOffset[1];
			dset.write( getDataBufferAddress(data), convertToHdf5Type(dataType), memspace, filespace );
			if (currentOffset[1] >= currentDim[1]){
				currentDim[1] += sizeIncrement;
				dset.extend( currentDim );
			}
		}
		catch (...) {
			errorString += QString("hdf5DataSet:addData: error writing to dataset %1").arg(dsname);
			return ERROR;
		}
		errorString += QString("successfully wrote dataset %1").arg(dsname);
	}
	return status;
}

void hdf5DataSet::init(eveDataMessage* data){

	arraySize = data->getArraySize();
	dataType = data->getDataType();
	if ((arraySize == 2) && (data->getDataMod() != DMTunmodified)) isModifiedData = true;

	if ((arraySize == 1)||(isModifiedData)){
		rank = 1;
		maxdim[0] = H5S_UNLIMITED;   /* Dataspace dimensions file*/
		currentDim[0] = sizeIncrement;
		chunk_dims[0] = 1;
	}
	else {
		sizeIncrement = 1;
		rank = 2;
		maxdim[0] = arraySize;
		maxdim[1] = H5S_UNLIMITED;
		currentDim[0] = arraySize;		// not constant
		currentDim[1] = sizeIncrement; 	// default array sizeIncrement
		chunk_dims[0] = arraySize;		// constant
		chunk_dims[1] = sizeIncrement;
	}

	try {
		Exception::dontPrint();

		memspace = DataSpace( rank, chunk_dims );		/* Dataspace dimensions memory*/
		dspace = DataSpace( rank, currentDim, maxdim );
		currentOffset[0] = 0;
		currentOffset[1] = 0;

		//Modify dataset creation properties, i.e. enable chunking.
		createProps.setChunk( rank, chunk_dims );

		if (arraySize == 1){
			QString index = "PosCounter";
			if (data->getChainId() == 0) index = "mSecsSinceStart";

			compoundType = createDataType(index, qPrintable(basename), data->getDataType(), arraySize);
			// Create the memory buffer.
			memBuffer = (memSpace_t*)malloc(compoundType.getSize());

			// Create the dataset.
			dset = dataFile->createDataSet(qPrintable(dspath), compoundType, dspace, createProps);
			dsetOpen = true;
			addAttributes(&dset);

			// add attributes for normalized data
			if (data->getDataMod() == DMTnormalized)
				addModifiedDataAttributes(&dset, "axis", data->getAuxString(), "channel", basename);

			dset.extend( currentDim );
		}
		else if (isModifiedData){
			compoundType = createModDataType(QString("PosCounter"), data->getAuxString(), basename);
			dset = dataFile->createDataSet(qPrintable(dspath), compoundType, dspace, createProps);
			dsetOpen = true;

			// add attributes
			addModifiedDataAttributes(&dset, "axis", data->getAuxString(), "channel", basename);

			dset.extend( currentDim );

		}
		else {
			// Create the group
			targetGroup = dataFile->createGroup(qPrintable(dspath));
			addAttributes(&targetGroup);
		}
	}
	catch( Exception error )
	{
		errorString += QString("hdf5DataSet::init: Exception: %1").arg(error.getCDetailMsg());
		status = ERROR;
	}
	addLink(dspath, basename, name);
	isInit = true;

}

void hdf5DataSet::addAttributes(H5Object *target){

	// add attributes
	hsize_t stringDim = 1;
	while (!params.isEmpty()){
		QStringList infoSplit = params.takeFirst().split(":");
		if (infoSplit.count() > 1){
			QString attribName = infoSplit.takeFirst();
			QString attribValue = infoSplit.join(":");
			if ((attribName.length() > 0) && (attribValue.length() > 0)) {
				StrType st = StrType(PredType::C_S1, attribValue.toLatin1().length());
				Attribute attrib = target->createAttribute(qPrintable(attribName), st, DataSpace(1, &stringDim));
				attrib.write(st, qPrintable(attribValue));
			}
		}
	}
}

void hdf5DataSet::addModifiedDataAttributes(H5Object *target, QString name1, QString val1, QString name2, QString val2){

	// add attributes
	hsize_t stringDim = 1;
	if ((name1.length() > 0) && (val1.length() > 0)) {
		StrType st = StrType(PredType::C_S1, val1.toLatin1().length());
		Attribute attrib = target->createAttribute(qPrintable(name1), st, DataSpace(1, &stringDim));
		attrib.write(st, qPrintable(val1));
	}
	if ((name2.length() > 0) && (val2.length() > 0)) {
		StrType st = StrType(PredType::C_S1, val2.toLatin1().length());
		Attribute attrib = target->createAttribute(qPrintable(name2), st, DataSpace(1, &stringDim));
		attrib.write(st, qPrintable(val2));
	}
}

void hdf5DataSet::addLink(QString name, QString basename, QString linkname){

	bool linkit = false;
	if (linkname.length() > 0){
		linkname.replace(" ","_");
		linkname.replace("/","%");
		QString groupname = name.left(name.lastIndexOf("/"));
		try {
			Group group = dataFile->openGroup(qPrintable(groupname));
			H5G_stat_t buffer;
			group.getObjinfo(qPrintable(linkname), (hbool_t)true, buffer);
		}
		catch (GroupIException) {
			linkit = true;
		}
		catch (...) {
		}
		if (linkit){
			linkname = QString("%1/%2").arg(groupname).arg(linkname);
			errorString += QString("HDF5Plugin::addLink Successfully added link %1").arg(linkname);
			status = DEBUG;
			try {
				dataFile->link(H5G_LINK_SOFT, qPrintable(name), qPrintable(linkname));
			}
			catch (...) {
				errorString += QString("HDF5Plugin::addLink Unable to link to %1").arg(linkname);
				status = ERROR;
			}
		}
		else {
			errorString += QString("HDF5Plugin::addLink link %1/%2 already in use").arg(groupname).arg(linkname);
			status = MINOR;
		}
	}
	else {
		errorString += QString("hdf5DataSet::addLink zero length link name");
		status = DEBUG;
	}
}

/**
 *
 * @param data eveDataMessage
 * @param number compare to this
 * @return the minimum of number and the used bytes in the data array
 */
int hdf5DataSet::getMinimumDataBufferLength(eveDataMessage* data, int other){

	int arraySize = data->getArraySize();
	int bufferSize = 0;

	switch (data->getDataType()) {
	case eveInt8T:
	case eveUInt8T:
		bufferSize = 1;
		break;
	case eveInt16T:
	case eveUInt16T:
		bufferSize = 2;
		break;
	case eveInt32T:
	case eveUInt32T:
	case eveFloat32T:
		bufferSize = 4;
		break;
	case eveFloat64T:
		bufferSize = 8;
		break;
	default:
		break;
	}
	bufferSize *= arraySize;

	if (bufferSize < other)
		return bufferSize;

	return other;
}

void* hdf5DataSet::getDataBufferAddress(eveDataMessage* data){

	switch (data->getDataType()) {
	case eveInt8T:
	case eveUInt8T:
		return (void*) data->getCharArray().constData();
		break;
	case eveInt16T:
	case eveUInt16T:
		return (void*) data->getShortArray().constData();
		break;
	case eveInt32T:
	case eveUInt32T:
		return (void*) data->getIntArray().constData();
		break;
	case eveFloat32T:
		return (void*) data->getFloatArray().constData();
		break;
	case eveFloat64T:
		return (void*) data->getDoubleArray().constData();
		break;
	default:
		return NULL;
	}
}

CompType hdf5DataSet::createDataType(QString name1, QString name2, eveType type, int arrayCount){

	int ndims=1;
	hsize_t arrayDims = arrayCount;
	DataType typeA(PredType::NATIVE_INT32);
	DataType typeB;

	switch (type) {
	case eveInt8T:
	case eveUInt8T:
	case eveInt16T:
	case eveUInt16T:
	case eveInt32T:
	case eveUInt32T:
	case eveFloat32T:
	case eveFloat64T:
		if (arrayCount == 1)
			typeB = AtomType(convertToHdf5Type(type));
		else
			typeB = ArrayType (convertToHdf5Type(type), ndims, &arrayDims);
		break;
	case eveEnum16T:
		if (arrayCount == 1)
			typeB = AtomType(StrType(PredType::C_S1, STANDARD_ENUM_STRINGSIZE+1));
		else
			typeB = ArrayType (StrType(PredType::C_S1, STANDARD_ENUM_STRINGSIZE+1), ndims, &arrayDims);
		break;
	case eveStringT:
		if (arrayCount == 1)
			typeB = AtomType(StrType(PredType::C_S1, STANDARD_STRINGSIZE+1));
		else
			typeB = ArrayType (StrType(PredType::C_S1, STANDARD_STRINGSIZE+1), ndims, &arrayDims);
		break;
	case eveDateTimeT:
		if (arrayCount == 1)
			typeB = AtomType(StrType(PredType::C_S1, DATETIME_STRINGSIZE+1));
		else
			typeB = ArrayType (StrType(PredType::C_S1, DATETIME_STRINGSIZE+1), ndims, &arrayDims);
		break;
	default:
		return CompType(0);
	}

	CompType comptype( typeA.getSize() + typeB.getSize());
	comptype.insertMember( qPrintable(name1), 0, typeA);
	comptype.insertMember( qPrintable(name2), typeA.getSize(), typeB);
	return comptype;
}

CompType hdf5DataSet::createModDataType(QString namePC, QString nameX, QString nameY){

	DataType typePC(PredType::NATIVE_INT32);
	DataType typeX(PredType::NATIVE_DOUBLE);
	DataType typeY(PredType::NATIVE_DOUBLE);

	CompType comptype( typePC.getSize() + typeX.getSize() + typeY.getSize());
	comptype.insertMember( qPrintable(namePC), 0, typePC);
	comptype.insertMember( qPrintable(nameX), typePC.getSize(), typeX);
	comptype.insertMember( qPrintable(nameY), typePC.getSize()+typeX.getSize(), typeY);
	return comptype;
}

PredType hdf5DataSet::convertToHdf5Type(eveType type){
	switch (type) {
		case eveInt8T:
			return PredType::NATIVE_INT8;
		case eveUInt8T:
			return PredType::NATIVE_UINT8;
		case eveInt16T:
			return PredType::NATIVE_INT16;
		case eveUInt16T:
			return PredType::NATIVE_UINT16;
		case eveInt32T:
			return PredType::NATIVE_INT32;
		case eveUInt32T:
			return PredType::NATIVE_UINT32;
		case eveFloat32T:
			return PredType::NATIVE_FLOAT;
		case eveFloat64T:
			return PredType::NATIVE_DOUBLE;
		case eveDateTimeT:
		case eveEnum16T:
		case eveStringT:
			return PredType::C_S1;
		default:
			return PredType::NATIVE_UINT32;
	}
}

