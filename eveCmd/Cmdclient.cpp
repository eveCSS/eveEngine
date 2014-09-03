#include <QtCore/QCoreApplication>
#include "Cmdclient.h"
#include "eveParameter.h"
#include "eveError.h"
#include "eveMessageFactory.h"
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QProcess>
#include <QFile>

Cmdclient::Cmdclient(QObject *parent) :
    QObject(parent)
{

    outOfSync = false;
    messageInProgress = false;
    singleFile = false;
    setRepeatCount = false;
    allSent = false;
    playListCounter = 0;
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
               this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readMessage()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(deleteSocket()));
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
    timeoutClock = new QTimer(this);
    connect(timeoutClock, SIGNAL(timeout()), this, SLOT(timeout()));

    startTag.resize(4);
    startTag[0] = EVEMESSAGE_STARTTAG >> 24 & 0xff;
    startTag[1] = EVEMESSAGE_STARTTAG >> 16 & 0xff;
    startTag[2] = EVEMESSAGE_STARTTAG >> 8 & 0xff;
    startTag[3] = EVEMESSAGE_STARTTAG & 0xff;

    tcpSocket->abort();
    eveError::log(INFO, QString("connecting to host %1").arg(eveParameter::getParameter("host")));
    tcpSocket->connectToHost(eveParameter::getParameter("host"),eveParameter::getParameter("port").toInt());

}

Cmdclient::~Cmdclient()
{
        delete timeoutClock;
        delete tcpSocket;
}

void Cmdclient::displayError(QAbstractSocket::SocketError socketError)
{
        switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            eveError::log(ERROR,QString("Host %1 (port %2) was not found, check hostname / portnumber").arg(eveParameter::getParameter("host")).arg(eveParameter::getParameter("port")));
            break;
        case QAbstractSocket::ConnectionRefusedError:
            eveError::log(ERROR,QString("Host %1 refused connection, check hostname / portnumber, make sure application is running").arg(eveParameter::getParameter("host")));
            break;
        default:
            eveError::log(ERROR,QString("Network Error %1 ").arg(tcpSocket->errorString()));
        }

}

void Cmdclient::deleteSocket()
{
    eveError::log(ERROR,QString("Socket has been disconnected"));
    QCoreApplication::exit(0);
}

void Cmdclient::readMessage()
{
    QDataStream instream(tcpSocket);
    instream.setVersion(QDataStream::Qt_4_0);

        if (outOfSync){
        quint8 dummyBuffer;
        while (tcpSocket->bytesAvailable() > 3) {
                if (instream.device()->peek(4).startsWith(startTag)){
                        outOfSync = false;
                        readMessage();
                        return;
                }
                instream >> dummyBuffer;
        }
        return;
    }

    if (!messageInProgress) {
        if (tcpSocket->bytesAvailable() > 3) {
            if (!instream.device()->peek(4).startsWith(startTag)){
                eveError::log(INFO, QString("ECP Error: out of sync!, new message with unknown starttag"));
                outOfSync=true;
                readMessage();
                return;
            }
        }
        int bytesAvail = tcpSocket->bytesAvailable();
        if (bytesAvail < (int)sizeof(header)) return;

        instream >> header.starttag >> header.version >> header.type >> header.length ;

        // check if version, type and length are valid
        if (!eveMessageFactory::validate(header.version, header.type, header.length)){
            eveError::log(ERROR, QString("ECP Error: version 0x%1, type 0x%2, length %3").arg(header.version,4,16).arg(header.type,4,16).arg(header.length));
            timeoutClock->stop();
            if (tcpSocket->bytesAvailable()) readMessage();
            return;
        }
        //allocate buffer
        messageBuffer = new QByteArray();
        messageInProgress=true;
        eveError::log(DEBUG,QString("new message coming in: length %1").arg(header.length));
    }

    if (messageInProgress) {
        //restart the Timeout Clock
        timeoutClock->start();

        if (tcpSocket->bytesAvailable() < header.length) return;

        // read from instream into current buffer
        QByteArray socketBuffer = tcpSocket->read(header.length);
        //if (messageBuffer) instream >> (*messageBuffer);
        messageBuffer->append(socketBuffer);
        addMessage(eveMessageFactory::getNewMessage(header.type, header.length, messageBuffer));
        delete messageBuffer;
        messageBuffer = 0;
        messageInProgress=false;
        timeoutClock->stop();
        if (tcpSocket->bytesAvailable() > 0) readMessage();
    }
}
void Cmdclient::sendMessage(eveMessage *message)
{
        QByteArray * sendByteArray = eveMessageFactory::getNewStream(message);
        if (tcpSocket->write(*sendByteArray) != sendByteArray->length())
                eveError::log(ERROR,"Cmdclient::sendMessage: unable to send buffer");
}

