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
#include "eveParameter.h"
#include "eveDataCollector.h"
#include "eveStorageManager.h"
#include "eveAsciiFileWriter.h"
#include "eveFileTest.h"
#include "eveSimplePV.h"


/**
 * @param sman corresponding Storage Manager
 * @param message configuration info
 * @param xmldata pointer to xml buffer with scandescription
 */
eveDataCollector::eveDataCollector(eveStorageManager* sman, QHash<QString, QString>& paraHash, QByteArray* xmldata) {

	QString emptyString("");
	chainIdList.empty();
	fileName = paraHash.value("filename", emptyString);
	fileType = paraHash.value("format", emptyString);
	QString suffix = paraHash.value("suffix", emptyString);
	pluginName = paraHash.value("pluginname", emptyString);
	pluginPath = paraHash.value("location", emptyString);
	comment = paraHash.value("comment", emptyString);
	bool saveXML = paraHash.value("savescandescription", emptyString).startsWith("true");
	manager = sman;
	fileWriter = NULL;
	fwInitDone = false;
	fwOpenDone = false;
	StartTimeDone = false;
	StartDateDone = false;
	bool fileTest = false;
	bool doAutoNumber = false;

	if (paraHash.value("autonumber", emptyString).toLower() == "true") doAutoNumber = true;;

	if (fileName.isEmpty()){
		sman->sendError(ERROR,0,QString("eveDataCollector: empty filename not allowed, using dummy-filename"));
		fileName = "dummy-filename";
	}
	// do macro expansion if necessary
	if (fileName.contains("${")) fileName = macroExpand(fileName);
	if (!suffix.isEmpty() && (!fileName.endsWith(suffix))) {
		if (!(fileName.endsWith(".") || suffix.startsWith("."))) fileName.append(".");
		fileName.append(suffix);
	}

	// if filename is relative and EVE_ROOT is defined, we use EVE_ROOT as root directory
	if (QFileInfo(fileName).isRelative()){
		QString eve_root = eveParameter::getParameter("eveRoot");
		if (!eve_root.isEmpty()){
			QFileInfo eve_rootInfo = QFileInfo(eve_root);
			// note: the "/" will be translated to "\" on windows
			if (!eve_rootInfo.completeBaseName().isEmpty()) eve_root += "/";
			fileName = QFileInfo(eve_root + fileName).absoluteFilePath();
		}
	}

	//create directory hierarchy if it doesn't exist
	QDir absDir = QFileInfo(fileName).absoluteDir();
	if (!absDir.exists()) {
		if (!absDir.mkpath(absDir.absolutePath())) sman->sendError(MINOR, 0, QString("(DataCollector) unable to create path %1").arg(absDir.absolutePath()));
	}

	if (!doAutoNumber && (eveFileTest::createTestfile(fileName) == 1)){
		doAutoNumber = true;
		sman->sendError(MINOR, 0, QString("(DataCollector) file %1 already exists, enabling autonumber").arg(fileName));
	}

	QString numberedName = fileName;
	if (doAutoNumber) numberedName = eveFileTest::addNumber(fileName);
	if (numberedName.isEmpty()){
		sman->sendError(ERROR,0,QString("eveDataCollector: unable to add number to filename %1 ").arg(fileName));
	}
	else {
		fileName = numberedName;
	}

	switch (eveFileTest::createTestfile(fileName)){
	case 1:
		sman->sendError(ERROR, 0, QString("(DataCollector) unable to use filename %1, file exists").arg(fileName));
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
	if (fileTest) sman->sendError(EVEMESSAGESEVERITY_SYSTEM, EVEERRORMESSAGETYPE_FILENAME, fileName);

	if (pluginName.isEmpty() || (pluginName == "ASCII")){
		if (fileType.isEmpty() || (fileType == "ASCII")){
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
	else {

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
		    			pluginpath = env.remove("EVE_PLUGINPATH=");
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
		    	else {
					if (fileWriter->getVersionString() == eveParameter::getParameter("savepluginversion"))
						sman->sendError(DEBUG, 0, QString("eveDataCollector: successfully loaded plugin %1").arg(fullname));
					else {
						sman->sendError(ERROR, 0, QString("eveDataCollector: plugin %1 version error, version is %2 should be %3").arg(
								fullname).arg(fileWriter->getVersionString()).arg(eveParameter::getParameter("savepluginversion")));
						fileWriter = NULL;
					}

		    	}
		    }
		    else {
				sman->sendError(FATAL, 0, QString("eveDataCollector: Load Error while loading plugin %1").arg(fullname));
		    }
	}

	if (fileWriter){
		int status = fileWriter->init(fileName, fileType, paraHash);
		if ((status == ERROR)||(status == FATAL)){
			manager->sendError(status, 0, QString("FileWriter: init error: %1").arg(fileWriter->errorText()));
		}
		else {
			if (status == MINOR)manager->sendError(status, 0, QString("FileWriter: init warning: %1").arg(fileWriter->errorText()));
			if (fileTest) {
				fwInitDone = true;
			}
		}
	}

	if (fwInitDone && !comment.isEmpty()) addMetaData(0, "Comment", comment);

	if (fwInitDone && xmldata && saveXML) fileWriter->setXMLData(xmldata);

}

eveDataCollector::~eveDataCollector() {
	if (fileWriter){
		int status = fileWriter->close();
		manager->sendError(status, 0, QString("FileWriter: close message: %1").arg(fileWriter->errorText()));
		delete fileWriter;
	}
	deviceList.clear();
}

/**
 * @brief get xml-id and send data to corresponding fileWriter
 * @param message data to store
 */
void eveDataCollector::addData(eveDataMessage* message) {

	if (fwInitDone && ((message->getChainId() == 0 ) || deviceList.contains(message->getXmlId()))) {
		if (!fwOpenDone){
			int status = fileWriter->open();
			if (status > ERROR) fwOpenDone = true;
			manager->sendError(status, 0, QString("FileWriter: openFile: %1").arg(fileWriter->errorText()));
		}
		if (fwOpenDone){
			int status = fileWriter->addData(message->getChainId(), message);
			manager->sendError(status, 0, QString("FileWriter: addData: %1").arg(fileWriter->errorText()));
		}
	}
	else {
		manager->sendError(ERROR, 0, QString("DataCollector: cannot add data for %1 (%2), no device info")
				.arg(message->getXmlId())
				.arg(message->getName())
				);
	}
}

/**
 * @brief add metadata like comment to filewriter
 * @param Id       chainId if Id > 0, for all files if Id == 0
 * @param message comment text
 * @param message comment text
 */
void eveDataCollector::addMetaData(int Id, QString attribute, QString& messageText) {

	if (fwInitDone){
		int status = fileWriter->addMetaData(Id, attribute, messageText);
		manager->sendError(status, 0, QString("addMetaData: %1").arg(fileWriter->errorText()));
		if ((Id > 0) && (attribute=="StartTime") && !StartTimeDone) {
			fileWriter->addMetaData(0, attribute, messageText);
			StartTimeDone = true;
		}
		else if ((Id > 0) && (attribute=="StartDate") && !StartDateDone) {
			fileWriter->addMetaData(0, attribute, messageText);
			StartDateDone = true;
		}
	}
	else {
		manager->sendError(ERROR, 0, QString("DataCollector: cannot add metadata before init: %1").arg(messageText));
	}
}

/**
 * @brief add a new device
 * @param message device information
 */
void eveDataCollector::addDevice(eveDevInfoMessage* message) {

	if (fwInitDone){
		if (deviceList.contains(message->getXmlId()))
			manager->sendError(DEBUG, 0, QString("DataCollector: already received device info for %1").arg(message->getXmlId()));
		else {
			deviceList.append(message->getXmlId());
			manager->sendError(DEBUG, 0, QString("DataCollector: setting device info for %1").arg(message->getXmlId()));
			int status = fileWriter->addColumn(message);
			manager->sendError(status, 0, QString("addDevice: %1").arg(fileWriter->errorText()));
		}
	}
	else
		manager->sendError(ERROR, 0, "DataCollector: can't add devices, file initialization unsuccessful or file already opened");
}

void eveDataCollector::addChain(eveStorageMessage* message){
	int chainId = message->getChainId();
	if (!chainIdList.contains(chainId)) chainIdList.append(chainId);
}

/**
 * @brief	expand predefined macros
 * @eString	string containing macros
 * @return expanded string
 */
QString eveDataCollector::macroExpand(QString eString){

	QDateTime now=QDateTime::currentDateTime();

	if (eString.contains("${WEEK}")){
		eString.replace(QString("${WEEK}"), QString("%1").arg(now.date().weekNumber()));
	}
	if (eString.contains("${DATE}")){
		eString.replace(QString("${DATE}"), QString("%1").arg(now.toString("yyyyMMdd")));
	}
	if (eString.contains("${DATE-}")){
		eString.replace(QString("${DATE-}"), QString("%1").arg(now.toString("yyyy-MM-dd")));
	}
	if (eString.contains("${TIME}")){
		eString.replace(QString("${TIME}"), QString("%1").arg(now.toString("hhmmss")));
	}
	if (eString.contains("${TIME-}")){
		eString.replace(QString("${TIME-}"), QString("%1").arg(now.toString("hh-mm-ss")));
	}
	while (eString.contains(QRegExp("\\$\\{PV:[^}]*\\}"))) {

		QRegExp pvPattern("\\$\\{PV:([^}]*)\\}");
		QString replaceText("__PV-replacing-error__");
		int pos = pvPattern.indexIn(eString);
		if (pos > -1) {
			manager->sendError(MINOR, 0, QString("PV macro expansion is an experimental feature and has an impact on system stability "));
			manager->sendError(DEBUG, 0, QString("PV macro expansion: pv >%1<").arg(pvPattern.cap(1)));
			eveSimplePV* macroPV = new eveSimplePV(pvPattern.cap(1));
			if (macroPV->readPV() == SCSSUCCESS){
				replaceText = macroPV->getStringValue();
				manager->sendError(DEBUG, 0, QString("PV macro expansion: new value >%1<").arg(replaceText));
			}
			else {
				manager->sendError(MINOR, 0, QString("PV macro expansion: pv error %1").arg(macroPV->getErrorString()));
			}
			delete macroPV;
			manager->sendError(DEBUG, 0, QString("resuming after macro expansion"));
		}
		eString.replace(QString("${PV:%1}").arg(pvPattern.cap(1)), replaceText);
	}
	if (eString.contains("${")) {
		eString.replace(QRegExp("\\$\\{[^}]*\\}"), "__unknownMacro__");
		manager->sendError(MINOR, 0, "DataCollector: unknown filename macro");
	}
	return eString;
}

