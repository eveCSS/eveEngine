/*
 * eveDataCollector.cpp
 *
 *  Created on: 03.09.2009
 *      Author: eden
 */

#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QPluginLoader>
#include "eveDataCollector.h"
#include "eveStorageManager.h"
#include "eveAsciiFileWriter.h"
#include "eveFileTest.h"


/**
 * @param sman corresponding Storge Manager
 * @param message configuration info
 */
eveDataCollector::eveDataCollector(eveStorageManager* sman, eveStorageMessage* message) {

	QHash<QString, QString>* paraHash = message->takeHash();
	QString emptyString;
	chainId = message->getChainId();
	fileName = message->getFileName();
	fileType = paraHash->value("saveformat", emptyString);
	pluginName = paraHash->value("pluginname", emptyString);
	pluginPath = paraHash->value("location", emptyString);
	manager = sman;
	fileWriter = NULL;
	fwInitDone = false;
	fwOpenDone = false;
	bool fileTest = false;

	if (fileName.isEmpty()){
		sman->sendError(ERROR,0,QString("eveDataCollector: empty filename not allowed, using dummy"));
		fileName = "dummy-filename";
	}
	// if filename is relative and EVE_ROOT is defined, we use EVE_ROOT as root directory
	if (QFileInfo(fileName).isRelative()){
		QString eve_root;
		QStringList envList = QProcess::systemEnvironment();
		foreach (QString env, envList){
			if (env.startsWith("EVE_ROOT")){
				QStringList pieces = env.split("=");
				if (pieces.count() == 2) eve_root = pieces.at(1);
				break;
			}
		}
		if (!eve_root.isEmpty()){
			QFileInfo eve_rootInfo = QFileInfo(eve_root);
			if (!eve_rootInfo.completeBaseName().isEmpty()) eve_root += "/";
			fileName = QFileInfo(eve_root + fileName).absoluteFilePath();
		}
	}
	QString numberedName = fileName;
	if (paraHash->value("autonumber", emptyString).toLower() == "true") numberedName = eveFileTest::addNumber(fileName);
	if (numberedName.isEmpty()){
		sman->sendError(ERROR,0,QString("eveDataCollector: unable to add number to filename %1 ").arg(message->getFileName()));
	}
	else {
		fileName = numberedName;
	}


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
	// predefined info message with filename
	if (fileTest) sman->sendError(INFO, 0x000a, fileName);

	if (pluginName.isEmpty()){
		if (fileType == "ASCII"){
			fileWriter = new eveAsciiFileWriter();
		}
		else if (fileType == "HDF5") {
			// TODO
			sman->sendError(MINOR, 0, "eveDataCollector::configStorage: please configure hdf5plugin for File Type HDF5");
			if (pluginName.isEmpty()) pluginName = "hdf5plugin";
		}
		else if (fileType == "HDF4"){
			// TODO
			sman->sendError(ERROR,0,QString("eveDataCollector::configStorage: use plugin for File Type %1").arg(fileType));
		}
		else {
			sman->sendError(ERROR,0,QString("eveDataCollector::configStorage: unknown file type: %1").arg(fileType));
		}
	}

	if (!pluginName.isEmpty()){

#if defined(Q_OS_WIN)
		pluginName.append(".dll");
#elif defined(Q_OS_MAC)
		// TODO what is the MacOS extension?
		pluginName = "lib" + pluginName + ".so";
#else
		if (!pluginName.endsWith(".so"))
				pluginName += ".so";
		if (!pluginName.startsWith("lib"))
			pluginName = "lib" + pluginName;
#endif

		QString fullname;
		QString libname = pluginPath;;
		// if location ends with "/", we append the lowercase plugin-name extended
		// with os-specific library-extension to the path
		// if location does not end with "/", we assume it is the fully-qualified
		// library name (which still may be a relative path)
		if (pluginPath.endsWith("/")) {
			libname = pluginPath+pluginName;
		}
		else {
			libname = pluginPath;
		}
		QFileInfo info(libname);
		if (info.isAbsolute()){
		    if (info.exists())
		    	fullname = info.absoluteFilePath();
		}
		else {
		    info.setFile(QDir::currentPath() + libname);
		    if (info.exists())
		    	fullname = info.absoluteFilePath();
		    else {
		    	QString pluginpath;
		    	QStringList envList = QProcess::systemEnvironment();
		    	foreach (QString env, envList){
		    		if (env.startsWith("EVE_PLUGINPATH")){
		    			QStringList pieces = env.split("=");
		    			if (pieces.count() == 2) pluginpath = pieces.at(1);
		    			break;
		    		}
		    	}
		    	// check if we have a search list
		    	QStringList pathdirs = pluginpath.split(":");
		    	foreach (QString path, pathdirs){
		    		info.setFile(path + libname);
		    		if (info.exists()) {
		    			fullname = info.absoluteFilePath();
						break;
		    		}
		    	}
		    }
		}
		if (fullname.isEmpty()){
			sman->sendError(INFO, 0, QString("eveDataCollector: unable to resolve plugin path, using system defaults").arg(libname));
			fullname = libname;
		}
		sman->sendError(DEBUG, 0, QString("eveDataCollector: loading plugin %1").arg(fullname));
		QPluginLoader pluginLoader(fullname);
		    QObject *plugin = pluginLoader.instance();
		    if (plugin) {
		    	fileWriter = qobject_cast<eveFileWriter *>(plugin);
		    	if (!fileWriter)
					sman->sendError(ERROR, 0, QString("eveDataCollector: Cast Error while loading plugin %1").arg(fullname));
		    	else
					sman->sendError(DEBUG, 0, QString("eveDataCollector: successfully loaded plugin %1").arg(fullname));
		    }
		    else {
				sman->sendError(ERROR, 0, QString("eveDataCollector: Load Error while loading plugin %1").arg(fullname));
		    }
	}

	if (fileWriter){
		if (fileWriter->init(chainId, fileName, fileType, paraHash) != SUCCESS){
			manager->sendError(ERROR, 0, QString("FileWriter: init error: %1").arg(fileWriter->errorText()));
		}
		else {
			if (fileTest) fwInitDone = true;
		}
	}
	delete paraHash;
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
 * @brief get xml-id and send data to corresponding fileWriter
 * @param message data to store
 */
void eveDataCollector::addData(eveDataMessage* message) {

	if (fwInitDone && deviceList.contains(message->getXmlId())) {
		if (!fwOpenDone){
			if (fileWriter->open(chainId) != SUCCESS){
				manager->sendError(ERROR, 0, QString("FileWriter: open error: %1").arg(fileWriter->errorText()));
			}
			else {
				manager->sendError(DEBUG, 0, QString("FileWriter: successfully opened file"));
				fwOpenDone = true;
			}
		}
		if (fwOpenDone){
			fileWriter->addData(chainId, message);
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
			manager->sendError(INFO, 0, QString("DataCollector: already received device info for %1").arg(message->getXmlId()));
		else {
			deviceList.append(message->getXmlId());
			manager->sendError(DEBUG, 0, QString("DataCollector: setting device info for %1").arg(message->getXmlId()));
			if (fileWriter->setCols(chainId,message->getXmlId(), message->getName(), *(message->getText())) != SUCCESS) {
				manager->sendError(ERROR, 0, QString("FileWriter Error: %1").arg(fileWriter->errorText()));
			}
		}
	}
	else
		manager->sendError(ERROR, 0, "DataCollector: can't add devices, file initialization unsuccessful or file already opened");
}
