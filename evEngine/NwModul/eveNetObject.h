#ifndef EVENETOBJECT_H_
#define EVENETOBJECT_H_

#include <QSet>
#include <QString>
#include <QTcpServer>
#include "eveMessageChannel.h"
#include "eveMessageHub.h"
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
	void log(QString);
	void handleMessage(eveMessage *);
	void sendMessage(eveMessage *);

signals:
	void initDelayed();

public slots:
	void shutdown();

private slots:
	void init();
	void acceptSocket();

private:
	bool use_net;
	eveMessageFilter *mFilter;
	eveMessageHub * mHub;
	QTcpServer * netListener;
	QSet<eveSocket*> socketList;
};

#endif /*EVENETOBJECT_H_*/
