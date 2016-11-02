/*
 * hdf5DataSet.cpp
 *
 *  Created on: 07.05.2012
 *      Author: eden
 */

#include "hdf5DataSet.h"
#include <QDateTime>
#include "eveStartTime.h"
#include "pcmemspace.h"

hdf5DataSet::hdf5DataSet(QString path, QString colid, QString devicename, QStringList info, H5File* file) {

    dspath = path;
    name = devicename;
    basename = colid;
    params = info;
    memBuffer = NULL;
    dataFile = file;
    posCounter = 0;
    arraySize = 1;
    isInit = false;
    dsetOpen = false;
    sizeIncrement = 50;
    longString = false;

    if (!params.isEmpty()){
        foreach (QString attribute, params) {
            if (attribute.contains("longString") && attribute.contains(":true")) longString = true;
        }
    }

}

hdf5DataSet::~hdf5DataSet() {

    close();
    if (arraySize == 1) {
        if (memBuffer != NULL) delete memBuffer;
        memBuffer = NULL;
    }
    else {
        if (isInit) targetGroup.close();
    }
}

/**
 * @brief close dataset
 */
void hdf5DataSet::close() {

    if (dsetOpen) {
        dset.extend( currentOffset );
        dset.close();
        dsetOpen = false;
    }
}

/**
 * @brief init dataset
 * @param data to write to dataset
 */
void hdf5DataSet::init(eveDataMessage* data){

    arraySize = data->getArraySize();
    dataType = data->getDataType();
    if (arraySize == 1)
        storageType = PosCountValues;
    else if ((arraySize == 2) && (data->getDataMod() != DMTunmodified))
        storageType = PosCountValues;
    else
        storageType = PosCountNamedArray;

    if (storageType == PosCountValues) {
        rank = 1;
        maxdim[0] = H5S_UNLIMITED;   /* Dataspace dimensions file*/
        currentDim[0] = sizeIncrement;
        currentDim[1] = 0;
        chunk_dims[0] = 1;
        chunk_dims[1] = 0;
    }
    else {
        sizeIncrement = 1;
        rank = 2;
        maxdim[0] = arraySize;
        maxdim[1] = H5S_UNLIMITED;
        currentDim[0] = arraySize;		// not constant
        currentDim[1] = 0;
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

        if (storageType == PosCountValues){
            QStringList columnTitels;

            if (data->getDataMod() == DMTdeviceData)
                columnTitels << "mSecsSinceStart";
            else
                columnTitels << "PosCounter";

            if (arraySize == 2) columnTitels << data->getAuxString();

            if (data->getDataMod() == DMTaverageParams)
                columnTitels << data->getName();
            else
                columnTitels << basename;

            compoundType = createDataType(columnTitels, data->getDataType());
            // Create the memory buffer.
            memBuffer = new PCmemSpace(compoundType.getSize(), longString);

            // Create the dataset.
            dset = dataFile->createDataSet(qPrintable(dspath), compoundType, dspace, createProps);
            dsetOpen = true;
            addParamAttributes(&dset);

            // add attributes if not empty
            if (data->getDataMod() == DMTnormalized) {
                addDataAttribute(&dset, "channel", basename);
                addDataAttribute(&dset, "normalizeId", data->getNormalizeId());
            }
            else if (arraySize == 2) {
                addDataAttribute(&dset, "channel", basename);
                if (data->getDataMod() != DMTaverageParams) {
                    addDataAttribute(&dset, "axis", data->getAuxString());
                    addDataAttribute(&dset, "normalizeId", data->getNormalizeId());
                }
            }

            dset.extend( currentDim );
        }
        else {
            // Create the group
            targetGroup = dataFile->createGroup(qPrintable(dspath));
            addParamAttributes(&targetGroup);
        }
    }
    catch( Exception error )
    {
        errorString += QString("hdf5DataSet::init: Exception: %1").arg(error.getCDetailMsg());
        status = ERROR;
    }
    isInit = true;
}

/**
 * @brief add data to a dataset, init the dataset if not already done
 * @param data
 * @return error status
 */
