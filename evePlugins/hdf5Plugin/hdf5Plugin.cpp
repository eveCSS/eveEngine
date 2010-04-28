/*
 * hdf5Plugin.cpp
 *
 *  Created on: 13.11.2009
 *      Author: eden
 */

#include <iostream>
#include <string>

#include <QtGui>
#include "hdf5Plugin.h"

using namespace std;

hdf5Plugin::hdf5Plugin() {
	// TODO Auto-generated constructor stub
	fileName = "";
	sizeIncrement = 50; // default file size increment
	isFileOpen = false;
}

hdf5Plugin::~hdf5Plugin() {

	if (isFileOpen){
		try {
			dataFile->close();
		}
		catch( Exception error )
		{
			cerr << qPrintable(QString(": closingFile: %1").arg(error.getCDetailMsg())) << endl;
		}
	}
}

/**
 * @brief			open the File (all setCol commands are done), noop for hdf5
 * @param setID		dataset-identification (chain-id)
 * @return			error severity
 */
int hdf5Plugin::open(int setID)
{
	int status = SUCCESS;
	errorString.clear();
	if (!setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin: Data Set with id %1, is not initialized").arg(setID);
		status = ERROR;
	}
	if (!isFileOpen) {
		errorString = QString("HDF5Plugin: data file %1 has not been opened").arg(fileName);
		status = ERROR;
	}
	return status;
}

/**
 *
 * @param setID 	dataset-identification (chain-id)
 * @param filename 	Filename including absolute path
 * @param format	dataformat
 * @param parameter	Plugin-parameter from xml
 * @return			error severity
 */
int hdf5Plugin::init(int setID, QString filename, QString format, QHash<QString, QString>* parameter){
	if (setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin: Data Set with id %1, is already initialized").arg(setID);
		return MINOR;
	}
	if (!fileName.isEmpty() && (fileName != filename)){
		errorString = QString("HDF5Plugin: unable to handle more than one filename: %1").arg(filename);
		return MINOR;
	}
	if (format != "HDF5"){
		errorString = QString("HDF5Plugin: unable to handle any other format than HDF5; not: %1").arg(filename);
		return ERROR;
	}
	setIdList.append(setID);
	if (parameter->contains("extent")){
		int tmp = parameter->value("extent").toInt();
		if ((tmp > 0) && (tmp < 1000000)) sizeIncrement = tmp;
	}

	fileName = filename;
	fileFormat = format;
	errorString.clear();

	if (!isFileOpen){
		try
		{
			Exception::dontPrint();
			dataFile = new H5File(qPrintable(fileName), H5F_ACC_TRUNC);
			isFileOpen = true;
		}
		catch( FileIException error )
		{
			errorString = QString("HDF5Plugin: %1").arg(error.getCDetailMsg());
			return ERROR;
		}
	}
	return SUCCESS;
}


int hdf5Plugin::setXMLData(QByteArray* xmldata){
	int retval = SUCCESS;
	if (isFileOpen) {
		QString xmlfilename = fileName + ".scml";
		QFile* filePtr = new QFile(xmlfilename);
		if (!filePtr->open(QIODevice::ReadWrite)){
			errorString = QString("AsciiFileWriter: error opening File %1").arg(xmlfilename);
			retval = ERROR;
		}
		else {
			if (filePtr->write(*xmldata) < 0){
				errorString = QString("AsciiFileWriter: error writing xmldata to File %1").arg(xmlfilename);
				retval = ERROR;
			}
			filePtr->close();
		}
	}
	if (xmldata != NULL) delete xmldata;
	return retval;
}

/**
 * @brief			create and label a column
 * @param setID 	dataset-identification (chain-id)
 * @param colid		xml-id of detectorchannel / axis
 * @param name		name of column (name as in xml)
 * @param info		stringlist with additional info
 * @return			error severity
 */
