
#include <QDataStream>
#include "eveSocket.h"
#include "eveMessageFactory.h"
#include "eveNetObject.h"
#include "eveError.h"

eveSocket::eveSocket(QTcpSocket * tcpsocket, eveNetObject *netOb)
{
	netObject = netOb;
	socket = tcpsocket;
	outOfSync = false;
	messageInProgress = false;
	connect(socket, SIGNAL(disconnected()), this, SLOT(deleteSocket()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(readMessage()));
    timeoutClock = new QTimer(this);
    timeoutClock->setSingleShot(true);
    connect(timeoutClock, SIGNAL(timeout()), this, SLOT(timeout()));
    timeoutClock->setInterval(EVESOCKET_TIMEOUT);
    startTag.resize(4);
    startTag[0] = EVEMESSAGE_STARTTAG >> 24 & 0xff;
    startTag[1] = EVEMESSAGE_STARTTAG >> 16 & 0xff;
    startTag[2] = EVEMESSAGE_STARTTAG >> 8 & 0xff;
    startTag[3] = EVEMESSAGE_STARTTAG & 0xff;
}

eveSocket::~eveSocket()
{
	disconnect(socket, SIGNAL(disconnected()), this, SLOT(deleteSocket()));
}

void eveSocket::deleteSocket()
{
	socket->disconnectFromHost();
	netObject->removeSocket(this);
	this->deleteLater();
}

void eveSocket::readMessage()
{
    QDataStream instream(socket);
    instream.setVersion(QDataStream::Qt_4_0);

	if (outOfSync){
    	quint8 dummyBuffer;
    	while (socket->bytesAvailable() > 3) {
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
    	if (socket->bytesAvailable() > 3) {
    		if (!instream.device()->peek(4).startsWith(startTag)){
            	eveError::log(4,QString("ECP Error: out of sync!, new message with unknown starttag"));
            	outOfSync=true;
            	readMessage();
    			return;
    		}
    	}
    	timeoutClock->start();
    	bytesWaiting = socket->bytesAvailable();
    	if (bytesWaiting < (int)sizeof(header)) return;

        instream >> header.starttag >> header.version >> header.type >> header.length ;

        // check if version, type and length are valid
        if (!eveMessageFactory::validate(header.version, header.type, header.length)){
    		addError(4,0,QString("ECP Error: version 0x%1, type 0x%2, length %3").arg(header.version,4,16).arg(header.type,4,16).arg(header.length));
        	timeoutClock->stop();
        	if (socket->bytesAvailable()) readMessage();
        	return;
        }
		//allocate buffer
		messageBuffer = new QByteArray();
		messageInProgress=true;
		eveError::log(1,QString("new message coming in: length %1").arg(header.length));
   }

    if (messageInProgress) {
    	//restart the Timeout Clock
    	timeoutClock->start();
    	bytesWaiting = socket->bytesAvailable();
    	if (bytesWaiting < header.length) return;

    	// read from socket into current buffer
		QByteArray socketBuffer = socket->read(header.length);
    	messageBuffer->append(socketBuffer);
    	netObject->addMessage(eveMessageFactory::getNewMessage(header.type, header.length, messageBuffer));
    	timeoutClock->stop();
    	delete messageBuffer;
    	messageBuffer = 0;
    	messageInProgress=false;
    	if (socket->bytesAvailable() > 0) readMessage();
    }
}
void eveSocket::sendMessage(QByteArray *messageStream)
{
	if (messageStream->length() > 0){
		if (socket->write(*messageStream) != messageStream->length())
			eveError::log(4,"eveSocket::sendMessage: unable to send buffer");
		//else
		//	eveError::log(4,QString("eveSocket::sendMessage: sent buffer with %1 bytes").arg(messageStream->length()));
	}
	else {
		eveError::log(4,"eveSocket::sendMessage: not sending messages with 0 bytes ");
	}
}

void eveSocket::timeout(){

	addError(ERROR, EVEERROR_TIMEOUT,"Timeout while receiving message, message skipped");
	// throw away partial message
	if (bytesWaiting > 0) QByteArray socketBuffer = socket->read(bytesWaiting);

	if (messageInProgress) {
    	delete messageBuffer;
    	messageBuffer=0;
    	messageInProgress=false;
	}
}
void eveSocket::addError(int severity, int errorType,  QString errorString){

	netObject->addMessage(new eveErrorMessage(severity, EVEMESSAGEFACILITY_NETWORK, errorType, errorString));
}
