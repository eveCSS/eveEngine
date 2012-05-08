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

hdf5Plugin::hdf5Plugin() {
	// TODO Auto-generated constructor stub
	fileName = "";
	defaultSizeIncrement = 50; // default file size increment
	isFileOpen = false;
	dataFile = NULL;
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
 * @brief			open the File, noop for hdf5
 * @return			error severity
 */
int hdf5Plugin::open()
{
	int status = DEBUG;
	errorString.clear();
	if (!isFileOpen) {
		errorString = QString("HDF5Plugin: data file %1 has not been opened").arg(fileName);
		status = ERROR;
	}
	return status;
}

/**
 *
 * @param filename 	Filename including absolute path
 * @param format	datafile format
 * @param parameter	Plugin-parameter from xml
 * @return			error severity
 */
int hdf5Plugin::init(QString filename, QString format, QHash<QString, QString>& parameter){
	if (!fileName.isEmpty() && (fileName != filename)){
		errorString = QString("HDF5Plugin: unable to handle more than one filename: %1").arg(filename);
		return MINOR;
	}
	if (format != "HDF5"){
		errorString = QString("HDF5Plugin: unable to handle any other format than HDF5; not: %1").arg(filename);
		return ERROR;
	}
	if (parameter.contains("extent")){
		int tmp = parameter.value("extent").toInt();
		if ((tmp > 0) && (tmp < 1000000)) defaultSizeIncrement = tmp;
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
			errorString = QString("HDF5Plugin: successfully opened file %1").arg(fileName);
		}
		catch( FileIException error )
		{
			errorString = QString("HDF5Plugin: %1").arg(error.getCDetailMsg());
			return ERROR;
		}
	}
	return DEBUG;
}


int hdf5Plugin::setXMLData(QByteArray* xmldata){
	int retval = DEBUG;
	if (isFileOpen) {
		QString xmlfilename = fileName + ".scml";
		QFile* filePtr = new QFile(xmlfilename);
		if (!filePtr->open(QIODevice::ReadWrite)){
			errorString = QString("HDF5Plugin: error opening xml file %1").arg(xmlfilename);
			retval = ERROR;
		}
		else {
			if (filePtr->write(*xmldata) < 0){
				errorString = QString("HDF5Plugin: error writing xmldata to File %1").arg(xmlfilename);
				retval = ERROR;
			}
			else {
				errorString = QString("HDF5Plugin: successfully wrote xmldata to File %1").arg(xmlfilename);
			}
			filePtr->close();
		}
	}
	return retval;
}

/**
 * @brief			create and label a column
 * @param pathId 	chainId
 * @param colid		xml-id of detectorchannel / axis
 * @param name		name of column (name as in xml)
 * @param info		stringlist with additional info
 * @return			error severity
 */
int hdf5Plugin::setCols(int pathId, QString colid, QString name, QStringList info){

	errorString.clear();
	QString dsname = getDSName(pathId, colid);

	if (!isFileOpen) {
		errorString = QString("HDF5Plugin:setCols: data file has not been opened");
		return ERROR;
	}

	if (dsNameHash.contains(dsname)){
		errorString = QString("HDF5Plugin: Data Set with name %1, is already initialized").arg(dsname);
		return INFO;
	}
	createGroup(pathId);
//	dsNameHash.insert(dsname, new columnInfo(name, info));
	hdf5DataSet *ds = new hdf5DataSet(dsname, colid, name, info, dataFile);
	ds->setSizeIncrement(defaultSizeIncrement);
	dsNameHash.insert(dsname, ds);
	return DEBUG;
}