int hdf5DataSet::addData(eveDataMessage* data){

    status = DEBUG;
    errorString.clear();
    if (!isInit) init(data);
    if (!isInit) return ERROR;

    // we don't save severity therefor we overwrite invalid data with NAN
    if (data->getDataStatus().getSeverity() == 3) data->invalidate();

    if (arraySize != data->getArraySize()){
        errorString += QString("hdf5DataSet::addData: array size mismatch: should be %1, but is %2").arg(arraySize).arg(data->getArraySize());
        return ERROR;
    }
    if (dataType != data->getDataType()){
        errorString += QString("hdf5DataSet::addData: datatype mismatch: should be %1, but is %2").arg(dataType).arg(data->getDataType());
        return ERROR;
    }

    QString dsname = dspath;
    if (storageType == PosCountValues){
        DataSpace filespace;
        try {
            if (currentOffset[0] >= currentDim[0]){
                currentDim[0] += sizeIncrement;
                dset.extend( currentDim );
            }
            filespace = dset.getSpace();
            filespace.selectHyperslab( H5S_SELECT_SET, chunk_dims, currentOffset );
        }
        catch (...) {
            errorString += QString("hdf5DataSet:addData: error opening dataset %1").arg(dsname);
            return ERROR;
        }

        // zero the memory buffer
        memBuffer->clear();

        if (data->getDataMod() == DMTdeviceData)
            memBuffer->setPosCount(data->getMSecsSinceStart());
        else
            memBuffer->setPosCount(data->getPositionCount());

        memBuffer->setData(data);

        try {
            dset.write ( memBuffer->getBufferStartAddr(), compoundType, memspace, filespace );
            ++currentOffset[0];
        }
        catch (...) {
            errorString += QString("hdf5DataSet:addData: error writing to dataset %1").arg(dsname);
            return ERROR;
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
            errorString += QString("hdf5DataSet:addData: posCounter must be monotonically increasing");
            return ERROR;
        }
        dsname = QString("%1/%2").arg(dspath).arg(posCounter);
        if (!dsetOpen){
            bool  datasetExists = true;
            try {
                dset = dataFile->openDataSet(qPrintable(dsname));
            }
            catch (...){
                datasetExists = false;
            }
            if (datasetExists){
                try {
                    DataSpace newdspace = dset.getSpace();
                    newdspace.getSimpleExtentDims(currentDim, NULL);
                    currentOffset[0] = currentDim[0];
                    currentOffset[1] = currentDim[1];
                }
                catch (...) {
                    errorString += QString("hdf5DataSet:addData: error creating dataset %1").arg(dsname);
                    return ERROR;
                }
                errorString += QString("successfully opened dataset %1").arg(dsname);
            }
            else {
                try {
                    currentDim[0] = arraySize;		// not constant
                    currentDim[1] = sizeIncrement; 	// default array sizeIncrement
                    currentOffset[0] = 0;
                    currentOffset[1] = 0;
                    dset = dataFile->createDataSet(qPrintable(dsname), convertToHdf5Type(dataType, longString), dspace, createProps);
                    dset.extend( currentDim );
                    dsetOpen = true;
                }
                catch (...) {
                    errorString += QString("hdf5DataSet:addData: error creating dataset %1").arg(dsname);
                    return ERROR;
                }
                errorString += QString("successfully created dataset %1").arg(dsname);
            }
        }

        try {
            DataSpace filespace = dset.getSpace();
            filespace.selectHyperslab( H5S_SELECT_SET, chunk_dims, currentOffset );

            ++currentOffset[1];
            while (currentOffset[1] > currentDim[1]){
                currentDim[1] += sizeIncrement;
                dset.extend( currentDim );
            }
            dset.write(data->getBufferAddr(), convertToHdf5Type(dataType, longString), memspace, filespace );
        }
        catch( Exception error )
        {
            errorString += QString(" hdf5DataSet:addData: Exception: %1").arg(error.getCDetailMsg());
            return ERROR;
        }
        errorString += QString("successfully wrote dataset %1").arg(dsname);
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
    }
    return status;
}

