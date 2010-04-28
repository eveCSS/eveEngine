
#include <QString>
#include "eveMessageFilter.h"
#include "eveNetObject.h"
#include "eveError.h"


eveMessageFilter::eveMessageFilter(eveNetObject *nwPtr)
{
	netObject = nwPtr;
	skippedMessageCount = 0;
	messageCount=0;
	chainStatusCache = NULL;
	engineStatusCache = NULL;
	playlistCache = NULL;
	currentXmlCache = NULL;
	seqTimer = new QTimer(this);
    connect(seqTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    seqTimer->start(EVEMESSAGEFILTER_TIMEOUT);
}

eveMessageFilter::~eveMessageFilter()
{
	if (engineStatusCache) delete engineStatusCache;
	if (chainStatusCache) delete chainStatusCache;
	if (playlistCache) delete playlistCache;
	if (currentXmlCache) delete currentXmlCache;
	while (errorMessageCacheList.count() > 0) delete errorMessageCacheList.takeFirst();
	seqTimer->stop();
	delete seqTimer;
}

/**
 * \brief receives and handles the next message
 * \param message current message
 * \return true if the filter condition matched, else false
 *
 * Actual filter method receives the message and compares it to the last message
 * If this is an error message and the last EVEMESSAGEFILTER_LOWLIMIT messages
 * were the same type of error message, this message is discarded.
 * If the EVEMESSAGEFILTER_HIGHLIMIT is reached, this message is discarded
 *
 */
bool eveMessageFilter::checkMessage(eveMessage *message)
{
	if (message->getType() == EVEMESSAGETYPE_ERROR){
		++messageCount;
		if (messageCount > EVEMESSAGEFILTER_LOWLIMIT){
			if (!errorMessageCacheList.isEmpty()){
				if (((eveErrorMessage*)message)->compare(errorMessageCacheList.last())) {
					++skippedMessageCount;
					--messageCount;
					return true;
				}
			}
		}
		if (messageCount > EVEMESSAGEFILTER_HIGHLIMIT){
			++skippedMessageCount;
			--messageCount;
			return true;
		}
	}
	return false;
}

/**
 * \brief return a list with all cached message
 * \return the list with messages
 *
 * note: order is important, Clients expect the following order if engine is executing
 */
QList<eveMessage * > * eveMessageFilter::getCache()
{
	QList<eveMessage * > *cachelist = new  QList<eveMessage * >;
	if (currentXmlCache) cachelist->append(currentXmlCache);
	if (playlistCache) cachelist->append(playlistCache);
	if (engineStatusCache) cachelist->append(engineStatusCache);
	if (chainStatusCache) cachelist->append(chainStatusCache);

	return cachelist;
}

/**
 * \brief caches or deletes a message, when it has been sent
 * \param message current message
 *
 * Delete the message after caching the last few error messages, chain status,
 * engine status, current xml and playlist.
 * The cached messages are sent to a viewer, when it connects.
 *
 */
void eveMessageFilter::queueMessage(eveMessage *message)
{

	switch (message->getType()) {
		case EVEMESSAGETYPE_ERROR:
			// we keep the last few errors
			errorMessageCacheList.append(message);
			while (errorMessageCacheList.count() > EVEMESSAGEFILTER_QUEUELENGTH) {
				eveMessage *delMessage = errorMessageCacheList.takeFirst();
				delete delMessage;
			}
			break;
		case EVEMESSAGETYPE_ENGINESTATUS:
			// we keep the last status
			if (engineStatusCache) delete engineStatusCache;
			engineStatusCache = (eveEngineStatusMessage*) message;
			break;
		case EVEMESSAGETYPE_CHAINSTATUS:
			if (chainStatusCache) delete chainStatusCache;
			chainStatusCache = (eveChainStatusMessage*) message;
			break;
		case EVEMESSAGETYPE_CURRENTXML:
			if (currentXmlCache) delete currentXmlCache;
			currentXmlCache = (eveCurrentXmlMessage*) message;
			break;
		case EVEMESSAGETYPE_PLAYLIST:
			if (playlistCache) delete playlistCache;
			playlistCache = (evePlayListMessage*) message;
			break;
		case EVEMESSAGETYPE_DATA:
			// no data caching yet
		default:
			delete message;
			break;
	}
}

/**
 * \brief reset the limit counter; method is called by the internal timer
 *
 * Every EVEMESSAGEFILTER_TIMEOUT seconds this method is called and resets all
 * limit counters. If the counters exceeded limits an error message is sent.
 *
 */
void eveMessageFilter::timeout()
{
	if (skippedMessageCount){
		if (messageCount >= EVEMESSAGEFILTER_HIGHLIMIT) {
			netObject->sendMessage(new eveErrorMessage(MINOR, EVEMESSAGEFACILITY_MFILTER, 0x0007,
					QString("HighLimit: skipped %1 messages").arg(skippedMessageCount)));
		}
		else {
			netObject->sendMessage(new eveErrorMessage(MINOR, EVEMESSAGEFACILITY_MFILTER, 0x0006,
					QString("LowLimit: skipped %1 equal messages").arg(skippedMessageCount)));
		}
	}
	messageCount = 0;
	skippedMessageCount= 0;
}

/**
 * clear all cached messages except playlist message
 */
void eveMessageFilter::clearCache(){

	while (!errorMessageCacheList.isEmpty())
		delete errorMessageCacheList.takeFirst();
	if (engineStatusCache) delete engineStatusCache;
	engineStatusCache = NULL;
	if (chainStatusCache) delete chainStatusCache;
	chainStatusCache = NULL;
	if (currentXmlCache) delete currentXmlCache;
	currentXmlCache = NULL;

}
