
#include "eveMessageChannel.h"
#include "eveMessageHub.h"
#include <QThread>
#include "eveError.h"

//eveMessageChannel::eveMessageChannel(QObject *parent): QObject(parent)
eveMessageChannel::eveMessageChannel(QObject *parent)
{
	acceptInput = true;
	unregistered = false;
	connect(this, SIGNAL(messageArrived()), this, SLOT(newQueuedMessage()) ,Qt::QueuedConnection);
}

eveMessageChannel::~eveMessageChannel()
{
}

/**
 * \brief get the message from the internal send queue
 *
 * called by MessageHub to receive a message after messageWaiting has been signaled
 */
eveMessage * eveMessageChannel::getMessage()
{
	eveMessage *message=NULL;
	if (!sendMessageList.isEmpty()) {
		message = sendMessageList.takeFirst();
		emit messageTaken();
	}
	return message;
}

/**
 * \brief add a message to the internal send queue, to be fetched by messageHub
 *
 */
void eveMessageChannel::addMessage(eveMessage * message)
{
	if (!message) return;

	QWriteLocker locker(&sendLock);
	sendMessageList.append(message);

	emit messageWaiting(channelId);
}

/**
 * \brief is sendqueue empty
 * \return true if sendqueue is empty, else false
 */
bool eveMessageChannel::sendQueueIsEmpty()
{
	QWriteLocker locker(&sendLock);
	return sendMessageList.isEmpty();
}

/**
 * \brief unregister if sendqueue is empty
 * \return true if sendqueue is empty, else false
 */
bool eveMessageChannel::shutdownThreadIfQueueIsEmpty()
{
	// if already successfully unregistered return true
	if (unregistered) return true;

	QWriteLocker locker(&sendLock);
	if (sendMessageList.isEmpty()){
		eveMessageHub::getmHub()->unregisterChannel(channelId);
		unregistered = true;
		QThread::currentThread()->quit();
		return true;
	}
	else {
		eveError::log(1, QString("eveMessageChannel: %1 more Messages before shutdown").arg(sendMessageList.size()));
		emit messageWaiting(channelId);
		return false;
	}
}

/**
 * \brief put a message into the internal receive queue
 * \param message the message to be queued
 * \return true if message has been accepted else false
 *
 * called by messageHub to send a message
 *
 */
bool eveMessageChannel::queueMessage(eveMessage * message)
{

	QWriteLocker locker(&recLock);
	if (!acceptInput) return false;
	if (message->getPriority() == EVEMESSAGEPRIO_HIGH)
		receiveFastMessageList.append(message);
	else
		receiveMessageList.append(message);

	emit messageArrived();
	return true;
}
/**
 * \brief signals the arrival of a new message
 *
 */
void eveMessageChannel::newQueuedMessage()
{
	QReadWriteLock recLock;
	bool fastEmpty = true, normalEmpty = true;
	eveMessage * message;

	recLock.lockForWrite();
	if ((fastEmpty=receiveFastMessageList.isEmpty())) normalEmpty=receiveMessageList.isEmpty();
	recLock.unlock();

	while ((!fastEmpty) || (!normalEmpty)){
		recLock.lockForWrite();
		if (!fastEmpty)
			message = receiveFastMessageList.takeFirst();
		else
			message = receiveMessageList.takeFirst();
		recLock.unlock();

		handleMessage(message);

		recLock.lockForWrite();
		if ((fastEmpty=receiveFastMessageList.isEmpty())) normalEmpty=receiveMessageList.isEmpty();
		recLock.unlock();
	}
}

/**
 * \brief process message from receive queue (must be implemented by derived classes)
 *
 */
void eveMessageChannel::handleMessage(eveMessage * message)
{
	// child classes should implement method
	eveError::log(4,"eveMessageChannel::handleMessage: missing message handler for child class! \n");
}

/**
 * \brief may be implemented by derived classes
 *
 */
void eveMessageChannel::shutdown()
{
	// child classes should implement method
	eveError::log(4,"eveMessageChannel::shutdown: no child implementation, shutdown this thread \n");
	QThread::currentThread()->quit();
}

/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveMessageChannel::sendError(int severity, int facility, int errorType,  QString errorString){
	addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}
