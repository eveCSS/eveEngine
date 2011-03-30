/*
 * eveManager.cpp
 *
 *  Created on: 08.09.2008
 *      Author: eden
 */

#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "eveManager.h"
#include "eveRequestManager.h"
#include "eveMessageHub.h"
#include "eveError.h"
#include "eveXMLReader.h"
#include "eveScanManager.h"
#include "eveScanThread.h"
#include "eveStorageThread.h"
#include "eveMathThread.h"

/**
 * \brief init and register with messageHub
 */
eveManager::eveManager() {
	deviceList = new eveDeviceList();
	playlist = new evePlayListManager();
	eveMessageHub * mHub = eveMessageHub::getmHub();
	channelId = mHub->registerChannel(this, EVECHANNEL_MANAGER);
	engineStatus = new eveManagerStatusTracker();
}

eveManager::~eveManager() {
	delete playlist;
	delete engineStatus;
	// we already unregistered with mhub
}

/**
 * \brief process messages sent to eveManager
 */
void eveManager::handleMessage(eveMessage *message){

	eveError::log(4, "eveManager: message arrived");
	switch (message->getType()) {
		case EVEMESSAGETYPE_ADDTOPLAYLIST:
			playlist->addEntry(((eveAddToPlMessage*)message)->getXmlName(), ((eveAddToPlMessage*)message)->getXmlAuthor(), ((eveAddToPlMessage*)message)->getXmlData());
			// answer with the current playlist
			addMessage(playlist->getCurrentPlayList());
			// load the next playlist entry, if Engine is idle and no XML is loaded
			if (engineStatus->getEngineStatus() == eveEngIDLENOXML)
				if (loadPlayListEntry()){
					if (engineStatus->getAutoStart())
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
		{
			//
			if (loadPlayListEntry() || (engineStatus->getEngineStatus() == eveEngPAUSED)){
				if (!sendStart())
					sendError(INFO,0,"cannot process START command with current engine status");
			}
		}
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
				deviceList->clearAll();
				if (loadPlayListEntry()){
					if (engineStatus->getAutoStart())
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

	eveError::log(1, QString("eveManager: shutdown"));
	// stop input Queue
	disableInput();
	// make sure mHub reads all outstanding messages before closing the channel
	if (unregisterIfQueueIsEmpty()){
		QThread::currentThread()->quit();
	}
	else {
		// call us again
		QTimer::singleShot(500, this, SLOT(shutdown()));
	}
}

/**
 * \brief load the next playlist Entry
 * \return true if xml was loaded or has been successfully loaded
 */
bool eveManager::loadPlayListEntry(){

	// we load a new playlist entry if not already done
	if (!engineStatus->isXmlLoaded()){
		if (!playlist->isEmpty()){
			// load next playlist entry
			evePlayListData* plEntry = playlist->takeFirst();
			if (!engineStatus->setLoadingXML(plEntry->name)) return false;
			addMessage(engineStatus->getEngineStatusMessage());
			// load XML, send the appropriate engineStatus-Message, send playlist if successful
			if (engineStatus->setXMLLoaded(createSMs(plEntry->data))){
				addMessage(new eveCurrentXmlMessage(plEntry->name, plEntry->author, plEntry->data));
				addMessage(playlist->getCurrentPlayList());
				addMessage(engineStatus->getEngineStatusMessage());
				startChains();
			}
			else {
				addMessage(engineStatus->getEngineStatusMessage());
			}
			delete plEntry;
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
 * for now, we cannot call this method if there are still chains running
 * from previous XML-Files
 */
bool eveManager::createSMs(QByteArray xmldata) {

	eveXMLReader* scmlParser = new eveXMLReader(this);
	deviceList->clearAll();
    if (scmlParser->read(xmldata, deviceList)) {
    	sendError(INFO,0,"eveManager::createSMs: successfully parsed XML");
    }
    else {
    	sendError(ERROR,0,"eveManager::createSMs: error parsing XML");
    	return false;
    }
	// create a thread for every chain in the xml-File

	QStringList fileNameList;
	scanThreadList.clear();
	foreach (int chainId, scmlParser->getChainIdList()){
		int storageChannelId = 0;
		// we start a storage thread, if we have a savefilename and the name is not already on the list
		QString value = scmlParser->getChainString(chainId, "savefilename");
		if (!value.isEmpty()){
			if (!fileNameList.contains(value)){
				fileNameList.append(value);
				QMutex mutex;
		        mutex.lock();
		        QWaitCondition waitRegistration;
				eveStorageThread *storageThread = new eveStorageThread(value, &xmldata, &waitRegistration, &mutex);
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

void eveManager::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}
