#ifndef CMDCLIENT_H
#define CMDCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QAbstractSocket>
#include "eveMessage.h"

class Cmdclient : public QObject
{
    Q_OBJECT

public:
    explicit Cmdclient(QObject *parent = 0);
    virtual ~Cmdclient();
    void addToPlayList(QString);
    void sendMessage(eveMessage *);
    void addMessage(eveMessage *);

public slots:
    void readMessage();
    void deleteSocket();
    void timeout();
    void displayError(QAbstractSocket::SocketError);
    void socketConnected();
    void disconnectSocket();

private:
    QString getAuthor();
    bool messageInProgress;
    bool outOfSync;
    bool singleFile;
    bool setRepeatCount;
    bool allSent;
    int playListCounter;
    QByteArray startTag;
    QTimer* timeoutClock;
    QTcpSocket* tcpSocket;
    QByteArray * messageBuffer;
    struct {
        quint32	starttag;
        quint16 version;
        quint16 type;
        quint32 length;
    }header;

};

#endif // CMDCLIENT_H
