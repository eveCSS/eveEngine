
#include <QTimer>
#include "eveMessageHub.h"
#include "eveNwThread.h"
#include "eveManagerThread.h"
#include "eveEventThread.h"
#include "eveError.h"
#include "eveRequestManager.h"
#include "eveParameter.h"

eveMessageHub* eveMessageHub::mHub=NULL;

eveMessageHub::eveMessageHub()
{
	nextChannel = EVECHANNELS_RESERVED;
	engineStatus = EVEENGINESTATUS_IDLENOXML;
	mHub = this;
	currentXmlId = "none";
	reqMan = new eveRequestManager();
}

void eveMessageHub::init()
{
    // start nwThread
    if (eveParameter::getParameter("use_network") == "yes"){
    	QMutex mutex;
        mutex.lock();
        QWaitCondition waitRegistration;
    	nwThread = new eveNwThread(&waitRegistration, &mutex);
        nwThread->start();
    	waitRegistration.wait(&mutex);
    }
    // start managerThread
    {
    	QMutex mutex;
        mutex.lock();
        QWaitCondition waitRegistration;
    	managerThread= new eveManagerThread(&waitRegistration, &mutex);
        managerThread->start();
    	waitRegistration.wait(&mutex);
    }
    // start eventThread
    {
    	QMutex mutex;
        mutex.lock();
        QWaitCondition waitRegistration;
    	eventThread= new eveEventThread(&waitRegistration, &mutex);
    	eventThread->start();
    	waitRegistration.wait(&mutex);
    }

}

eveMessageHub::~eveMessageHub()
{
}

/**
 * \brief static function returns mHub
 * \returns	a pointer to current mHub
 */
eveMessageHub * eveMessageHub::getmHub()
{
	return mHub;
}

/**
 *  \brief register a messageChannel as a new message source
 * \param messageChannel
 * \param channelId predefined number to identify message source or 0
 *
 * this is usually called from a different thread
 */
int eveMessageHub::registerChannel(eveMessageChannel * channel, int channelId)
{
	int newId;
	QWriteLocker locker(&channelLock);

	if (channelId == 0 ) {
		newId = ++nextChannel;
	}
	else if (channelId == EVECHANNEL_STORAGE ) {
		newId = ++nextChannel;
		storageChannelList.append(newId);
	}
	else
		newId = channelId;

	mChanHash.insert(newId, channel);
	connect (channel, SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)), Qt::QueuedConnection);
	connect (this, SIGNAL(closeAll()), channel, SLOT(shutdown()), Qt::QueuedConnection);

	return newId;
}

/**
 *  \brief unregister a messageChannel
 * \param channelId id which was assigned during registerChannel
 *
 * this is usually called from a different thread
 */
void eveMessageHub::unregisterChannel(int channelId)
{
	QWriteLocker locker(&channelLock);
	if (mChanHash.contains(channelId)){
		disconnect (mChanHash.value(channelId), SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)));
		disconnect (this, SIGNAL(closeAll()), mChanHash.value(channelId), SLOT(shutdown()));
		mChanHash.remove(channelId);
	}
	storageChannelList.removeAll(channelId);
	return;
}

/**
 * \brief handle the various messages
 * \param messageSource source of new message (registered id of messageChannel)
 *
 * message channels (usually running in a different thread) put a message in their
 * sendqueue and signal messageWaiting which calls this method.
 * Take the message from the channels queue and send it to someone else or delete it.
 */
