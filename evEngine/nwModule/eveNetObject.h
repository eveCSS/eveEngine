#ifndef EVENETOBJECT_H_
#define EVENETOBJECT_H_

#include <QSet>
#include <QString>
#include <QTcpServer>
#include "eveMessageChannel.h"
#include "eveMessageFilter.h"
class eveSocket;

/**
 *  \brief manages network connections
 *
 *  registers with messageHub starts a listener thread and
 *  manages all incoming network connections
 *
 */
class eveNetObject : public eveMessageChannel
{
	Q_OBJECT

public:
	eveNetObject();
	virtual ~eveNetObject();
	void removeSocket(eveSocket*);
//	void log(QString);
	void handleMessage(eveMessage *);
	void sendMessage(eveMessage *);
	void sendError(int, int, int, QString);

signals:
	void initDelayed();

public slots:
	void shutdown();

private slots:
	void init();
	void acceptSocket();

private:
	bool use_net;
	bool shutdownPending;
	eveMessageFilter *mFilter;
	QTcpServer * netListener;
	QSet<eveSocket*> socketList;
};

#endif /*EVENETOBJECT_H_*/