/**
 * \brief create the datasets, since we now know the datatype
 *
 * @param pathId		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
int hdf5Plugin::addData(int pathId, eveDataMessage* data)
{
	int status;
	hdf5DataSet *ds;

	if (!isFileOpen) {
		errorString = QString("HDF5Plugin:addData: data file has not been opened");
		return ERROR;
	}
	errorString.clear();

	QString dsname = getDSName(pathId, data->getXmlId());

	if (dsNameHash.contains(dsname)) {
		ds = dsNameHash.value(dsname);
	}
	else {
		ds = new hdf5DataSet(dsname, data->getXmlId(), data->getName(), QStringList(), dataFile);
		dsNameHash.insert(dsname, ds);
		ds->setSizeIncrement(defaultSizeIncrement);
	}
	status = ds->addData(data);
	errorString = ds->getError();
	return status;
}

/**
 * \brief create the datasets, since we now know the datatype
 *
 * @param pathId		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
//int hdf5Plugin::addArrayData(int pathId, eveDataMessage* data)
//{
//
//	QString dsname = getDSName(pathId, data->getXmlId());
//	if (!dsNameHash.contains(dsname)){
//		createGroup(pathId);
//		dsNameHash.insert(dsname, new columnInfo(data->getName(), QStringList()));
//	}
//	columnInfo* colInfo = dsNameHash.value(dsname);
//
//	if (colInfo->isNotInit){
//		colInfo->arraySize = data->getArraySize();
//		colInfo->dataType = data->getDataType();
//		if (initArrayDataSet(dsname, colInfo, data->getXmlId()) == ERROR) return ERROR;
//		addLink(pathId, dsname, data->getName());
//	}
//
//	// create a new dataset if positionCounter increased
//	if (data->getPositionCount() > colInfo->posCounter){
//		if (colInfo->dsetOpen){
//			// TODO
//			// colInfo->dset.extend( colInfo->currentOffset );
//			colInfo->dset.close();
//		}
//		colInfo->posCounter = data->getPositionCount();
//		// Create the dataset.
//		dsname = QString("%1/%2").arg(dsname).arg(colInfo->posCounter);
//		colInfo->dset = dataFile->createDataSet(qPrintable(dsname), convertToHdf5Type(colInfo->dataType), colInfo->dspace, colInfo->createProps);
//		colInfo->dset.extend( colInfo->currentDim );
//	}
//	else if (data->getPositionCount() == colInfo->posCounter){
//
//	}
//	else {
//		errorString = QString("HDF5Plugin:addData: posCounter must be monotonically increasing");
//		return ERROR;
//	}
//
//	DataSpace filespace = colInfo->dset.getSpace();
//	hsize_t dims1[2];             /* data1 dimensions */
//	dims1[0] = colInfo->arraySize;
//	dims1[1] = 1;
//	filespace.selectHyperslab( H5S_SELECT_SET, dims1, colInfo->currentOffset );
//
//	colInfo->dset.write( getDataBufferAddress(data), convertToHdf5Type(colInfo->dataType), colInfo->memspace, filespace );
//
//	++colInfo->currentOffset[1];
//	if (colInfo->currentOffset[1] >= colInfo->currentDim[1]){
//		colInfo->currentDim[1] += colInfo->sizeIncrement;
//		colInfo->dset.extend( colInfo->currentDim );
//	}
//	errorString = QString("HDF5Plugin:addData: successfully added %1. value").arg(colInfo->currentOffset[0]-1);
//    return DEBUG;
//}

