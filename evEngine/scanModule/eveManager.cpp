/*
 * eveManager.cpp
 *
 *  Created on: 08.09.2008
 *      Author: eden
 */

#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QHash>
#include "eveManager.h"
#include "eveRequestManager.h"
#include "eveMessageHub.h"
#include "eveError.h"
#include "eveDevice.h"
#include "eveXMLReader.h"
#include "eveScanManager.h"
#include "eveScanThread.h"
#include "eveStorageThread.h"
#include "eveMathThread.h"
#include "eveParameter.h"
#include "eveMonitorRegisterMessage.h"
#include "eveStartTime.h"

/**
 * \brief init and register with messageHub
 */
eveManager::eveManager() {
	currentPlEntry = NULL;
	playlist = new evePlayListManager();
	eveMessageHub * mHub = eveMessageHub::getmHub();
	channelId = mHub->registerChannel(this, EVECHANNEL_MANAGER);
	engineStatus = new eveManagerStatusTracker();
	shutdownPending = false;
	// if we have a start file we do batch processing and stop engine if status is idle
	if (!eveParameter::getParameter("startFile").isEmpty()){
		connect(engineStatus, SIGNAL(engineIdle()), mHub, SLOT(close()), Qt::QueuedConnection);
	}
}

eveManager::~eveManager() {
	delete playlist;
	delete engineStatus;
	if (currentPlEntry != NULL){
		delete currentPlEntry;
		currentPlEntry = NULL;
	}
	// we already unregistered with mhub
}

/**
 * \brief process messages sent to eveManager
 */
void eveManager::handleMessage(eveMessage *message){

	eveError::log(DEBUG, "eveManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_ADDTOPLAYLIST:
			playlist->addEntry(((eveAddToPlMessage*)message)->getXmlName(), ((eveAddToPlMessage*)message)->getXmlAuthor(), ((eveAddToPlMessage*)message)->getXmlData());
			// answer with the current playlist
			addMessage(playlist->getCurrentPlayList());
			// load the next playlist entry, if Engine is idle and no XML is loaded
			if (engineStatus->isNoXmlLoaded())
				if (loadPlayListEntry()){
					if (engineStatus->isAutoStart())
						if (!sendStart())
							sendError(INFO,0,"cannot process START command with current engine status");
				}
			break;
		case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
			playlist->removeEntry( ((eveMessageInt*)message)->getInt() );
			// answer with the current playlist
			addMessage(playlist->getCurrentPlayList());
			break;
		case EVEMESSAGETYPE_REORDERPLAYLIST:
			playlist->reorderEntry( ((eveMessageIntList*)message)->getInt(0), ((eveMessageIntList*)message)->getInt(1));
			// answer with the current playlist
			addMessage(playlist->getCurrentPlayList());
			break;
		case EVEMESSAGETYPE_AUTOPLAY:
			{
				bool autostart = true;
				if (((eveMessageInt*)message)->getInt() == 0) autostart = false;
				if (engineStatus->setAutoStart(autostart)) addMessage(engineStatus->getEngineStatusMessage());

			}
			break;
		case EVEMESSAGETYPE_START:
			if (!sendStart()) sendError(INFO,0,"cannot process START command with current engine status");
		break;
		case EVEMESSAGETYPE_STOP:
			if (engineStatus->setStop()){
				engineStatus->setAutoStart(false);
				emit stopSMs();
				addMessage(engineStatus->getEngineStatusMessage());
			}
			else {
				sendError(INFO,0,"cannot process STOP command with current engine status");
			}
			break;
		case EVEMESSAGETYPE_HALT:
			if (engineStatus->setHalt()){
				engineStatus->setAutoStart(false);
				emit haltSMs();
				addMessage(engineStatus->getEngineStatusMessage());
			}
			else {
				sendError(INFO,0,"cannot process HALT command");
			}
		break;
		case EVEMESSAGETYPE_BREAK:
			if (engineStatus->setBreak()){
				emit breakSMs();
			}
			else {
				sendError(INFO,0,"cannot process BREAK command with current engine status");
			}
			break;
		case EVEMESSAGETYPE_PAUSE:
			if (engineStatus->setPause()){
				emit pauseSMs();
				addMessage(engineStatus->getEngineStatusMessage());
			}
			else {
				sendError(INFO,0,"cannot process PAUSE command with current engine status");
			}
			break;
		case EVEMESSAGETYPE_STORAGECONFIG:
			engineStatus->addStorageId(((eveStorageMessage*)message)->getChainId());
			break;
		case EVEMESSAGETYPE_CHAINSTATUS:
			if (engineStatus->setChainStatus((eveChainStatusMessage*)message)){
				addMessage(engineStatus->getEngineStatusMessage());
			}
			// load the next entry from playlist, if whole chain is done
			if (engineStatus->getEngineStatus() == eveEngIDLENOXML) {
				if (loadPlayListEntry()){
					if (engineStatus->isAutoStart())
						if (!sendStart())
							sendError(INFO,0,"cannot process START command with current engine status");
				}
			}

			break;
		default:
			sendError(ERROR,0,"eveManager::handleMessage: unknown message");
			break;
	}
	delete message;
}
void eveManager::shutdown(){

	if (!shutdownPending){
		shutdownPending = true;
		disableInput();
		connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
	}

	// make sure mHub reads all outstanding messages before closing the channel
	shutdownThreadIfQueueIsEmpty();
}

