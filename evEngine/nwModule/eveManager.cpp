/*
 * eveManager.cpp
 *
 *  Created on: 08.09.2008
 *      Author: eden
 */

#include "eveManager.h"
#include "eveMessageHub.h"
#include "eveError.h"

/**
 * \brief init and register with messageHub
 */
eveManager::eveManager() {
	autoStart = false;
	playlist = new evePlayListManager();
	eveMessageHub * mHub = eveMessageHub::getmHub();
	channelId = mHub->registerChannel(this, EVECHANNEL_MANAGER);

}

eveManager::~eveManager() {
	// we do not unregister with mhub
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
			if (((eveMessageInt*)message)->getInt() == 0)
				autoStart = false;
			else
				autoStart = true;
			break;
	}
	delete message;
}