/**
 * \brief create the datasets, since we now know the datatype
 *
 * @param pathId		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
//int hdf5Plugin::addSingleData(int pathId, eveDataMessage* data)
//{
//
//	QString dsname = getDSName(pathId, data->getXmlId());
//	if (!dsNameHash.contains(dsname)){
//		createGroup(pathId);
//		dsNameHash.insert(dsname, new columnInfo(data->getName(), QStringList()));
//	}
//	columnInfo* colInfo = dsNameHash.value(dsname);
//
//	if (colInfo->isNotInit){
//		colInfo->arraySize = 1;
//		colInfo->dataType = data->getDataType();
//		if (initDataSet(dsname, colInfo, data->getXmlId()) == ERROR) return ERROR;
//		addLink(pathId, dsname, data->getName());
//	}
//
//	DataSpace filespace = colInfo->dset.getSpace();
//	hsize_t dims1[] = {1};             /* data1 dimensions */
//	filespace.selectHyperslab( H5S_SELECT_SET, dims1, colInfo->currentOffset );
//
//	// zero the memory buffer
//	memset(colInfo->memBuffer, 0, colInfo->memSize);
//	colInfo->memBuffer->positionCount = data->getPositionCount();
//	if ((data->getDataType() == eveEnum16T) || (data->getDataType() == eveStringT) || (data->getDataType() == eveDateTimeT)){
//		int stringLength;
//		if (data->getDataType() == eveStringT) {
//			stringLength = STANDARD_STRINGSIZE+1;
//		}
//		else if (data->getDataType() == eveDateTimeT){
//			stringLength = DATETIME_STRINGSIZE+1;
//		}
//		else {
//			stringLength = STANDARD_ENUM_STRINGSIZE+1;
//		}
//
//		char *start = (char*) &colInfo->memBuffer->aPtr;
//		foreach(QString dataString, data->getStringArray()){
//			if((start + stringLength) <= (((char*) &colInfo->memBuffer->aPtr) + colInfo->memSize - sizeof(qint32))){
//				strncpy(start, dataString.toLocal8Bit().constData(), stringLength);
//				start += stringLength;
//			}
//		}
//	}
//	else {
//		void *buffer = getDataBufferAddress(data);
//		colInfo->memBuffer->positionCount = data->getPositionCount();
//		memcpy(&colInfo->memBuffer->aPtr, buffer, getMinimumDataBufferLength(data, colInfo->memSize - sizeof(qint32)));
//	}
//	colInfo->dset.write( (void*)colInfo->memBuffer, colInfo->compoundType, colInfo->memspace, filespace );
//
//	++colInfo->currentOffset[0];
//	if (colInfo->currentOffset[0] >= colInfo->currentDim[0]){
//		colInfo->currentDim[0] += colInfo->sizeIncrement;
//		colInfo->dset.extend( colInfo->currentDim );
//	}
//	errorString = QString("HDF5Plugin:addData: successfully added %1. value").arg(colInfo->currentOffset[0]-1);
//    return DEBUG;
//}
//
//int hdf5Plugin::initDataSet(QString dsname, columnInfo* colInfo, QString colId){
//
//	try {
//		Exception::dontPrint();
//
//		colInfo->sizeIncrement = defaultSizeIncrement;
//
//		//Create the data space.
//		hsize_t maxdim[] = {H5S_UNLIMITED};   /* Dataspace dimensions file*/
//		colInfo->currentDim[0] = colInfo->sizeIncrement;
//		colInfo->dspace = DataSpace( 1, colInfo->currentDim, maxdim );
//
//		hsize_t memdim[] = {1};   			/* Dataspace dimensions memory*/
//		colInfo->memspace = DataSpace( 1, memdim );
//
//		//Modify dataset creation properties, i.e. enable chunking.
//		DSetCreatPropList createProps;
//		hsize_t chunk_dims[] = {1};
//		createProps.setChunk( 1, chunk_dims );
//
//
//		// Create the memory buffer.
//		colInfo->memSize = colInfo->compoundType.getSize();
//		colInfo->memBuffer = (memSpace_t *) calloc(1, colInfo->memSize);
//
//		// Create the dataset.
//		colInfo->dset = dataFile->createDataSet(qPrintable(dsname), colInfo->compoundType, colInfo->dspace, createProps);
//		colInfo->dset.extend( colInfo->currentDim );
//
//		// add attributes
//		hsize_t stringDim = 1;
//		while (!colInfo->info.isEmpty()){
//			QStringList infoSplit = colInfo->info.takeFirst().split(":");
//			if (infoSplit.count() > 1){
//				QString attribName = infoSplit.takeFirst();
//				QString attribValue = infoSplit.join(":");
//				if ((attribName.length() > 0) && (attribValue.length() > 0)) {
//					StrType st = StrType(PredType::C_S1, attribValue.toLocal8Bit().length());
//					Attribute attrib = colInfo->dset.createAttribute(qPrintable(attribName), st, DataSpace(1, &stringDim));
//					attrib.write(st, qPrintable(attribValue));
//				}
//			}
//		}
//		colInfo->currentOffset[0] = 0;
//	}
//	catch( Exception error )
//	{
//		errorString = QString("HDF5Plugin:initDataSet Exception: %1").arg(error.getCDetailMsg());
//		return ERROR;
//	}
//	colInfo->isNotInit = false;
//	return DEBUG;
//}
//
//int hdf5Plugin::initArrayDataSet(QString dsname, columnInfo* colInfo, QString colId){
//
//	try {
//		Exception::dontPrint();
//
//		colInfo->sizeIncrement = 1;
//
//		//Create the data space.
//		const int rank = 2;
//		hsize_t maxdim[2];   /* Dataspace dimensions file*/
//		colInfo->currentDim[0] = colInfo->arraySize;
//		colInfo->currentDim[1] = colInfo->sizeIncrement;
//		maxdim[0] = colInfo->arraySize;
//		maxdim[1] = H5S_UNLIMITED;
//
//		colInfo->dspace = DataSpace( rank, colInfo->currentDim, maxdim );
//
//		hsize_t memdim[2];   			/* Dataspace dimensions memory*/
//		memdim[0] = colInfo->arraySize;
//		memdim[1] = 1;
//		colInfo->memspace = DataSpace( rank, memdim );
//
//		//Modify dataset creation properties, i.e. enable chunking.
//		hsize_t chunk_dims[2];
//		chunk_dims[0] = colInfo->arraySize;
//		chunk_dims[1] = 1;
//		colInfo->createProps.setChunk( rank, chunk_dims );
//
//	    /*
//	     * Set fill value for the dataset
//	     * use default (0)
//	     */
//
//		// Create the group
//		Group targetGroup = dataFile->createGroup(qPrintable(dsname));
//
//		// add attributes
//		hsize_t stringDim = 1;
//		while (!colInfo->info.isEmpty()){
//			QStringList infoSplit = colInfo->info.takeFirst().split(":");
//			if (infoSplit.count() > 1){
//				QString attribName = infoSplit.takeFirst();
//				QString attribValue = infoSplit.join(":");
//				if ((attribName.length() > 0) && (attribValue.length() > 0)) {
//					StrType st = StrType(PredType::C_S1, attribValue.toLatin1().length());
//					Attribute attrib = targetGroup.createAttribute(qPrintable(attribName), st, DataSpace(1, &stringDim));
//					attrib.write(st, qPrintable(attribValue));
//				}
//			}
//		}
//		targetGroup.close();
//		colInfo->currentOffset[1] = 0;
//	}
//	catch( Exception error )
//	{
//		errorString = QString("HDF5Plugin:initDataSet Exception: %1").arg(error.getCDetailMsg());
//		return ERROR;
//	}
//	colInfo->isNotInit = false;
//	return DEBUG;
//}

