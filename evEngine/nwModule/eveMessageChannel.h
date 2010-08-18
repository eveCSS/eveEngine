#ifndef EVEMESSAGECHANNEL_H_
#define EVEMESSAGECHANNEL_H_

// predefined (reserved) Channels
#define EVECHANNELS_RESERVED 5
#define EVECHANNEL_NET 1
#define EVECHANNEL_STORAGE 2
#define EVECHANNEL_MANAGER 3
#define EVECHANNEL_EVENT 4
#define EVECHANNEL_MATH 5


#include <QObject>
#include <QReadWriteLock>
#include <eveMessage.h>

/**
 * \brief messageChannel is a base class to communicate with message hub
 *
 * sending messages from messageHub to MessageChannel:
 *   - messageHub sends the message by calling queueMessage.
 *   - queueMessage emits messageArrived which calls newQueuedMessage in the
 *     MessageChannel-thread-context
 *
 * sending messages from MessageChannel to messageHub:
 *   - MessageChannel calls addMessage
 *   - addMessage sends the messageWaiting signal to messageHub which makes messageHub
 *     to get the message by calling getMessage in the messageHub-thread-context
 */
class eveMessageChannel: public QObject
{
    Q_OBJECT

public:
	eveMessageChannel(QObject * parent=0);
	virtual ~eveMessageChannel();

	virtual eveMessage * getMessage();
	virtual bool queueMessage(eveMessage *);
	virtual void addMessage(eveMessage * message);
	bool sendQueueIsEmpty();
	void enableInput(){QWriteLocker locker(&recLock); acceptInput=true;};
	void disableInput(){QWriteLocker locker(&recLock); acceptInput=false;};

public slots:
	virtual void newQueuedMessage();
	virtual void shutdown();

signals:
	void messageWaiting(int);
	void messageArrived();
	void messageTaken();

protected:
	virtual void handleMessage(eveMessage*);
	bool unregisterIfQueueIsEmpty();
	int channelId;

private:
	int status;
	bool acceptInput;
	QList<eveMessage*> sendMessageList;
	QList<eveMessage*> receiveMessageList;
	QList<eveMessage*> receiveFastMessageList;
	QReadWriteLock sendLock;
	QReadWriteLock recLock;

};

#endif /*EVEMESSAGECHANNEL_H_*/
