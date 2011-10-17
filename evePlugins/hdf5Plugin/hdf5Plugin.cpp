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
int hdf5Plugin::init(int setID, QString filename, QString format, QHash<QString, QString>& parameter){
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
	if (parameter.contains("extent")){
		int tmp = parameter.value("extent").toInt();
		if ((tmp > 0) && (tmp < 1000000)) sizeIncrement = tmp;
	}

	fileName = filename;
	fileFormat = format;
	errorString.clear();
	comment.clear();

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
			errorString = QString("HDF5Plugin: error opening File %1").arg(xmlfilename);
			retval = ERROR;
		}
		else {
			if (filePtr->write(*xmldata) < 0){
				errorString = QString("HDF5Plugin: error writing xmldata to File %1").arg(xmlfilename);
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
 * \brief create the datasets, since we now know the datatype
 *
 * @param setID		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
int hdf5Plugin::addData(int setID, eveDataMessage* data)
{
	if (!isFileOpen) {
		errorString = QString("HDF5Plugin:addData: data file has not been opened");
		return ERROR;
	}
	if (!setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin:addData: setID %1 has not been initialized").arg(setID);
		return ERROR;
	}
	if (!idHash.contains(setID)) {
		errorString = QString("HDF5Plugin:addData: Columns for setID %1 not set").arg(setID);
		return ERROR;
	}
	QString colId = data->getXmlId();
	if (colId.length() < 1){
		errorString = QString("HDF5Plugin:addData: Columns invalid xml-id");
		return ERROR;
	}
//	if (data->getArraySize() != 1){
//		//TODO handle array data
//		errorString = QString("HDF5Plugin:addData: array data not yet implemented");
//		// return addArrayData(setID, data);
//		return ERROR;
//	}

	columnInfo* colInfo = idHash.value(setID)->value(colId);
	if (colInfo == NULL){
		errorString = QString("HDF5Plugin:addData: Column for device %1 not in hash").arg(colId);
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

			colInfo->arraySize = data->getArraySize();
			colInfo->compoundType = createDataType(QString("PosCounter"), qPrintable(colId), data->getDataType(), colInfo->arraySize);

			// Create the memory buffer.
			colInfo->memSize = colInfo->compoundType.getSize();
			colInfo->memBuffer = (memSpace_t *) calloc(1, colInfo->memSize);

			// Create the dataset.
			colInfo->dset = dataFile->createDataSet(qPrintable(colId), colInfo->compoundType, colInfo->dspace, createProps);
			colInfo->dset.extend( colInfo->currentDim );

			// add attribute XML-ID
			hsize_t stringDim = 1;
			StrType st = StrType(PredType::C_S1, colId.toLocal8Bit().length());
			Attribute attrib = colInfo->dset.createAttribute("XML-ID", st, DataSpace(1, &stringDim));
			attrib.write(st, qPrintable(colId));

			// add more attributes
			foreach(QString infoLine, colInfo->info){
				QStringList infoSplit = infoLine.split(":");
				if (infoSplit.count() > 1){
					QString attribName = infoSplit.takeFirst();
					QString attribValue = infoSplit.join(":");
					if ((attribName.length() > 0) && (attribValue.length() > 0)) {
						StrType st = StrType(PredType::C_S1, attribValue.toLocal8Bit().length());
						Attribute attrib = colInfo->dset.createAttribute(qPrintable(attribName), st, DataSpace(1, &stringDim));
						attrib.write(st, qPrintable(attribValue));
					}
				}
			}
			colInfo->currentOffset[0] = 0;
		}
		catch( Exception error )
		{
			errorString = QString("HDF5Plugin:addData: %1").arg(error.getCDetailMsg());
			setIdList.removeAll(setID);
			return ERROR;
		}
		colInfo->isNotInit = false;


	}

	DataSpace filespace = colInfo->dset.getSpace();
	hsize_t dims1[] = {1};             /* data1 dimensions */
	filespace.selectHyperslab( H5S_SELECT_SET, dims1, colInfo->currentOffset );

	// zero the memory buffer
	memset(colInfo->memBuffer, 0, colInfo->memSize);
	colInfo->memBuffer->positionCount = data->getPositionCount();
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

		char *start = (char*) &colInfo->memBuffer->aPtr;
		foreach(QString dataString, data->getStringArray()){
			if((start + stringLength) <= (((char*) &colInfo->memBuffer->aPtr) + colInfo->memSize - sizeof(qint32))){
				strncpy(start, dataString.toLocal8Bit().constData(), stringLength);
				start += stringLength;
			}
		}
	}
	else {
		void *buffer = getDataBufferAddress(data);
		colInfo->memBuffer->positionCount = data->getPositionCount();
		memcpy(&colInfo->memBuffer->aPtr, buffer, getMinimumDataBufferLength(data, colInfo->memSize - sizeof(qint32)));
	}
	colInfo->dset.write( (void*)colInfo->memBuffer, colInfo->compoundType, colInfo->memspace, filespace );

	++colInfo->currentOffset[0];
	if (colInfo->currentOffset[0] >= colInfo->currentDim[0]){
		colInfo->currentDim[0] += sizeIncrement;
		colInfo->dset.extend( colInfo->currentDim );
	}
	errorString = QString("HDF5Plugin:addData: successfully added %1. value").arg(colInfo->currentOffset[0]-1);
    return SUCCESS;
}