/**
 * \brief load the next playlist Entry
 * \return true if xml was loaded or has been successfully loaded
 */
bool eveManager::loadPlayListEntry(){

	// we load a new playlist entry if not already done
	if (engineStatus->isNoXmlLoaded()){
		bool isRepeat = false;
		if (engineStatus->getRepeatCount() > 0){
			engineStatus->decrRepeatCount();
			isRepeat = true;
		}
		if (isRepeat || !playlist->isEmpty()){
			// load next playlist entry
			if (!isRepeat) {
				if (currentPlEntry != NULL){
					delete currentPlEntry;
					currentPlEntry = NULL;
				}
				currentPlEntry = playlist->takeFirst();
			}
			engineStatus->setLoadingXML(currentPlEntry->name);
			addMessage(engineStatus->getEngineStatusMessage());
			// load XML, send the appropriate engineStatus-Message, send playlist if successful
			if (engineStatus->setXMLLoaded(createSMs(currentPlEntry->data, isRepeat))){
				addMessage(new eveCurrentXmlMessage(currentPlEntry->name, currentPlEntry->author, currentPlEntry->data));
				addMessage(playlist->getCurrentPlayList());
				addMessage(engineStatus->getEngineStatusMessage());
				startChains();
			}
			else {
				addMessage(engineStatus->getEngineStatusMessage());
			}
		}
		else {
			// Playlist is empty, there is nothing we can do
			return false;
		}
	}
	return true;
}

/**
 * \brief create threads and scanModule Objects,
 *
 * \param xmldata scml data
 * \param isRepeat true if we repeat the scan decribed in xmldata, otherwise false
 *
 * for now, we cannot call this method if there are still chains running
 * from previous XML-Files
 */
bool eveManager::createSMs(QByteArray xmldata, bool isRepeat) {

	eveStartTime::setStartTime(QDateTime::currentDateTime());
	eveXMLReader* scmlParser = new eveXMLReader(this);
    if (scmlParser->read(xmldata)) {
    	sendError(INFO,0,"eveManager::createSMs: successfully parsed XML");
    }
    else {
    	sendError(ERROR,0,"eveManager::createSMs: error parsing XML");
    	return false;
    }
	sendError(DEBUG,0,QString("eveManager::createSMs: RepeatCount is %1").arg(scmlParser->getRepeatCount()));
	if (!isRepeat) engineStatus->setRepeatCount(scmlParser->getRepeatCount());

	// create a thread for every chain in the xml-File

	QStringList fileNameList;
	scanThreadList.clear();
	int storageChannelId = -1;
	foreach (int chainId, scmlParser->getChainIdList()){
		storageChannelId = 0;
		// we start a storage thread, if we have a savefilename and the name is not already on the list
		// for now: each filename has its storage thread
		QString value = scmlParser->getChainString(chainId, "savefilename");
		if (!value.isEmpty()){
			if (!fileNameList.contains(value)){
				fileNameList.append(value);
				QMutex mutex;
		        mutex.lock();
		        QWaitCondition waitRegistration;
				eveStorageThread *storageThread = new eveStorageThread(value, chainId, scmlParser, &xmldata, &waitRegistration, &mutex);
				storageThread->start();
				waitRegistration.wait(&mutex);
				storageChannelId = storageThread->getChannelId();
			}
		}
		// start a scanManager for every chain
		eveScanManager *scanManager = new eveScanManager(this, scmlParser, chainId, storageChannelId);
		eveScanThread *chainThread = new eveScanThread(scanManager);
		scanManager->moveToThread(chainThread);
		scanThreadList.append(chainThread);
		QThread *mathThread = new eveMathThread(chainId, scmlParser->getFilteredMathConfigs(chainId));
		mathThread->start();
	}
	// create monitors
	if (storageChannelId > 0){
		QList<eveDevice *>* monitors = scmlParser->getMonitorDeviceList();
		foreach (eveDevice * monitorOption, *monitors){
			addMessage(new eveMonitorRegisterMessage(monitorOption, storageChannelId));
		}
		delete monitors;
	}
	delete scmlParser;

	return true;
}

void eveManager::startChains(){
	// Now start the chainThreads
	foreach (QThread * chainThread, scanThreadList) {
		chainThread->start();
	}

}

/**
 * \brief send start signal to scanmanagers
 * \return true, if start signal was sent
 *
 */
bool eveManager::sendStart(){

	if (engineStatus->setStart()){
		emit startSMs();
		addMessage(engineStatus->getEngineStatusMessage());
		return true;
	}
	return false;
}

void eveManager::sendError(int severity, int errorType,  QString errorString){
	sendError(severity, EVEMESSAGEFACILITY_MANAGER, errorType, errorString);
}
/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveManager::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}
