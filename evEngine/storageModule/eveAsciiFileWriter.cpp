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
	// TODO Auto-generated constructor stub

}

eveAsciiFileWriter::~eveAsciiFileWriter() {
	// TODO Auto-generated destructor stub
}

/**
 *
 * @param filename 	Filename including absolute path
 * @param format	dataformat
 * @param parameter	Plugin-parameter from xml
 * @return			error severity
 */
int eveAsciiFileWriter::init(QString filename, QString format, QHash<QString, QString>& parameter){
	if (initDone) {
		errorString = QString("AsciiFileWriter has already been initialized");
		return MINOR;
	}
	// asciiFileWriter ignores parameter
	initDone = true;
	// TODO
	//	setId = setID;
	fileName = filename;
	fileFormat = format;
	errorString.clear();
	return DEBUG;
}

int eveAsciiFileWriter::setXMLData(QByteArray* xmldata){
	int retval = DEBUG;
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
int eveAsciiFileWriter::addColumn(eveDevInfoMessage* message){

	if (!initDone) {
		errorString = "AsciiFileWriter not initialized";
		return ERROR;
	}
	errorString.clear();
	if (!fileOpen) open();
	if (!fileOpen) {
		errorString += " unable to open file";
		return ERROR;
	}

    QTextStream out(filePtr);
    out << "Device Information; ";
	out << "ChainId: " << message->getChainId() << "; XML-ID: " << message->getXmlId() << "; Name: " << message->getName() << "; ";
    foreach (QString info, *(message->getText())) out << info << " ";
    out << "\n";

	return DEBUG;
}

/**
 * @brief			open the File (all setCol commands are done)
 * @param setID		dataset-identification (chain-id)
 * @return			error severity
 */
int eveAsciiFileWriter::open(){

	if (!initDone) {
		errorString = "AsciiFileWriter not initialized";
		return ERROR;
	}
	if (fileOpen){
		errorString = QString("AsciiFileWriter: File %1 has already been opened").arg(fileName);
		return DEBUG;
	}
	filePtr = new QFile(fileName);
	if (!filePtr->open(QIODevice::ReadWrite)){
		errorString = QString("AsciiFileWriter: error opening File %1").arg(fileName);
		return ERROR;
	}
	fileOpen = true;
    QTextStream out(filePtr);
    out << "Column Separator: \";\" \n";
	return DEBUG;
}

/**
 *
 * @param setID		dataset-identification (chain-id)
 * @param data		data to write to file
 * @return			error severity
 */
int eveAsciiFileWriter::addData(int setID, eveDataMessage* data){

	if (!initDone) {
		errorString = "AsciiFileWriter not initialized";
		return ERROR;
	}
	errorString.clear();
	if (!fileOpen) open();
	if (!fileOpen) {
		errorString += " unable to open file";
		return ERROR;
	}

	eveVariant tempData = data->toVariant();

    QTextStream out(filePtr);
    out << "Data; ";
	out << "ChainId: " << data->getChainId() << "; XML-ID: " << data->getXmlId() << "; ";

	if (data->getChainId() == 0)
		out << "MSecsSinceStart: " << data->getMSecsSinceStart() << "; ";
	else
		out << "PositionCount: " << data->getPositionCount() << "; ";

	// TODO array output missing
	if (tempData.getType() == eveSTRING)
		out << QString("\"%1\"").arg(tempData.toString()) << "\n";
	else
		out << tempData.toString() << "\n";

	return DEBUG;
}

/**
 * @brief add metadata (e.g. comment) to be written at end of file
 */
int eveAsciiFileWriter::addMetaData(int setID, QString attribute, QString stringVal){

	if (!initDone) {
		errorString = "AsciiFileWriter not initialized";
		return ERROR;
	}
	errorString.clear();
	if (!fileOpen) open();
	if (!fileOpen) {
		errorString += " unable to open file";
		return ERROR;
	}

    QTextStream out(filePtr);
    out << "Metadata; ";
	out << "ChainId: " << setID << "; ";
	out << attribute << ":" << stringVal << "\n";

	return DEBUG;
}

/**
 * @brief			close File
 * @param setID		dataset-identification (chain-id)
 * @return			error severity
 */
int eveAsciiFileWriter::close() {

	errorString.clear();
	if (fileOpen) {
		filePtr->close();
		fileOpen = false;
		errorString = QString("AsciiFileWriter: closed File %1").arg(fileName);
	}
	return DEBUG;
}

