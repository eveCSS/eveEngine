/*
 * eveDataCollector.cpp
 *
 *  Created on: 03.09.2009
 *      Author: eden
 */

#include "eveDataCollector.h"
#include "eveStorageManager.h"
#include "eveAsciiFileWriter.h"
#include "eveFileTest.h"


/**
 * @param sman corresponding Storge Manager
 * @param message configuration info
 */
eveDataCollector::eveDataCollector(eveStorageManager* sman, eveStorageMessage* message) {

	chainId = message->getChainId();
	fileType = message->getFileType();
	pluginName = message->getPluginName();
	pluginPath = message->getPluginPath();
	manager = sman;
	fileWriter = NULL;
	fwInitDone = false;
	fwOpenDone = false;
	bool fileTest = false;

	//get enhanced filename
	fileName = eveFileTest::addNumber(message->getFileName());
	if (fileName.isEmpty()){
		sman->sendError(ERROR,0,QString("eveDataCollector: unable to add number to filename %1 ").arg(message->getFileName()));
	}
	else {
		switch (eveFileTest::createTestfile(fileName)){
		case 1:
			sman->sendError(ERROR, 0, QString("eveDataCollector: unable to use filename %1, file exists").arg(fileName));
			break;
		case 2:
			sman->sendError(ERROR, 0, QString("eveDataCollector: unable to use file %1, directory does not exist").arg(fileName));
			break;
		case 3:
			sman->sendError(ERROR, 0, QString("eveDataCollector: unable to create file %1").arg(fileName));
			break;
		case 4:
			sman->sendError(INFO, 0, QString("eveDataCollector: fileTest: unable to remove file %1").arg(fileName));
			fileTest = true;
			break;
		default:
			fileTest = true;
			break;
		}
	}

	// the following is obsolete?
	QString fileType = message->getFileType();
	if (fileType == "ASCII"){
		fileWriter = new eveAsciiFileWriter();
		QHash<QString, QString>* infoHash = new QHash<QString, QString>;
		infoHash->insert("paranam", "paraval");
		if (fileWriter->init(chainId, fileName, fileType, infoHash) != SUCCESS){
			manager->sendError(ERROR, 0, QString("FileWriter: init error: %1").arg(fileWriter->errorText()));
		}
		else {
			if (fileTest) fwInitDone = true;
		}
	}
	else if (fileType == "HDF5") {
		// TODO
		sman->sendError(ERROR,0,QString("eveStorageManager::configStorage: File Type %1 is not yet implemented").arg(fileType));
	}
	else if (fileType == "HDF4"){
		// TODO
		sman->sendError(ERROR,0,QString("eveStorageManager::configStorage: File Type %1 is not yet implemented").arg(fileType));
	}
	else {
		sman->sendError(ERROR,0,QString("eveStorageManager::configStorage: unknown file type: %1").arg(fileType));
	}
}

eveDataCollector::~eveDataCollector() {
	if (fwInitDone) {
		if (fileWriter->close(chainId) != SUCCESS){
			manager->sendError(ERROR, 0, QString("FileWriter: close error: %1").arg(fileWriter->errorText()));
		}
		delete fileWriter;
	}
	deviceList.clear();
}

/**
 * @brief get xml-id and send data to corresponding devicedatacollector
 * @param message data to store
 */
void eveDataCollector::addData(eveDataMessage* message) {

	if (fwInitDone && deviceList.contains(message->getXmlId())) {
		if (fwOpenDone){
			fileWriter->addData(chainId, message);
		}
		else {
			if (fileWriter->open(chainId) != SUCCESS){
				manager->sendError(ERROR, 0, QString("FileWriter: open error: %1").arg(fileWriter->errorText()));
			}
			else {
				manager->sendError(DEBUG, 0, QString("FileWriter: successfully opened file"));
			}
			fwOpenDone = true;
		}
	}
	else {
		manager->sendError(ERROR, 0, QString("DataCollector: cannot add data for %1 (%2/%3), no device info")
				.arg(message->getXmlId())
				.arg(message->getName())
				.arg(message->getId())
				);
	}
}

/**
 * @brief add a new device
 * @param message device information
 */
void eveDataCollector::addDevice(eveDevInfoMessage* message) {

	if (fwInitDone && !fwOpenDone){
		if (deviceList.contains(message->getXmlId()))
			manager->sendError(ERROR, 0, QString("DataCollector: already received device info for %1").arg(message->getXmlId()));
		else {
			deviceList.append(message->getXmlId());
			manager->sendError(DEBUG, 0, QString("DataCollector: setting device info for %1").arg(message->getXmlId()));
			if (fileWriter->setCols(chainId,message->getXmlId(), message->getName(), *(message->getText())) != SUCCESS) {
				manager->sendError(ERROR, 0, QString("FileWriter Error: %1").arg(fileWriter->errorText()));
			}
		}
	}
	else
		manager->sendError(ERROR, 0, "DataCollector: can't add devices, file already opened or not initialized");
}
