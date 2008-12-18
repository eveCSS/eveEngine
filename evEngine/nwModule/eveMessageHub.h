#ifndef EVEMESSAGEHUB_H_
#define EVEMESSAGEHUB_H_

#include <QObject>
#include <QHash>
#include <QString>
#include <QReadWriteLock>
#include "eveMessageChannel.h"
#include "eveNwThread.h"
#include "eveManagerThread.h"

class eveRequestManager;

/**
 * \brief all messages are sent to message hub, which cares about delivery
 */
class eveMessageHub: public QObject
{
    Q_OBJECT

public:
	eveMessageHub();
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

private:
	void addError(int, int,  QString);
	int scanChannelCounter;
	int nextChannel;
	eveNwThread *nwThread;
	eveManagerThread * mThread;
	eveMessageChannel * netChannel;
	QHash<int, eveMessageChannel * > mChanHash;
//	QList<eveMessageChannel> scanChannels;
	static eveMessageHub* mHub;
	int engineStatus;
	QString currentXmlId;
	eveRequestManager *reqMan;
	QReadWriteLock channelLock;



};

#endif /*EVEMESSAGEHUB_H_*/