void eveMessageHub::newMessage(int messageSource)
{
	eveMessage *message;
	QReadLocker locker(&channelLock);

	if (!mChanHash.contains(messageSource)){
		addError(4,0,QString("unable to process message from unregistered source %1").arg(messageSource));
		return;
	}

	if ((message = mChanHash.value(messageSource)->getMessage())!= NULL ){

		switch (message->getType()) {
			case EVEMESSAGETYPE_ERROR:
			case EVEMESSAGETYPE_CURRENTXML:
			case EVEMESSAGETYPE_ENGINESTATUS:
				/* send errors and enginestatus to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					if (mChanHash.value(EVECHANNEL_NET)->queueMessage(message))message = NULL;
				}
				break;
			case EVEMESSAGETYPE_CHAINSTATUS:
				/* send  to storagemodules if available */
				if (haveStorage()){
					eveMessage *mclone = message->clone();
					if (!sendToStorage(mclone))
						delete mclone;
				}
				/* send chainstatus to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					eveMessage *mclone = message->clone();
					if(!mChanHash.value(EVECHANNEL_NET)->queueMessage(mclone)) delete mclone;
				}
				/* send chainstatus to eventManager (we always have one)*/
				if (mChanHash.contains(EVECHANNEL_EVENT)){
					eveMessage *mclone = message->clone();
					if(!mChanHash.value(EVECHANNEL_EVENT)->queueMessage(mclone)) delete mclone;
				}
				/* send chainstatus to manager (we always have one)*/
				if (mChanHash.contains(EVECHANNEL_MANAGER)){
					if (mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message))message = NULL;
				}
				break;
			case EVEMESSAGETYPE_DATA:
				{
					/* send data to viewers if available */
					if (mChanHash.contains(EVECHANNEL_NET)){
						// do we need to clone the message?
						if (haveStorage()){
							eveMessage *mclone = message->clone();
							if (!mChanHash.value(EVECHANNEL_NET)->queueMessage(mclone)){
								delete mclone;
								addError(ERROR, 0, "EVEMESSAGETYPE_DATA: EVECHANNEL_NET does not accept data ");
							}
						}
						else {
							if (mChanHash.value(EVECHANNEL_NET)->queueMessage(message)) message = NULL;

						}
					}
					/* send data to storagemodules if available */
					if (haveStorage()){
						if (sendToStorage(message)) message = NULL;
					}
				}
				break;
			case EVEMESSAGETYPE_AUTOPLAY:
			case EVEMESSAGETYPE_REORDERPLAYLIST:
			case EVEMESSAGETYPE_ADDTOPLAYLIST:
			case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
			case EVEMESSAGETYPE_START:
			case EVEMESSAGETYPE_STOP:
			case EVEMESSAGETYPE_HALT:
			case EVEMESSAGETYPE_BREAK:
			case EVEMESSAGETYPE_PAUSE:
				// forward this to manager
				if (mChanHash.contains(EVECHANNEL_MANAGER)) {
					if (mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message)) message = NULL;
				}
				else {
					addError(ERROR,0,"MessageHub: Manager not connected");
				}
				break;
			case EVEMESSAGETYPE_PLAYLIST:
				/* send to viewers if any available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					if (mChanHash.value(EVECHANNEL_NET)->queueMessage(message)) message = NULL;
				}
				break;
			case EVEMESSAGETYPE_ENDPROGRAM:
				addError(DEBUG, 0, "received End command, exiting...");
				delete message;
				message = NULL;
				// don't just call close(); we locked the channelLock !
				QTimer::singleShot(0, this, SLOT(close()));
				break;
			case EVEMESSAGETYPE_LIVEDESCRIPTION:
				addError(DEBUG, 0, QString("received Live Description %1").arg(((eveMessageText*)message)->getText()));
				if (haveStorage()){
					if (sendToStorage(message)) message = NULL;
				}
				break;
			case EVEMESSAGETYPE_REQUEST:
			case EVEMESSAGETYPE_REQUESTCANCEL:
				// requests may be cancelled by SMs too
				/* send requests to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					/* do we need to keep track of requests ? */
					if (mChanHash.value(EVECHANNEL_NET)->queueMessage(message)) message = NULL;
				}
				else {
					// no network connection, we log an error and drop the message
					addError(ERROR, 0, "EVEMESSAGETYPE_REQUEST(CANCEL): request cannot send request, running without net");
				}
				break;
			case EVEMESSAGETYPE_REQUESTANSWER:
			{
				/* find the request source and send the answer */
				int rid = ((eveRequestAnswerMessage*)message)->getReqId();
				int mChannelId = reqMan->takeId(rid);
				if (mChannelId){
					if (mChanHash.contains(mChannelId)){
						// send answer to request source
						if (mChanHash.value(mChannelId)->queueMessage(message)) message = NULL;
					}
					else {
						// request source unavailable, log an error and drop the message
						addError(ERROR,0,QString("request source unavailable %1").arg(((eveRequestAnswerMessage*)message)->getReqId()));
					}
				}
				else {
					addError(ERROR,0,"request without source");
				}
				// cancel this request for other viewers
				if (mChanHash.contains(EVECHANNEL_NET)){
					eveMessage *cMessage = new eveRequestCancelMessage(rid);
					if (!mChanHash.value(EVECHANNEL_NET)->queueMessage(cMessage)){
						delete cMessage;
						addError(ERROR,0,"NetChannel does not accept a cancelRequest Message");
					}
				}
			}
				break;
			case EVEMESSAGETYPE_STORAGECONFIG:
				/* send configuration to storagemodule if available */
				if (haveStorage()){
					// tell manager cids with storage
					if (mChanHash.contains(EVECHANNEL_MANAGER)){
						eveMessage *mclone = message->clone();
						if (!mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(mclone)) delete mclone;
					}
					if (sendToStorage(message)) message = NULL;
				}
				break;