int hdf5Plugin::setCols(int setID, QString colid, QString name, QStringList info){

	if (!setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin: setID %1 has not been initialized").arg(setID);
		return ERROR;
	}
	errorString.clear();
	if (!idHash.contains(setID)){
		idHash.insert(setID, new QHash<QString, columnInfo*>());
	}

	QHash<QString, columnInfo* >* colHashPtr = idHash.value(setID);
	if (colHashPtr->contains(colid)){
		errorString = QString("HDF5Plugin: device already in column list");
		return MINOR;
	}
	else {
		colHashPtr->insert(colid, new columnInfo(name, info));
	}

	return SUCCESS;
}

/**
 *
 * @param setID		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
int hdf5Plugin::addData(int setID, eveDataMessage* data)
{
	if (!setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin: setID %1 has not been initialized").arg(setID);
		return ERROR;
	}
	if (!idHash.contains(setID)) {
		errorString = QString("HDF5Plugin: Columns for setID %1 not set").arg(setID);
		return ERROR;
	}
	if (data->getArraySize() != 1){
		//TODO handle array data
		// return addArrayData(setID, data);
		return ERROR;
	}

	QString colId = data->getXmlId();
	columnInfo* colInfo = idHash.value(setID)->value(colId);
	if (colInfo == NULL){
		errorString = QString("HDF5Plugin: Column for device %1 not in hash").arg(colId);
		return ERROR;
	}
	if (colInfo->isNotInit){

		try {
			Exception::dontPrint();

			//Create the data space.
			hsize_t maxdim[] = {H5S_UNLIMITED};   /* Dataspace dimensions file*/
			colInfo->currentDim[0] = sizeIncrement;
			colInfo->dspace = DataSpace( 1, colInfo->currentDim, maxdim );

			hsize_t memdim[] = {1};   			/* Dataspace dimensions memory*/
			colInfo->memspace = DataSpace( 1, memdim );

			//Modify dataset creation properties, i.e. enable chunking.
			DSetCreatPropList createProps;
			hsize_t chunk_dims[] = {1};
			createProps.setChunk( 1, chunk_dims );

			colInfo->compoundType = createStandardDataType(qPrintable(data->getName()), QString("PosCounter"), data->getDataType());

			// Create the dataset.
			colInfo->dset = dataFile->createDataSet(qPrintable(data->getName()), colInfo->compoundType, colInfo->dspace, createProps);
			colInfo->dset.extend( colInfo->currentDim );

			// add Attribute XML-ID=colId
			hsize_t stringdim[1];
			stringdim[0] = colId.toLocal8Bit().length();
			Attribute attrib = colInfo->dset.createAttribute("XML-ID", PredType::C_S1, DataSpace(1, stringdim));
			attrib.write(PredType::C_S1, qPrintable(colId));
			//TODO add other attributes after changing info from QStringList to a parahash
			// add Attribute PV=

			DataSpace filespace = colInfo->dset.getSpace();
			colInfo->currentOffset[0] = 0;
			hsize_t dims1[1] = {1};            /* data1 dimensions */
			filespace.selectHyperslab( H5S_SELECT_SET, dims1, colInfo->currentOffset );

			void *buffer = getBufferAddress(data);
			colInfo->dset.write( buffer, colInfo->compoundType, colInfo->memspace, filespace );
			++colInfo->currentOffset[0];
		}
		catch( Exception error )
		{
			errorString = QString("HDF5Plugin: %1").arg(error.getCDetailMsg());
			setIdList.removeAll(setID);
			return ERROR;
		}
		colInfo->isNotInit = false;


	}
	else {
		// not the first value, init has been done

		DataSpace filespace = colInfo->dset.getSpace();
		hsize_t dims1[] = {1};            /* data1 dimensions */
		filespace.selectHyperslab( H5S_SELECT_SET, dims1, colInfo->currentOffset );
		void *buffer = getBufferAddress(data);
		colInfo->dset.write( buffer, colInfo->compoundType, colInfo->memspace, filespace );
		++colInfo->currentOffset[0];
		if (colInfo->currentOffset[0] >= colInfo->currentDim[0]){
			colInfo->currentDim[0] += sizeIncrement;
			colInfo->dset.extend( colInfo->currentDim );
		}
	}
	errorString = QString("HDF5Plugin: successfully added %1. value").arg(colInfo->currentOffset[0]-1);
    return SUCCESS;
}

