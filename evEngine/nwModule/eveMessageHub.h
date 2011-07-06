#ifndef EVEMESSAGEHUB_H_
#define EVEMESSAGEHUB_H_

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QThread>
#include <QReadWriteLock>
#include "eveMessageChannel.h"

class eveRequestManager;

/**
 * \brief all messages are sent to message hub, which cares about delivery
 */
class eveMessageHub: public QObject
{
    Q_OBJECT

public:
	eveMessageHub(bool);
	virtual ~eveMessageHub();
	int registerChannel(eveMessageChannel *, int);
	void unregisterChannel(int);
	void log(QString);
	void init();
	static eveMessageHub* getmHub();

public slots:
	void newMessage(int);
	void close();
	void waitUntilDone();

signals:
	void finished();
	void closeAll();
	void closeParent();
	void messageWaiting(int);

private:
	void addError(int, int,  QString);
	bool sendToStorage(eveMessage*);
	bool sendToMath(eveMessage*);
	bool haveStorage(){return !storageChannelList.isEmpty();};
	int scanChannelCounter;
	int nextChannel;
	QThread* nwThread;
	QThread* managerThread;
	QThread* eventThread;
	eveMessageChannel * netChannel;
	QHash<int, eveMessageChannel * > mChanHash;
//	QList<eveMessageChannel> scanChannels;
	QList<int> storageChannelList;
	QList<int> mathChannelList;
	static eveMessageHub* mHub;
	int engineStatus;
	QString currentXmlId;
	eveRequestManager *reqMan;
	QReadWriteLock channelLock;
	bool useGui;


};

#endif /*EVEMESSAGEHUB_H_*/