//			case EVEMESSAGETYPE_STORAGEACK:
//				/* send back to corresponding chain */
//				if (mChanHash.contains(message->getDestination())){
//					eveMessage *mclone = message->clone();
//					if (!mChanHash.value(message->getDestination())->queueMessage(mclone)) delete mclone;
//				}
//				/* send storageack to manager (we always have one) to keep track of storagemodules*/
//				if (mChanHash.contains(EVECHANNEL_MANAGER)){
//					if (mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message))message = NULL;
//				}
//				break;
			case EVEMESSAGETYPE_DEVINFO:
				/* send device info to storagemodule if available */
				if (haveStorage()){
					if (sendToStorage(message)) message = NULL;
				}
				break;
			case EVEMESSAGETYPE_EVENTREGISTER:
				if (mChanHash.contains(EVECHANNEL_EVENT)){
					if (mChanHash.value(EVECHANNEL_EVENT)->queueMessage(message)) message = NULL;
				}
				break;
			default:
				addError(ERROR, 0, "newMessage: unknown message type");
				break;
		}
		if (message != NULL) {
			addError(ERROR, 0, QString("newMessage: unable to forward message, type: %1").arg(message->getType()));
			delete message;
		}
	}
}

/**
 * \brief queue an error message to be sent to connected Viewers (use only for messageHub)
 * \param severity	Severity as defined in VE-Protocol-Spec
 * \param errorType errorType as defined in VE-Protocol-Spec
 * \param errorString	additional error description
 *
 */
void eveMessageHub::addError(int severity, int errorType,  QString errorString)
{
	QReadLocker locker(&channelLock);
	if (mChanHash.contains(EVECHANNEL_NET)){
		eveErrorMessage *errorMessage = new eveErrorMessage(severity, EVEMESSAGEFACILITY_MHUB, errorType, errorString);
		if (!mChanHash.value(EVECHANNEL_NET)->queueMessage(errorMessage)) delete errorMessage;
	}
	else {
		// no viewer connected, we log locally
		eveError::log(severity, errorString);
	}
}

/**
 * \brief shutdown messageHub
 *
 * call shutdown method of all registered channels, start timer which calls waitUntilDone methode
 */
void eveMessageHub::close()
{

	eveError::log(0, "MessageHub shut down");
	emit closeAll();
	QTimer::singleShot(500, this, SLOT(waitUntilDone()));
}
/**
 * \brief wait until all channels have unregistered, then shutdown
 *
 */
void eveMessageHub::waitUntilDone()
{
	QReadLocker locker(&channelLock);
	if (!mChanHash.isEmpty()){
		QTimer::singleShot(500, this, SLOT(waitUntilDone()));
		addError(INFO, 0, "eveMessageHub: still waiting for threads to shutdown");
		return;
	}
	eveError::log(0, "MessageHub shutdown, done");
	printf("MessageHub shutdown, done\n");
	emit closeParent();
}

/**
 *
 * @param message pointer to the message to be sent
 * @return true if message was put into the receiver queue or has been deleted
 */
bool eveMessageHub::sendToStorage(eveMessage* message)
{
	eveMessage* clone;
	bool retval = false;
	int channel = message->getDestination();
	// channel EVECHANNEL_STORAGE means: send it to all storageChannels
	if ( channel == EVECHANNEL_STORAGE ){
		int count = storageChannelList.count();
		foreach (int chan, storageChannelList) {
			if (count > 1)
				clone = message->clone();
			else
				clone = message;
			if (!mChanHash.value(chan)->queueMessage(clone)) delete clone;
			--count;
			retval = true;
		}
	}
	else {
		if (mChanHash.contains(channel) && storageChannelList.contains(channel)){
			if (!mChanHash.value(channel)->queueMessage(message)) delete message;
			retval = true;
		}
	}
	return retval;
}