/**
 * @brief			close File
 * @param setID		dataset-identification (chain-id)
 * @return			error severity
 */
int hdf5Plugin::close(int setID)
{
	if (!setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin: unable to close; setID %1 has not been initialized").arg(setID);
		return ERROR;
	}
	setIdList.removeAll(setID);
	if (!idHash.contains(setID)) {
		errorString = QString("HDF5Plugin: Columns for setID %1 not set").arg(setID);
		return MINOR;
	}
	int status = SUCCESS;
	errorString = QString("HDF5Plugin");

	QHash<QString, columnInfo* >* columnHash = idHash.take(setID);
	foreach (columnInfo* colInfo, *columnHash){
		if (!colInfo->isNotInit){
			try {
				colInfo->dset.extend( colInfo->currentOffset );
				colInfo->dset.close();
			}
			catch( Exception error )
			{
				errorString += QString(": %1").arg(error.getCDetailMsg());
				status = ERROR;
			}
		}
		delete colInfo;
	}
	delete columnHash;
	idHash.remove(setID);

	if (idHash.isEmpty()){
		try {
			dataFile->close();
			isFileOpen = false;
			errorString += QString(": successfully closed file: %1").arg(fileName);
		}
		catch( Exception error )
		{
			errorString += QString(": closingFile: %1").arg(error.getCDetailMsg());
			status = ERROR;
		}
	}
    return status;
}


QString hdf5Plugin::errorText()
{
    return errorString;
}

hdf5Plugin::columnInfo::columnInfo(QString colname, QStringList colinfo){
	isNotInit = true;
	name = colname;
	info = colinfo;
}

PredType hdf5Plugin::convertToHdf5Type(eveType type){
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
			return PredType::NATIVE_UINT64;
		case eveEnum16T:
		case eveStringT:
			return PredType::C_S1;
		default:
			return PredType::NATIVE_UINT32;
	}
}

CompType hdf5Plugin::createStandardDataType(QString name1, QString name2, eveType type){


	size_t size, offsetA, offsetB;

	DataType typeA;
	DataType typeB(PredType::NATIVE_INT32);

	switch (type) {
	case eveInt8T:
		offsetA = HOFFSET(sInt8_t, a);
		offsetB = HOFFSET(sInt8_t, b);
		typeA = PredType::NATIVE_INT8;
		size = sizeof(sInt8_t);
		break;
	case eveUInt8T:
		offsetA = HOFFSET(sUInt8_t, a);
		offsetB = HOFFSET(sUInt8_t, b);
		typeA = PredType::NATIVE_UINT8;
		size = sizeof(sUInt8_t);
		break;
	case eveInt16T:
		offsetA = HOFFSET(sInt16_t, a);
		offsetB = HOFFSET(sInt16_t, b);
		typeA = PredType::NATIVE_INT16;
		size = sizeof(sInt16_t);
		break;
	case eveUInt16T:
		offsetA = HOFFSET(sUInt16_t, a);
		offsetB = HOFFSET(sUInt16_t, b);
		typeA = PredType::NATIVE_UINT16;
		size = sizeof(sUInt16_t);
		break;
	case eveInt32T:
		offsetA = HOFFSET(sInt32_t, a);
		offsetB = HOFFSET(sInt32_t, b);
		typeA = PredType::NATIVE_INT32;
		size = sizeof(sInt32_t);
		break;
	case eveUInt32T:
		offsetA = HOFFSET(sUInt32_t, a);
		offsetB = HOFFSET(sUInt32_t, b);
		typeA = PredType::NATIVE_UINT32;
		size = sizeof(sUInt32_t);
		break;
	case eveDateTimeT:
		offsetA = HOFFSET(sUInt64_t, a);
		offsetB = HOFFSET(sUInt64_t, b);
		typeA = PredType::NATIVE_UINT64;
		size = sizeof(sUInt64_t);
		break;
	case eveFloat32T:
		offsetA = HOFFSET(sFloat_t, a);
		offsetB = HOFFSET(sFloat_t, b);
		typeA = PredType::NATIVE_FLOAT;
		size = sizeof(sFloat_t);
		break;
	case eveFloat64T:
		offsetA = HOFFSET(sDouble_t, a);
		offsetB = HOFFSET(sDouble_t, b);
		typeA = PredType::NATIVE_DOUBLE;
		size = sizeof(sDouble_t);
		break;
	case eveEnum16T:
		offsetA = HOFFSET(sEumString_t, a);
		offsetB = HOFFSET(sEumString_t, b);
		size = sizeof(sEumString_t);
		typeA = StrType(PredType::C_S1, STANDARD_ENUM_STRINGSIZE +1);
		break;
	case eveStringT:
		offsetA = HOFFSET(sString_t, a);
		offsetB = HOFFSET(sString_t, b);
		size = sizeof(sString_t);
		typeA = StrType(PredType::C_S1, STANDARD_STRINGSIZE +1);
		break;
	default:
		return (CompType) NULL;
	}

	CompType comptype( size );
	// printf("in progress: %s %s \n", qPrintable(name1), qPrintable(name2));
	comptype.insertMember( qPrintable(name1), offsetA, typeA);
	comptype.insertMember( qPrintable(name2), offsetB, typeB);
	return comptype;
}

