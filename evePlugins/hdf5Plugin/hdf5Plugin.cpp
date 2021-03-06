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
    fileName = "";
    defaultSizeIncrement = 50; // default file size increment
    isFileOpen = false;
    dataFile = NULL;
    groupList.append("/");		// HDF root group always exists
    modificationHash.insert(DMTnormalized,"normalized");
    modificationHash.insert(DMTcenter,"center");
    modificationHash.insert(DMTedge,"edge");
    modificationHash.insert(DMTmin,"minimum");
    modificationHash.insert(DMTmax,"maximum");
    modificationHash.insert(DMTfwhm,"fwhm");
    modificationHash.insert(DMTmean,"mean");
    modificationHash.insert(DMTstandarddev,"standarddev");
    modificationHash.insert(DMTsum,"sum");
    modificationHash.insert(DMTpeak,"peak");
    modificationHash.insert(DMTdeviceData,"device");
    modificationHash.insert(DMTmetaData,"meta");
    modificationHash.insert(DMTaverageParams,"averagemeta");
    modificationHash.insert(DMTunknown,"unknown");
}

hdf5Plugin::~hdf5Plugin() {

    if (isFileOpen){
        try {
            dataFile->close();
        }
        catch( Exception error )
        {

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
    errorString = QString("HDF5Plugin: file %1 has been opened").arg(fileName);
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
        errorString = QString("HDF5Plugin: unable to handle any other format than HDF5; not: %1").arg(format);
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
    if (isFileOpen) addMetaData(0, "EVEH5Version", EVEH5_VERSION);
    return DEBUG;
}

/**
 * @brief hdf5Plugin::setXMLData stores the xml data in a separate file with the same file name plus extension ".scml"
 * @param xmldata
 * @return error severity
 */
int hdf5Plugin::setXMLData(QByteArray* xmldata){
    int retval = DEBUG;
    if (isFileOpen) {
        QString xmlfilename(fileName);
        if (xmlfilename.endsWith(".h5",Qt::CaseInsensitive)) xmlfilename.chop(3);
        xmlfilename.append(".scml");
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
 * @brief hdf5Plugin::addColumn add a device to the data file
 * @param message device info
 * @return error severity
 */
int hdf5Plugin::addColumn(eveDevInfoMessage* message){

    if (message->getStorageHint().toLower() == "none") {
        errorString = QString("HDF5Plugin::addColumn: skip data with storage \"none\"");
        return DEBUG;
    }

    QString dsname = getDSName(message->getChainId(), message->getXmlId(), message->getStorageHint(), message->getDataMod(), message->getAuxString(), message->getNormalizeId());

    if (!isFileOpen) {
        errorString = QString("HDF5Plugin::addColumn: data file has not been opened");
        return ERROR;
    }

    if (dsNameHash.contains(dsname)){
        errorString = QString("HDF5Plugin::addColumn Data Set with name %1, is already initialized").arg(dsname);
        return INFO;
    }
    createGroup(dsname);

    hdf5DataSet *ds = new hdf5DataSet(dsname, message->getXmlId(), message->getName(), *(message->getText()), dataFile);
    ds->setSizeIncrement(defaultSizeIncrement);
    dsNameHash.insert(dsname, ds);
    errorString = QString("HDF5Plugin::addColumn Successfully added device info for %1").arg(dsname);
    return DEBUG;
}

/**
 * \brief create the datasets, since we now know the datatype
 *
 * @param pathId	dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
int hdf5Plugin::addData(int pathId, eveDataMessage* data)
{
    int status;
    hdf5DataSet *ds;

    if (data->getStorageHint().toLower() == "none") {
        errorString = QString("HDF5Plugin::addColumn: skip data with storage \"none\"");
        return DEBUG;
    }
    if (!isFileOpen) {
        errorString = "HDF5Plugin:addData: data file has not been opened";
        return ERROR;
    }

    QString dsname = getDSName(pathId, data->getXmlId(), data->getStorageHint(), data->getDataMod(), data->getAuxString(), data->getNormalizeId());
    errorString = QString("HDF5Plugin: add Data to %1").arg(dsname);

    if (dsNameHash.contains(dsname)) {
        ds = dsNameHash.value(dsname);
    }
    else {
        ds = new hdf5DataSet(dsname, data->getXmlId(), data->getName(), data->getAttributes(), dataFile);
        dsNameHash.insert(dsname, ds);
        ds->setSizeIncrement(defaultSizeIncrement);
        createGroup(dsname);
    }
    status = ds->addData(data);
    errorString += ds->getError();
    return status;
}

/**
 * @brief hdf5Plugin::addMetaData add metadata (e.g. comment) as attribute to a HDF group
 * @param pathId    chain id or zero
 * @param attribute attribut name
 * @param stringVal attribut value
 * @return error severity
 */
int hdf5Plugin::addMetaData(int pathId, QString attribute, QString stringVal){
    if (!isFileOpen) {
        errorString = QString("HDF5Plugin:addData: data file has not been opened");
        return ERROR;
    }
    errorString.clear();

    try {
        hsize_t stringDim = 1;
        QString groupname = getGroupName(pathId);

        Group targetGroup = dataFile->openGroup(qPrintable(createGroup(groupname)));
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
            attrib.close();
            targetGroup.removeAttr(qPrintable(attribute));
            newVal = QString("%1; %2").arg(QString::fromLatin1(strg_C)).arg(stringVal);
            delete []strg_C;
        }
        StrType st = StrType(PredType::C_S1, newVal.toLatin1().length());
        // UTF8 funktioniert leider nicht (st.setCset(H5T_CSET_UTF8);)
        Attribute attrib = targetGroup.createAttribute(qPrintable(attribute), st, DataSpace(1, &stringDim));
        attrib.write(st, newVal.toLatin1().data());
        errorString += QString("new Attribute: %1 (%2) path: %3").arg(attribute).arg(newVal).arg(groupname);
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
 * @brief			flush data to disk
 * @return			error severity
 */
int hdf5Plugin::flush()
{
    if (isFileOpen) {
        dataFile->flush(H5F_SCOPE_GLOBAL);
        errorString = QString("HDF5Plugin::flush: flushed %1").arg(fileName);
    }
    return DEBUG;
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


/**
 * @brief hdf5Plugin::getDSName create a dataset name from various data parameters
 * @param id    chain id or 0
 * @param name  xml_id
 * @param storage   default or alternate storage location
 * @param modified  raw or modified data
 * @param other     aux info
 * @param normalizeId id of normalizce channel if present
 * @return full qualified name of dataset
 */
QString hdf5Plugin::getDSName(int id, QString name, QString storage, eveDataModType modified, QString other, QString normalizeId){

    QString groupname = getGroupName(id);
    QString newname = name;

    if ((id != 0) && (storage.length() > 0)) groupname += QString("%1/").arg(storage);

    if (modified != DMTunmodified){
        groupname += QString("%1/").arg(modificationHash.value(modified));
        if ((modified == DMTnormalized) && !normalizeId.isEmpty())
            newname = QString("%1__%2").arg(newname).arg(normalizeId);
        if (!other.isEmpty())
            newname = QString("%1__%2").arg(newname).arg(other);
    }
    return groupname.append(newname);
}

/**
 * @brief hdf5Plugin::createGroup create all HDF groups mentioned in name
 * @param name desired dataset name, all characters after the last "/" will be removed
 * @return full path i.e. all group names concatenated with "/"
 */
QString hdf5Plugin::createGroup( QString name ){

    QString groupname = "/";
    int charposition = name.lastIndexOf(QChar('/'));
    if (charposition >= 0)name.remove(charposition,name.length());
    if (!groupList.contains(name + "/")){
        foreach(QString part, name.split(QChar('/'), QString::SkipEmptyParts)){
            groupname += part + "/";
            if (!groupList.contains(groupname) && isFileOpen) {
                try
                {
                    dataFile->createGroup(qPrintable(groupname));
                    groupList.append(groupname);
                }
                catch( Exception error )
                {
                    errorString += QString("createGroup: %1: %2").arg(groupname).arg(error.getCDetailMsg());
                }
            }
        }
    }
    else {
        groupname = name + "/";
    }
    return groupname;
}

/**
 * @brief hdf5Plugin::getGroupName returns path for path id or root group
 * @param pathId chain id or zero
 * @return full path for pathId
 */
QString hdf5Plugin::getGroupName(int pathId){
    if (pathId == 0)
        return QString("/");
    else
        return QString("/c%1/").arg(pathId);
}

Q_EXPORT_PLUGIN2(hdf5plugin, hdf5Plugin);