/**
 * @brief add a metadata (e.g. comment) to be written at end of file
 */
int hdf5Plugin::addMetaData(int pathId, QString attribute, QString stringVal){
	if (!isFileOpen) {
		errorString = QString("HDF5Plugin:addData: data file has not been opened");
		return ERROR;
	}
	errorString.clear();

	try {
		hsize_t stringDim = 1;
		Group targetGroup = dataFile->openGroup(qPrintable(createGroup(pathId)));
		unsigned int index = 0;
		// check if we already have this attribute
		QString compare = QString(attribute);
		QString newVal = QString(stringVal);
		targetGroup.iterateAttrs(&compareNames, &index, (void *) &compare);
		if (compare.isEmpty()){
			Attribute attrib = targetGroup.openAttribute(qPrintable(attribute));
			StrType stread = StrType(attrib.getStrType());
			int length = stread.getSize();
			char *strg_C = new char [(size_t)length+1];
			attrib.read(stread, strg_C);
			strg_C[length] = 0;
			// errorString += QString("existing Attribute: %1 (%2 / %3)").arg(QString::fromLatin1(strg_C)).arg(QString::fromLatin1(strg_C).length()).arg(length);
			attrib.close();
			targetGroup.removeAttr(qPrintable(attribute));
			newVal = QString("%1; %2").arg(QString::fromLatin1(strg_C)).arg(stringVal);
			delete []strg_C;
		}
		StrType st = StrType(PredType::C_S1, newVal.toLatin1().length());
		// UTF8 funktioniert leider nicht (st.setCset(H5T_CSET_UTF8);)
		Attribute attrib = targetGroup.createAttribute(qPrintable(attribute), st, DataSpace(1, &stringDim));
		attrib.write(st, newVal.toLatin1().data());
		errorString += QString("new Attribute: %1 (%2)").arg(attribute).arg(newVal);
		targetGroup.close();
	}
	catch( Exception error )
	{
		errorString += QString(": %1").arg(error.getCDetailMsg());
		return ERROR;
	}

	return DEBUG;
}

