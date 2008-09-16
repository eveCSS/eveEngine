
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
 */
int eveMessageHub::registerChannel(eveMessageChannel * channel, int channelId)
{
	if (channelId == 0 ) channelId = ++nextChannel;
	mChanHash.insert(channelId, channel);
	connect (channel, SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)), Qt::QueuedConnection);
	addError(INFO,0,QString("Successfully registered Channel %1\n").arg(channelId));

	// send status if this is the network channel
	if (channelId == EVECHANNEL_NET){
		channel->queueMessage(new eveEngineStatusMessage(engineStatus, currentXmlId));
	}
	return channelId;
}

/**
 *  \brief unregister a messageChannel
 * \param channelId predefined nr to identify message source or 0
 */
void eveMessageHub::unregisterChannel(int channelId)
{
	if (mChanHash.contains(channelId)){
		disconnect (mChanHash.value(channelId), SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)));
		mChanHash.remove(channelId);
		addError(INFO,0,QString("Successfully unregistered Channel %1\n").arg(channelId));
	}
	else {
		addError(4, 0, QString("Error unregistering unregistered channel %1\n").arg(channelId));
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

	if (!mChanHash.contains(messageSource)){
		addError(4,0,QString("unable to process message from unregistered source %1").arg(messageSource));
		return;
	}

	if ((message = mChanHash.value(messageSource)->getMessage())!= NULL ){

		switch (message->getType()) {
			case EVEMESSAGETYPE_ERROR:
				/* send errors and enginestatus to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
				}
				break;
			case EVEMESSAGETYPE_CHAINSTATUS:
			case EVEMESSAGETYPE_DATA:
				/* send chainstatus to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
				}
				/* send chainstatus to storagemodul if available */
				if (mChanHash.contains(EVECHANNEL_STORAGE)){
					mChanHash.value(EVECHANNEL_STORAGE)->queueMessage(message);
				}
				break;
			case EVEMESSAGETYPE_REQUEST:
				/* send requests to viewers if available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					/* keep track of requests */
					//TODO
					//reqHash.insert(message->getReqId, messageSource);

					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
				}
				else {
					//TODO
					// no viewer registered, we answer the request with error and drop the message
					addError(ERROR, 0, "EVEMESSAGETYPE_REQUEST: not yet implemented");
					delete message;
				}
				break;
			case EVEMESSAGETYPE_REQUESTCANCEL:
				addError(ERROR,0,"EVEMESSAGETYPE_REQUESTCANCEL: this should never happen");
				delete message;
				break;
			case EVEMESSAGETYPE_AUTOPLAY:
			case EVEMESSAGETYPE_REORDERPLAYLIST:
			case EVEMESSAGETYPE_ADDTOPLAYLIST:
			case EVEMESSAGETYPE_REMOVEFROMPLAYLIST:
				// forward this to manager
				if (mChanHash.contains(EVECHANNEL_MANAGER)) {
					addError(ERROR,0,"MessageHub: ADDTOPLAYLIST: got message");
					mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message);
				}
				else {
					addError(ERROR,0,"MessageHub: ADDTOPLAYLIST: Manager not connected");
					delete message;
				}
				break;
			case EVEMESSAGETYPE_PLAYLIST:
				/* send to viewers if any available */
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(message);
				}
				else {
					delete message;
				}
				break;
			case EVEMESSAGETYPE_START:
			{
				addError(INFO,0,"received EVEMESSAGETYPE_START ");
				delete message;
				// TODO
				//we send a dummy message for testing purpose only
				eveDataStatus dstat;
				eveMessage *newMessage = new eveDataMessage("Detektor 1", dstat, DMTunmodified, epicsTime::getCurrent(), QVector<float>(1, 15.1234));
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(newMessage);
				}
			}
			break;
			case EVEMESSAGETYPE_STOP:
				addError(ERROR,0,"received EVEMESSAGETYPE_STOP: not yet implemented");
				delete message;
				break;
			case EVEMESSAGETYPE_HALT:
			{
				//we send a dummy message for testing purpose only
				eveMessage *newMessage = new eveRequestMessage(reqMan->newId(0),EVEREQUESTTYPE_OKCANCEL, "Sie haben HALT gedrueckt");
				if (mChanHash.contains(EVECHANNEL_NET)){
					mChanHash.value(EVECHANNEL_NET)->queueMessage(newMessage);
				}
				delete message;
			}
			break;
			case EVEMESSAGETYPE_BREAK:
				addError(ERROR,0,"received EVEMESSAGETYPE_BREAK");
				delete message;
				break;
			case EVEMESSAGETYPE_PAUSE:
				addError(ERROR,0,"received EVEMESSAGETYPE_BREAK");
				delete message;
				break;
			case EVEMESSAGETYPE_ENDPROGRAM:
				addError(ERROR,0,"received EVEMESSAGETYPE_ENDPROGRAM");
				delete message;
				break;
			case EVEMESSAGETYPE_LIVEDESCRIPTION:
				{
					QString liveDesc = ((eveMessageText*)message)->getText();
					addError(ERROR,0,QString("received Live Description %1").arg(liveDesc));
					delete message;
				}
				break;
			case EVEMESSAGETYPE_REQUESTANSWER:
			{
				/* find the requests source and send the answer */
				int rid = ((eveRequestAnswerMessage*)message)->getReqId();
				int mChannelId = reqMan->takeId(rid);
				if (mChannelId){
					if (mChanHash.contains(mChannelId)){
						// send answer to request source
						mChanHash.value(mChannelId)->queueMessage(message);
					}
					else {
						// request source unavailable, send error and drop the message
						addError(ERROR,0,QString("request source unavailable %1").arg(((eveRequestAnswerMessage*)message)->getReqId()));
						delete message;
					}
				}
				else {
					addError(ERROR,0,"request without source");
					delete message;
				}
				// cancel this request for other viewers
				addError(ERROR,0,"sending cancel request");
				if (mChanHash.contains(EVECHANNEL_NET)){
					eveMessage *cMessage = new eveRequestCancelMessage(rid);
					mChanHash.value(EVECHANNEL_NET)->queueMessage(cMessage);
				}
			}
			break;
			default:
				addError(ERROR, 0, "eveMessageHub::newMessage unknown message type");
				delete message;
				break;
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
 */
void eveMessageHub::close()
{
	// if a netObject is registered, we call its shutdown method, else we kill the thread
	if (mChanHash.contains(EVECHANNEL_NET)){
		//mChanHash.value(EVECHANNEL_NET)->shutdown();
		connect (this, SIGNAL(closeAll()), mChanHash.value(EVECHANNEL_NET), SLOT(shutdown()), Qt::QueuedConnection);
	}
	// if a manager is registered, we call its shutdown method
	if (mChanHash.contains(EVECHANNEL_MANAGER)){
		//mChanHash.value(EVECHANNEL_MANAGER)->shutdown();
		connect (this, SIGNAL(closeAll()), mChanHash.value(EVECHANNEL_MANAGER), SLOT(shutdown()), Qt::QueuedConnection);
	}
	emit closeAll();

	if (!nwThread->wait(1000)) nwThread->terminate();
	if (!mThread->wait(1000)) mThread->terminate();

	emit closeParent();
}
// TODO remove
void eveMessageHub::log(QString message)
{
	// obsolete
	eveError::log(1,message);
}

