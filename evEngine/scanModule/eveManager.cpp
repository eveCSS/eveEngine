/*
 * eveManager.cpp
 *
 *  Created on: 08.09.2008
 *      Author: eden
 */

#include <QTimer>
#include "eveManager.h"
#include "eveRequestManager.h"
#include "eveMessageHub.h"
#include "eveError.h"
#include "eveXMLReader.h"
#include "eveScanManager.h"
#include "eveScanThread.h"

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
			// TODO
			//we send dummy data for testing purpose only
			addMessage(new eveRequestMessage(eveRequestManager::getRequestManager()->newId(channelId),EVEREQUESTTYPE_OKCANCEL, "Sie haben HALT gedrueckt"));
			break;
		case EVEMESSAGETYPE_PAUSE:
			if (engineStatus->setPause()){
				emit pauseSMs();
				addMessage(engineStatus->getEngineStatusMessage());
			}
			else {
				sendError(INFO,0,"cannot process STOP command with current engine status");
			}
			break;
		case EVEMESSAGETYPE_CHAINSTATUS:
			if (engineStatus->setChainStatus((eveChainStatusMessage*)message)){
				addMessage(engineStatus->getEngineStatusMessage());
			}
			// load the next entry from playlist, if whole chain is done
			if (engineStatus->getEngineStatus() == eveEngIDLENOXML) {
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
	eveMessageHub::getmHub()->unregisterChannel(channelId);
	QThread::currentThread()->quit();
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
			}
			addMessage(engineStatus->getEngineStatusMessage());
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
    	sendError(INFO,0,"eveManager::createSMs: error parsing XML");
    	return false;
    }
	// create a thread for every chain in the xml-File
    // TODO we might reuse existing threads in future

    // TODO might be easier to loop over a QList of chainids, since we dont't need the
    //      domelements any more
	QHashIterator<int, QDomElement> itera(scmlParser->getChainIdHash());
	while (itera.hasNext()) {
		itera.next();
		eveScanManager *scanManager = new eveScanManager(this, scmlParser, itera.key());
		eveScanThread *chainThread = new eveScanThread(scanManager);
		scanManager->moveToThread(chainThread);
		scanThreadList.append(chainThread);
		chainThread->start();
		// TODO remove prints
		printf("ScanManager: Current Thread id: %d\n",QThread::currentThread());
		printf("ScanManager: new Thread id: %d\n", chainThread);
	}
	delete scmlParser;

	return true;
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