void hdf5Plugin::compareNames(H5Object& obj, string str, void* data){
	QString *search = (QString*)(data);
	string other = string(qPrintable(*search));
	if (str == string(qPrintable(*search))) {
		search->clear();
	}
}


/**
 * @brief			close File
 * @param pathId		dataset-identification (chain-id)
 * @return			error severity
 */
int hdf5Plugin::close()
{
	if (!isFileOpen) {
		errorString = QString("HDF5Plugin::close: file %1 already closed").arg(fileName);
		return MINOR;
	}
	int status = DEBUG;
	errorString = QString("HDF5Plugin::close:");

	foreach (hdf5DataSet* ds, dsNameHash.values()){
		delete ds;
	}
	dsNameHash.clear();

	try {
		isFileOpen = false;
		dataFile->close();
		delete dataFile;
		dataFile = NULL;
		errorString += QString(": successfully closed file: %1").arg(fileName);
	}
	catch( Exception error )
	{
		errorString += QString(": closingFile: %1").arg(error.getCDetailMsg());
		status = ERROR;
	}
    return status;
}


QString hdf5Plugin::errorText()
{
    return errorString;
}

QString hdf5Plugin::getDSName(int id, QString name){

	if (id == 0)
		return QString("/%1").arg(name);
	else
		return QString("/%1/%2").arg(id).arg(name);
}

int hdf5Plugin::addLink(int pathId, QString dsname, QString linkname){

	if (linkname.length() > 0){
		linkname.replace(" ","_");
		linkname.replace("/","%");
		linkname = getDSName(pathId, linkname);
		if (!linkNames.contains(linkname)){
			dataFile->link(H5G_LINK_SOFT, qPrintable(dsname), qPrintable(linkname));
			linkNames.append(linkname);
			errorString = QString("HDF5Plugin::addLink Successfully added link %1").arg(linkname);
			return DEBUG;
		}
		else {
			errorString = QString("HDF5Plugin::addLink link %1 already in use").arg(linkname);
			return MINOR;
		}
	}
	errorString = QString("HDF5Plugin::addLink zero length link name not added");
	return DEBUG;
}

QString hdf5Plugin::createGroup(int pathId){

	if (pathId == 0) return QString("/");
	QString groupname = QString("/%1/").arg(pathId);
	if (!groupList.contains(pathId) && isFileOpen) {
		try
		{
			dataFile->createGroup(qPrintable(groupname));
			groupList.append(pathId);
		}
		catch( Exception error )
		{
			errorString += QString("createGroup: %1").arg(error.getCDetailMsg());
		}
	}
	return QString("/%1/").arg(pathId);
}

Q_EXPORT_PLUGIN2(hdf5plugin, hdf5Plugin);
