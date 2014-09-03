
#include <iostream>
#include <QTimer>
#include <QApplication>
#include <QFile>
#include "eveMessageHub.h"
#include "eveNwThread.h"
#include "eveManagerThread.h"
#include "eveEventThread.h"
#include "eveError.h"
#include "eveRequestManager.h"
#include "eveParameter.h"
#include "eveMathManager.h"

eveMessageHub* eveMessageHub::mHub=NULL;

eveMessageHub::eveMessageHub(bool gui, bool net)
{
	useGui = gui;
	useNet = net;
	nextChannel = EVECHANNELS_RESERVED;
	engineStatus = EVEENGINESTATUS_IDLENOXML;
	mHub = this;
	currentXmlId = "none";
	reqMan = new eveRequestManager();
	loglevel = eveParameter::getParameter("loglevel").toInt();
	connect (this, SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)), Qt::QueuedConnection);
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
    // batch processing: execute startFile if any
	QString fileName = eveParameter::getParameter("startFile");
	if (!fileName.isEmpty()){
		// read the file and send the content
		QFile xmlFile(fileName);
		if (xmlFile.open(QFile::ReadOnly)) {
		    eveMessage* plmessage = new eveAddToPlMessage(fileName, QString("none"), xmlFile.readAll());

		    // switch on autoplay
			eveMessage* message = new eveMessageInt(EVEMESSAGETYPE_AUTOPLAY, 1);
			if (!mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message)) delete message;

		    // add to playList
			if (!mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(plmessage)) delete plmessage;
			eveError::log(DEBUG, QString("Added %1 to playList").arg(fileName));
		}
		else
		eveError::log(ERROR, QString("unable to execute file: %1").arg(fileName));

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
	else if (channelId == EVECHANNEL_MATH ) {
		newId = ++nextChannel;
                int channelchid = ((eveMathManager*)channel)->getChainId();
                mathChidHash.insert(channelchid, channel);
	}
	else
		newId = channelId;

	mChanHash.insert(newId, channel);
	connect (channel, SIGNAL(messageWaiting(int)), this, SLOT(newMessage(int)), Qt::QueuedConnection);
	if(channelId == EVECHANNEL_NET){
		connect (this, SIGNAL(closeNet()), channel, SLOT(shutdown()), Qt::QueuedConnection);
	}
	else {
		connect (this, SIGNAL(closeAll()), channel, SLOT(shutdown()), Qt::QueuedConnection);
	}
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
        // remove the channel from mathChidHash
        eveMessageChannel* mchannel = mChanHash.take(channelId);
        int key = mathChidHash.key(mchannel, 0);
        if (key != 0) mathChidHash.remove(key);
    }
    storageChannelList.removeAll(channelId);
    eveError::log(DEBUG, QString("eveMessageHub: unregistered channel %1").arg(channelId));
    return;
}

/**
 * \brief get messages from message source and handle it
 * \param messageSource source of new message (registered id of messageChannel)
 *
 * message channels (usually running in a different thread) put a message in their
 * sendqueue and signal messageWaiting which calls this method.
 * Take the message from the channels queue and send it to someone else or delete it.
 */
