#ifndef EVESOCKET_H_
#define EVESOCKET_H_

#define EVESOCKET_TIMEOUT 2000 		// timeout in msecs

#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include "eveNetObject.h"

class eveNetObject;
class eveMessage;

class eveSocket: public QObject
{
    Q_OBJECT

public:
	eveSocket(QTcpSocket *, eveNetObject *);
	virtual ~eveSocket();

public slots:
	void readMessage();
	void deleteSocket();
	void disconnectSocket();
	void timeout();
	void sendMessage(QByteArray *);

private:
	void addError(int , int ,  QString );

	bool messageInProgress;
	bool outOfSync;
	QByteArray startTag;
	QTimer *timeoutClock;
	QTcpSocket *socket;
	eveNetObject *netObject;
	QByteArray * messageBuffer;
	qint64 bytesWaiting;
    struct {
    	quint32	starttag;
    	quint16 version;
    	quint16 type;
    	quint32 length;
    }header;

	
};

#endif /*EVESOCKET_H_*/