/**
 * @brief add a comment to be written at end of file
 */
int hdf5Plugin::addComment(int setID, QString newComment){
	if (!setIdList.contains(setID)) {
		errorString = QString("HDF5Plugin:addComment: setID %1 has not been initialized").arg(setID);
		return ERROR;
	}
	else {
		comment.append(QString("%1; ").arg(newComment));
	}
	errorString.clear();
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
		errorString = QString("HDF5Plugin:close: unable to close; setID %1 has not been initialized").arg(setID);
		return ERROR;
	}
	setIdList.removeAll(setID);
	if (!idHash.contains(setID)) {
		errorString = QString("HDF5Plugin:close: Columns for setID %1 not set").arg(setID);
		return MINOR;
	}
	if (!isFileOpen) {
		errorString = QString("HDF5Plugin:close: file already closed");
		return MINOR;
	}
	int status = SUCCESS;
	errorString = QString("HDF5Plugin");
// TODO
// put the comment into file, not into columns, but this does not work:
//	dataFile->setComment("Comment", qPrintable(comment));
	QHash<QString, columnInfo* >* columnHash = idHash.take(setID);
	foreach (columnInfo* colInfo, *columnHash){
		if (!colInfo->isNotInit){
			try {
				// write the comment
				if (comment.length() > 0) {
					hsize_t stringDim = 1;
					StrType st = StrType(PredType::C_S1, comment.toLocal8Bit().length());
					Attribute attrib = colInfo->dset.createAttribute("Comment", st, DataSpace(1, &stringDim));
					attrib.write(st, qPrintable(comment));
				}
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
			delete dataFile;
			dataFile = NULL;
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
	memBuffer = NULL;
}

hdf5Plugin::columnInfo::~columnInfo(){
	if (memBuffer) free(memBuffer);
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
		case eveEnum16T:
		case eveStringT:
			return PredType::C_S1;
		default:
			return PredType::NATIVE_UINT32;
	}
}


CompType hdf5Plugin::createDataType(QString name1, QString name2, eveType type, int arrayCount){

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


void* hdf5Plugin::getDataBufferAddress(eveDataMessage* data){

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

/**
 *
 * @param data eveDataMessage
 * @param number compare to this
 * @return the minimum of number and the used bytes in the data array
 */
int hdf5Plugin::getMinimumDataBufferLength(eveDataMessage* data, int other){

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

Q_EXPORT_PLUGIN2(hdf5plugin, hdf5Plugin);
