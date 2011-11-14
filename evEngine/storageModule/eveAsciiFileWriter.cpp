/*
 * eveAsciiFileWriter.cpp
 *
 *  Created on: 10.09.2009
 *      Author: eden
 */

#include "eveAsciiFileWriter.h"
#include <QTextStream>
#include <QList>

eveAsciiFileWriter::eveAsciiFileWriter() {
	initDone = false;
	fileOpen = false;
	currentIndex = -1;
	// TODO Auto-generated constructor stub

}

eveAsciiFileWriter::~eveAsciiFileWriter() {
	// TODO Auto-generated destructor stub
}

/**
 *
 * @param setID 	dataset-identification (chain-id)
 * @param filename 	Filename including absolute path
 * @param format	dataformat
 * @param parameter	Plugin-parameter from xml
 * @return			error severity
 */
int eveAsciiFileWriter::init(int setID, QString filename, QString format, QHash<QString, QString>& parameter){
	if (initDone) {
		errorString = QString("AsciiFileWriter does not support multiple Data Sets, initialize only once");
		return ERROR;
	}
	// asciiFileWriter ignores parameter
	initDone = true;
	setId = setID;
	fileName = filename;
	fileFormat = format;
	errorString.clear();
	metaData.clear();
	return SUCCESS;
}

int eveAsciiFileWriter::setXMLData(QByteArray* xmldata){
	int retval = SUCCESS;
	if (initDone) {
		QString xmlfilename = fileName + ".scml";
		QFile* xmlfilePtr = new QFile(xmlfilename);
		if (!xmlfilePtr->open(QIODevice::ReadWrite)){
			errorString = QString("AsciiFileWriter: error opening File %1").arg(xmlfilename);
			retval = ERROR;
		}
		else {
			if (xmlfilePtr->write(*xmldata) < 0){
				errorString = QString("AsciiFileWriter: error writing xmldata to File %1").arg(xmlfilename);
				retval = ERROR;
			}
			xmlfilePtr->close();
		}
	}
	if (xmldata != NULL) delete xmldata;
	return retval;
}

/**
 * @brief			create and label a column
 * @param setID 	dataset-identification (chain-id)
 * @param colid		xml-id of detectorchannel / axis
 * @param rname		name of column (name as in xml)
 * @param info		stringlist with additional info
 * @return			error severity
 */
int eveAsciiFileWriter::setCols(int setID, QString colid, QString rname, QStringList info){

	if (setId != setID) {
		errorString=QString("AsciiFileWriter does not support multiple Data Sets, give each chain a different Filename!");
		return ERROR;
	}
	if (!initDone) {
		errorString = "AsciiFileWriter not initialized";
		return ERROR;
	}

	if (!colHash.contains(colid)){
		columnInfo* colinfo = new columnInfo(colid, rname, info);
		colHash.insert(colid, colinfo);
	}
	else {
		errorString=QString("device already in column list");
		return ERROR;
	}
	errorString.clear();
	return SUCCESS;
}

/**
 * @brief			open the File (all setCol commands are done)
 * @param setID		dataset-identification (chain-id)
 * @return			error severity
 */
int eveAsciiFileWriter::open(int setID){
	if (setId != setID) {
		errorString=QString("AsciiFileWriter does not support multiple Data Sets, give each chain a different Filename!");
		return ERROR;
	}
	filePtr = new QFile(fileName);
	if (!filePtr->open(QIODevice::ReadWrite)){
		errorString = QString("AsciiFileWriter: error opening File %1").arg(fileName);
		return ERROR;
	}
	fileOpen = true;
    QTextStream out(filePtr);
    out << "Column Separator: ; \n<<<DATA>>>\n PosCnt ";
    colList = colHash.keys();
    // TODO make sure colinfo does not contain any "
    foreach (QString id, colList){
    	columnInfo* colinfo = colHash.value(id);
    	out << "; \"" << colinfo->name << " (" << colinfo->id << " / " << colinfo->info.join(", ")  << ")\"";
    	colHash.remove(id);
    	delete colinfo;
    }
    out << "\n";
    lineHash.clear();
	errorString.clear();
	return SUCCESS;
}

/**
 *
 * @param setID		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
int eveAsciiFileWriter::addData(int setID, eveDataMessage* data){
	if (setId != setID) {
		errorString=QString("AsciiFileWriter does not support multiple Data Sets, give each chain a different Filename!");
		return ERROR;
	}
	if (!fileOpen) {
		errorString = "AsciiFileWriter unable to add data, file not open";
		return ERROR;
	}
	eveVariant tempData = data->toVariant();

	if (data->getPositionCount() > currentIndex){
		currentIndex = data->getPositionCount();
		nextPosition();
	}
	else if (data->getPositionCount() < currentIndex){
		errorString = "AsciiFileWriter: invalid positionCounter";
		return ERROR;
	}
	// TODO make sure data does not contain "
	if (tempData.getType() == eveSTRING)
		lineHash.insert(data->getXmlId(), QString("\"%1\"").arg(tempData.toString()));
	else
		lineHash.insert(data->getXmlId(), data->toVariant().toString());

	errorString.clear();
	return SUCCESS;
}

/**
 * @brief	new position has been reached (increment the row-count)
 */
void eveAsciiFileWriter::nextPosition(){

    QTextStream out(filePtr);
    out << currentIndex ;
    foreach (QString id, colList){
    	if (lineHash.contains(id)){
    		out << "; " << lineHash.value(id) ;
    	}
    	else {
    		out << " ; " ;
    	}
    }
    out << "\n" ;

    lineHash.clear();
}

/**
 * @brief add metadata (e.g. comment) to be written at end of file
 */
int eveAsciiFileWriter::addMetaData(int setID, QString attribute, QString stringVal){
	if (setId != setID) {
		errorString=QString("AsciiFileWriter does not support multiple Data Sets. Each chain needs a different Filename!");
		return ERROR;
	}
	else {
		if ((attribute.length() > 0) && (stringVal.length() > 0))
			metaData.insert(attribute, stringVal);
	}
	errorString.clear();
	return SUCCESS;
}

/**
 * @brief			close File
 * @param setID		dataset-identification (chain-id)
 * @return			error severity
 */
int eveAsciiFileWriter::close(int setID) {
	if (setId != setID) {
		errorString=QString("AsciiFileWriter does not support multiple Data Sets, give each chain a different Filename!");
		return ERROR;
	}
	if (fileOpen) {
	    QTextStream out(filePtr);
		QList<QString> keyList = metaData.uniqueKeys();
		foreach(QString key, keyList){
			while (metaData.count(key) > 0)
				out << "\n" << key << ": " << metaData.take(key) << "\n";
		}
		filePtr->close();
		fileOpen = false;
	}
	errorString.clear();
	return SUCCESS;
}

eveAsciiFileWriter::columnInfo::columnInfo(QString colid, QString colname, QStringList colinfo){
	id = colid;
	name = colname;
	info = colinfo;
}