/**
 * @brief add all device infos as attributes to a given group or dataset
 * @param target HDF5 group or dataset
 */
void hdf5DataSet::addParamAttributes(H5Object *target){

    while (!params.isEmpty()){
        QStringList infoSplit = params.takeFirst().split(":");
        if (infoSplit.count() > 1){
            QString attribName = infoSplit.takeFirst();
            addDataAttribute(target, attribName, infoSplit.join(":"));
        }
    }
}

/**
 * @brief add an HDF attribute to a given group or dataset
 * @param target HDF5 group or dataset
 * @param attrName attribute name
 * @param attrVal attribute value
 */
void hdf5DataSet::addDataAttribute(H5Object *target, QString attrName, QString attrVal){

    // add attributes
    hsize_t stringDim = 1;
    if ((attrName.length() > 0) && (attrVal.length() > 0)) {
        StrType st = StrType(PredType::C_S1, attrVal.toLatin1().length());
        Attribute attrib = target->createAttribute(qPrintable(attrName), st, DataSpace(1, &stringDim));
        attrib.write(st, qPrintable(attrVal));
    }
}

/**
 *
 * @param data eveDataMessage
 * @param number compare to this
 * @return the minimum of number and used bytes in the data array
 */

/**
 * @brief create a compound type with at least one int column
 * @param names string list containing the column titels
 * @param type datatype of all columns (except the first)
 * @return compound type
 */
CompType hdf5DataSet::createDataType(QStringList names, eveType type){

    size_t offset = 0;
    DataType typeA(PredType::NATIVE_INT32);
    DataType typeB;
    int count = 0;
    size_t dtSize = typeA.getSize() + convertToHdf5Type(type, longString).getSize() *(names.size() - 1);

    CompType comptype(dtSize);
    std::string title = std::string(qPrintable(names.at(count)));
    comptype.insertMember( H5std_string(qPrintable(names.at(count))), offset, typeA);
    offset += typeA.getSize();
    ++count;

    while (count < names.size()) {

        switch (type) {
        case eveInt8T:
        case eveUInt8T:
        case eveInt16T:
        case eveUInt16T:
        case eveInt32T:
        case eveUInt32T:
        case eveFloat32T:
        case eveFloat64T:
        case eveEnum16T:
        case eveStringT:
        case eveDateTimeT:
            typeB = convertToHdf5Type(type, longString);
            break;
        default:
            return CompType(0);
        }
        comptype.insertMember( H5std_string(qPrintable(names.at(count))), offset, typeB);
        ++count;
        offset += typeB.getSize();
    }
    return comptype;
}

/**
 * @brief convert an eveType to the corresponding H5 type
 * @param type
 * @return H5 type
 */
AtomType hdf5DataSet::convertToHdf5Type(eveType type, bool longstring){
    switch (type) {
    case eveInt8T:
        return AtomType(PredType::NATIVE_INT8);
    case eveUInt8T:
        return AtomType(PredType::NATIVE_UINT8);
    case eveInt16T:
        return AtomType(PredType::NATIVE_INT16);
    case eveUInt16T:
        return AtomType(PredType::NATIVE_UINT16);
    case eveInt32T:
        return AtomType(PredType::NATIVE_INT32);
    case eveUInt32T:
        return AtomType(PredType::NATIVE_UINT32);
    case eveFloat32T:
        return AtomType(PredType::NATIVE_FLOAT);
    case eveFloat64T:
        return AtomType(PredType::NATIVE_DOUBLE);
    case eveEnum16T:
        return AtomType(StrType(PredType::C_S1, STANDARD_ENUM_STRINGSIZE+1));
    case eveStringT:
        if (longstring)
            return AtomType(StrType(PredType::C_S1, LONGSTRING_STRINGSIZE+1));
        else
            return AtomType(StrType(PredType::C_S1, STANDARD_STRINGSIZE+1));
    case eveDateTimeT:
        return AtomType(StrType(PredType::C_S1, DATETIME_STRINGSIZE+1));
    default:
        return AtomType(PredType::NATIVE_UINT32);
    }
}