void Cmdclient::timeout(){

    if (messageInProgress) {

        if (messageBuffer) delete messageBuffer;
        messageInProgress=false;
        timeoutClock->stop();
        eveError::log(ERROR,"Error: Cmdclient: Timeout while receiving message, message skipped\n");
    }

}

void Cmdclient::addMessage(eveMessage* message){

    if (message == NULL ) return;

    switch (message->getType()) {
    case EVEMESSAGETYPE_ERROR:
    {
        eveErrorMessage* emesg = (eveErrorMessage*)message;
        if (emesg->getSeverity() > 0) eveError::log(emesg->getSeverity(), emesg->getErrorText(), emesg->getFacility());
    }
        break;
    case EVEMESSAGETYPE_ENGINESTATUS:
    {
        eveEngineStatusMessage* statusmsg = (eveEngineStatusMessage*)message;
        if (setRepeatCount) {
            unsigned int rc = statusmsg->getStatus() >> 16;
            eveError::log(INFO,QString("RepeatCount %1").arg(rc));
            if ((int)rc == eveParameter::getParameter("repeatCount").toInt()) setRepeatCount = false;
        }
    }
        break;
    case EVEMESSAGETYPE_PLAYLIST:
        ++playListCounter;
        eveError::log(DEBUG,QString("Playlistcount is %1").arg(((evePlayListMessage*) message)->getCount()));
        if (singleFile && (playListCounter > 1)){
            eveError::log(INFO,QString("Playlistcount is %1").arg(((evePlayListMessage*) message)->getCount()));
            singleFile = false;
        }
        break;
    }
    if (allSent && !singleFile && !setRepeatCount) QTimer::singleShot(10, this, SLOT(disconnectSocket()));
    delete message;
}

void Cmdclient::addToPlayList(QString fileName){

    if (!tcpSocket->isWritable()) return;
    QDataStream out(tcpSocket);
    out.setVersion(QDataStream::Qt_4_0);

    QString author = getAuthor();

    if (!fileName.isEmpty())
    {
        QFile xmldata(fileName);
        if (xmldata.open(QFile::ReadOnly)) {
            QByteArray in = xmldata.readAll();
            sendMessage(new eveAddToPlMessage(fileName, author, in));
        }
        else {
            eveError::log(ERROR, QString("Error: addToPlayList: unable to open file %1").arg(fileName));
            return;
        }
    }
    else {
        eveError::log(ERROR,"Error: Cmdclient::addToPlayList: empty file name");
        return;
    }
}

void Cmdclient::socketConnected(){

    eveError::log(DEBUG,"Socket connected, start working");

    QString filename = eveParameter::getParameter("filename");
    if (filename.length() > 0) {
        singleFile = true;
        addToPlayList(filename);
    }
    int repCount = eveParameter::getParameter("repeatCount").toInt();
    if (repCount >= 0) {
        setRepeatCount = true;
        sendMessage(new eveMessageInt(EVEMESSAGETYPE_REPEATCOUNT, repCount));
    }
    allSent = true;
    QTimer::singleShot(2000, this, SLOT(disconnectSocket()));
}

void Cmdclient::disconnectSocket(){
    eveError::log(DEBUG,"disconnecting Socket");
    tcpSocket->disconnectFromHost();
}

QString Cmdclient::getAuthor() {
    QString author = QString("unknown");

//    recommended for recent qt versions
//    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
//    if (env.value("USER").length() > 0) author = env.value("USER");
//    if (env.value("HOST").length() > 0) author += QString("@") + env.value("HOST");

    QStringList environment = QProcess::systemEnvironment();
    foreach (QString line, environment){
        if (line.startsWith("USER=")) author=line.remove(0,5);
    }
    foreach (QString line, environment){
        if (line.startsWith("HOST=")) author += QString("@") + line.remove(0,5);
    }
    return author;
}

