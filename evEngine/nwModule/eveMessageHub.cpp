
#include <QTimer>
#include "eveMessageHub.h"
#include "eveManagerThread.h"
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
    if (eveParameter::getParameter("use_network") == "yes"){
    	nwThread = new eveNwThread();
        nwThread->start();
    }
    mThread= new eveManagerThread();
    mThread->start();
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
 * \param channelId predefined nr to identify message source or 0
 *
 * this is usually called from a different thread
 */
int eveMessageHub::registerChannel(eveMessageChannel * channel, int channelId)
{
	int newId = channelId;
	QWriteLocker locker(&channelLock);

	if (newId == 0 ) newId = ++nextChannel;
	mChanHash.insert(newId, channel);
	connect (channel, SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)), Qt::QueuedConnection);

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
		mChanHash.remove(channelId);
	}
	return;
}

/**
 * \brief handle the various messages
 * \param messageSource source of new message (registered id of messageChannel)
 *
 * message channels (usually running in a different thread) put a message in our message queue
 * and signals messageWaiting which calls this method.
 * Take the message from the input queue and send it to someone else or delete it.
 */
void eveMessageHub::newMessage(int messageSource)
{
	eveMessage *message;
	QWriteLocker locker(&channelLock);

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
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
					message = NULL;
				}
				break;
			case EVEMESSAGETYPE_CHAINSTATUS:
				/* send chainstatus to manager */
				if (mChanHash.contains(EVECHANNEL_MANAGER)){
					mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message->clone());
				}
				/* send chainstatus to storagemodul if available */
				if (mChanHash.contains(EVECHANNEL_STORAGE)){
					mChanHash.value(EVECHANNEL_STORAGE)->queueMessage(message->clone());
					message = NULL;
				}
				/* send chainstatus to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
					message = NULL;
				}
				break;
			case EVEMESSAGETYPE_DATA:
				/* send chainstatus to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message->clone());
				}
				/* send chainstatus to storagemodul if available */
				if (mChanHash.contains(EVECHANNEL_STORAGE)){
					mChanHash.value(EVECHANNEL_STORAGE)->queueMessage(message);
					message = NULL;
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
					mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message);
					message = NULL;
				}
				else {
					addError(ERROR,0,"MessageHub: Manager not connected");
				}
				break;
			case EVEMESSAGETYPE_PLAYLIST:
				/* send to viewers if any available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
					message = NULL;
				}
				break;
			case EVEMESSAGETYPE_ENDPROGRAM:
				addError(ERROR,0,"received End command, exiting...");
				// don't just call close(); we locked the channelLock !
				QTimer::singleShot(0, this, SLOT(close()));
				break;
			case EVEMESSAGETYPE_LIVEDESCRIPTION:
				addError(ERROR,0,QString("received Live Description %1").arg(((eveMessageText*)message)->getText()));
				if (mChanHash.contains(EVECHANNEL_STORAGE)){
					mChanHash.value(EVECHANNEL_STORAGE)->queueMessage(message);
					message = NULL;
				}
				break;
			case EVEMESSAGETYPE_REQUEST:
			case EVEMESSAGETYPE_REQUESTCANCEL:
				// requests may be cancelled by SMs too
				/* send requests to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					/* do we need to keep track of requests ? */
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
					message = NULL;
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
						mChanHash.value(mChannelId)->queueMessage(message);
						message = NULL;
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
					mChanHash.value(EVECHANNEL_NET)->queueMessage(cMessage);
				}
			}
			break;
			default:
				addError(ERROR, 0, "eveMessageHub::newMessage unknown message type");
				break;
		}
		if (message != NULL) delete message;
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
	QWriteLocker locker(&channelLock);
	if (mChanHash.contains(EVECHANNEL_NET)){
		mChanHash.value(EVECHANNEL_NET)->queueMessage(new eveErrorMessage(severity, EVEMESSAGEFACILITY_MHUB, errorType, errorString));
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
	channelLock.lockForRead();
	foreach (eveMessageChannel *channel, mChanHash) {
		connect (this, SIGNAL(closeAll()), channel, SLOT(shutdown()), Qt::QueuedConnection);
	}
	channelLock.unlock();

	emit closeAll();
	QTimer::singleShot(500, this, SLOT(waitUntilDone()));
}
/**
 * \brief wait until all channels have unregistered, then shutdown
 *
 */
void eveMessageHub::waitUntilDone()
{
	QWriteLocker locker(&channelLock);
	if (!mChanHash.isEmpty()){
		QTimer::singleShot(500, this, SLOT(waitUntilDone()));
		addError(ERROR, 0, "eveMessageHub: still waiting for threads to shutdown");
		return;
	}
	emit closeParent();
}