void eveMessageHub::newMessage(int messageSource)
{
    eveMessage *message=NULL;

    QReadLocker locker(&channelLock);

    if (!mChanHash.contains(messageSource)){
        // addError(DEBUG,0,QString("unable to process message from unregistered source %1").arg(messageSource));
        return;
    }

    // try to get the lock
    if (mChanHash.value(messageSource)->getLock()){
        message = mChanHash.value(messageSource)->getMessage();
        mChanHash.value(messageSource)->releaseLock();
    }
    else {
        // we couldn't get the lock, try again later
        emit messageWaiting(messageSource);
    }

    while (message != NULL ){

        switch (message->getType()) {
        case EVEMESSAGETYPE_ERROR:
        {
            eveErrorMessage* emesg = (eveErrorMessage*)message;
            eveError::log(emesg->getSeverity(), emesg->getErrorText(), emesg->getFacility());
            // skip messages with severity below loglevel
            if (loglevel < emesg->getSeverity()) {
                delete message;
                message = NULL;
                break;
            }
        }
        case EVEMESSAGETYPE_CURRENTXML:
            if (useNet && mChanHash.contains(EVECHANNEL_NET)){
                if (mChanHash.value(EVECHANNEL_NET)->queueMessage(message))message = NULL;
            }
            else {
                delete message;
                message = NULL;
            }
            break;
        case EVEMESSAGETYPE_ENGINESTATUS:
            if (useNet && mChanHash.contains(EVECHANNEL_NET)){
                eveMessage *mclone = message->clone();
                if(!mChanHash.value(EVECHANNEL_NET)->queueMessage(mclone)) delete mclone;
            }
            if (mChanHash.contains(EVECHANNEL_MANAGER)){
                if (mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message))message = NULL;
            }
            break;
        case EVEMESSAGETYPE_CHAINSTATUS:
        {
            eveMessage* mclone = NULL;
            /* send  to storagemodules if available */
            if (haveStorage() && message->hasDestinationFacility(EVECHANNEL_STORAGE)){
                if (mclone == NULL) mclone = message->clone();
                if (sendToStorage(mclone)) mclone = NULL;
            }
            /* send to math if available */
            if (message->hasDestinationFacility(EVECHANNEL_MATH)){
                if (mclone == NULL) mclone = message->clone();
                if (sendToMath(mclone)) mclone = NULL;
            }

            /* send chainstatus to viewers if available */
            if (mChanHash.contains(EVECHANNEL_NET) && message->hasDestinationFacility(EVECHANNEL_NET)){
                if (mclone == NULL) mclone = message->clone();
                if(mChanHash.value(EVECHANNEL_NET)->queueMessage(mclone)) mclone = NULL;
            }
            /* send chainstatus to eventManager (we always have one)*/
            if (mChanHash.contains(EVECHANNEL_EVENT) && message->hasDestinationFacility(EVECHANNEL_EVENT)){
                if (mclone == NULL) mclone = message->clone();
                if(mChanHash.value(EVECHANNEL_EVENT)->queueMessage(mclone)) mclone = NULL;
            }
            if (mclone != NULL) delete mclone;
            /* send chainstatus to manager (we always have one)*/
            if (mChanHash.contains(EVECHANNEL_MANAGER) && message->hasDestinationFacility(EVECHANNEL_MANAGER)){
                if (mChanHash.value(EVECHANNEL_MANAGER)->queueMessage(message))message = NULL;
            }
            else {
                delete message;
                message = NULL;
            }
            break;
        }
        case EVEMESSAGETYPE_DATA:
        {
            eveMessage* mclone = NULL;
            /* send data to viewers if available */
            if (message->hasDestinationFacility(EVECHANNEL_NET)){
                if (mclone == NULL) mclone = message->clone();
                // if this is a normalized detector value, append normalizeId
                // we need this unless ecp is modified to specify normalizeId
                eveDataMessage* dataclone = (eveDataMessage*)mclone;
                if ((dataclone->getDataMod() != DMTunmodified) && !dataclone->getNormalizeId().isEmpty())
                    dataclone->setXmlId(dataclone->getXmlId() + "__" + dataclone->getNormalizeId());
                if (mChanHash.value(EVECHANNEL_NET)->queueMessage(mclone)) mclone = NULL;
            }
            /* measurement data will be sent to Math */
            if (message->hasDestinationFacility(EVECHANNEL_MATH)){
                if (mclone == NULL) mclone = message->clone();
                if (sendToMath(mclone)) mclone = NULL;
            }

            // use message for the last test
            if (mclone != NULL) delete mclone;
            /* send data to storagemodules if available */
            if (haveStorage() && message->hasDestinationFacility(EVECHANNEL_STORAGE)){
                if (sendToStorage(message)) message = NULL;
            }
            if (message != NULL) delete message;
            message = NULL;
            break;
        }
        case EVEMESSAGETYPE_AUTOPLAY:
        case EVEMESSAGETYPE_REPEATCOUNT:
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
        case EVEMESSAGETYPE_METADATA:
            if (haveStorage()){
                ((eveMessageText*)message)->setDestinationFacility(EVECHANNEL_STORAGE);
                if (sendToStorage(message)) message = NULL;
            }
            else {
                delete message;
                message = NULL;
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
            else {
                delete message;
                message = NULL;
            }
            break;
        case EVEMESSAGETYPE_DEVINFO:
            /* send device info to storagemodule if available */
            if (haveStorage()){
                if (sendToStorage(message)) message = NULL;
            }
            else {
                delete message;
                message = NULL;
            }
            break;
        case EVEMESSAGETYPE_EVENTREGISTER:
        case EVEMESSAGETYPE_MONITORREGISTER:
        case EVEMESSAGETYPE_STORAGEDONE:
        case EVEMESSAGETYPE_DETECTORREADY:
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
        // get the next message, if we get the lock
        if (mChanHash.value(messageSource)->getLock()){
            message = mChanHash.value(messageSource)->getMessage();
            mChanHash.value(messageSource)->releaseLock();
        }
        else {
            // we couldn't get the lock, try again later
            message = NULL;
            emit messageWaiting(messageSource);
        }
    }
}

/**
 * \brief queue an error message to be sent to connected Viewers (use only for messageHub with locked channelLock)
 * \param severity	Severity as defined in VE-Protocol-Spec
 * \param errorType errorType as defined in VE-Protocol-Spec
 * \param errorString	additional error description
 *
 */
void eveMessageHub::addError(int severity, int errorType,  QString errorString)
{
	eveError::log(severity, errorString, EVEMESSAGEFACILITY_MHUB);
	if (useNet && (loglevel >= severity) && (mChanHash.contains(EVECHANNEL_NET))) {
		eveErrorMessage *errorMessage = new eveErrorMessage(severity, EVEMESSAGEFACILITY_MHUB, errorType, errorString);
		if (!mChanHash.value(EVECHANNEL_NET)->queueMessage(errorMessage)) delete errorMessage;
	}
}

/**
 * \brief shutdown messageHub
 *
 * call shutdown method of all registered channels, start timer which calls waitUntilDone methode
 */
void eveMessageHub::close()
{

	eveError::log(DEBUG, "MessageHub shut down");
	emit closeAll();
	QTimer::singleShot(100, this, SLOT(waitUntilDone()));
}
/**
 * \brief wait until all channels have unregistered, then shutdown
 *
 */
void eveMessageHub::waitUntilDone()
{
	QReadLocker locker(&channelLock);
	if (!mChanHash.isEmpty()){
		// close nwThread last
		if (mChanHash.size()==1) emit closeNet();
		QTimer::singleShot(100, this, SLOT(waitUntilDone()));
		eveError::log(DEBUG, "eveMessageHub: still waiting for threads to shutdown");
		return;
	}
	eveError::log(DEBUG, "MessageHub shutdown, done");
	if (useGui) {
		emit closeParent();
	}
	else {
		QApplication::exit(0);
	}
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
    int channel = message->getDestinationChannel();

    // if destination == 0 => send it to all storageChannels
    // else => send it to a specific storageChannel
    if ( channel == 0 ){
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

/**
 *
 * @param message pointer to the message to be sent
 * @return true if message was put into the receiver queue or has been deleted
 */
bool eveMessageHub::sendToMath(eveMessage* message)
{
    bool retval = false;

    // send all data to corresponding math
    if (message->getType() == EVEMESSAGETYPE_DATA) {
        int chid = ((eveDataMessage*)message)->getChainId();
        if (mathChidHash.contains(chid)) {
            retval = mathChidHash.value(chid)->queueMessage(message);
        }
    }
    else if (message->getType() == EVEMESSAGETYPE_CHAINSTATUS) {
        int chid = ((eveChainStatusMessage*)message)->getChainId();
        if (mathChidHash.contains(chid)) {
            retval = mathChidHash.value(chid)->queueMessage(message);
        }
    }
    return retval;
    }