void* hdf5Plugin::getBufferAddress(eveDataMessage* data){

	void *buffer;
	switch (data->getDataType()) {
	case eveInt8T:
		sInt8.a = (qint8)data->getCharArray()[0];
		sInt8.b = data->getPositionCount();
		buffer = (void *) &sInt8;
		break;
	case eveUInt8T:
		sUInt8.a = (quint8)data->getCharArray()[0];
		sUInt8.b = data->getPositionCount();
		buffer = (void *) &sUInt8;
		break;
	case eveInt16T:
		sInt16.a = (qint16)data->getShortArray()[0];
		sInt16.b = data->getPositionCount();
		buffer = (void *) &sInt16;
		break;
	case eveUInt16T:
		sUInt16.a = (quint16)data->getShortArray()[0];
		sUInt16.b = data->getPositionCount();
		buffer = (void *) &sUInt16;
		break;
	case eveInt32T:
		sInt32.a = (qint32)data->getIntArray()[0];
		sInt32.b = data->getPositionCount();
		buffer = (void *) &sInt32;
		break;
	case eveUInt32T:
		sUInt32.a = (quint32)data->getIntArray()[0];
		sUInt32.b = data->getPositionCount();
		buffer = (void *) &sUInt32;
		break;
	case eveFloat32T:
		sFloat.a = data->getFloatArray()[0];
		sFloat.b = data->getPositionCount();
		buffer = (void *) &sFloat;
		break;
	case eveFloat64T:
		sDouble.a = data->getDoubleArray()[0];
		sDouble.b = data->getPositionCount();
		buffer = (void *) &sDouble;
		break;
	case eveDateTimeT:
		sUInt64.a = data->getDateTime().get64bitTime();
		sUInt64.b = data->getPositionCount();
		buffer = (void *) &sUInt64;
		break;
	case eveEnum16T:
		strncpy(sEumString.a, data->getStringArray().first().toLocal8Bit().constData(), STANDARD_ENUM_STRINGSIZE);
		sEumString.a[STANDARD_ENUM_STRINGSIZE] = '\0';
		sEumString.b = data->getPositionCount();
		buffer = (void *) &sEumString;
		break;
	case eveStringT:
		strncpy(sString.a, data->getStringArray().first().toLocal8Bit().constData(), STANDARD_STRINGSIZE);
		sString.a[STANDARD_STRINGSIZE] = '\0';
		sString.b = data->getPositionCount();
		buffer = (void *) &sString;
		break;
	default:
		return NULL;
	}

	return buffer;
}

Q_EXPORT_PLUGIN2(hdf5plugin, hdf5Plugin);
