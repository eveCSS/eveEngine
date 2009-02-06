
#include <QThread>
#include "eveMessageChannel.h"
#include "eveError.h"

//eveMessageChannel::eveMessageChannel(QObject *parent): QObject(parent)
eveMessageChannel::eveMessageChannel(QObject *parent)
{
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
	QWriteLocker locker(&sendLock);

	if (sendMessageList.isEmpty()){
		return NULL;
	}
	return sendMessageList.takeFirst();
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
 * \brief put a message into the internal receive queue
 * \param message the message to be queued
 *
 * called by messageHub to send a message
 *
 */
void eveMessageChannel::queueMessage(eveMessage * message)
{
	QWriteLocker locker(&recLock);
	if (message->getPriority() == EVEMESSAGEPRIO_HIGH)
		receiveFastMessageList.append(message);
	else
		receiveMessageList.append(message);

	emit messageArrived();
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
