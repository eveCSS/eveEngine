
#include <QTcpSocket>
#include <QTimer>

#include "eveNetObject.h"
#include "eveSocket.h"
#include "eveError.h"
#include "eveMessageHub.h"
#include "eveMessageFactory.h"
#include "eveParameter.h"

eveNetObject::eveNetObject()
{
    connect(this, SIGNAL(initDelayed()), this, SLOT(init()), Qt::QueuedConnection);
    emit initDelayed();
    channelId=EVECHANNEL_NET;
    use_net = false;
    mFilter = NULL;
    netListener = NULL;
    shutdownPending = false;
    if (eveParameter::getParameter("use_network") == "yes") use_net = true;
}

eveNetObject::~eveNetObject()
{
	// most work has been done in shutdown()
	if (netListener) delete netListener;
	if (mFilter) delete mFilter;
}

/* \brief init, called with delay by constructor
 *
 *  if running with network: starts a TCP-Listener and
 *  registers with messageHub
 */
void eveNetObject::init(){

    // TODO
    if (eveParameter::getParameter("interfaces") != "all")
        eveError::log(MINOR, "eveNetObject::init: selecting specific interfaces not yet implemented");

    disconnect(this, SIGNAL(initDelayed()), this, SLOT(init()));

    if (use_net){
        // register with mHub if not already done
        eveMessageHub::getmHub()->registerChannel(this, channelId);
        mFilter = new eveMessageFilter(this);

        qint32 testport = eveParameter::getParameter("port").toInt();

        if ((testport > 65535) || (testport < 1024)) {
            eveError::log(ERROR, QString("invalid port %1, must be in range 1024 -> 65535").arg(testport));
            addMessage(new eveMessage(EVEMESSAGETYPE_ENDPROGRAM));
        }
        else {
            quint16 port = eveParameter::getParameter("port").toUShort();

            // create a TCP-Listener Object
            netListener = new QTcpServer(this);
            connect(netListener, SIGNAL(newConnection()), this, SLOT(acceptSocket()));

            if (!netListener->listen(QHostAddress::Any, port )){
                eveError::log(ERROR, QString("Error listening on port %1; Error: %2").arg(port).arg(netListener->errorString()));
                addMessage(new eveMessage(EVEMESSAGETYPE_ENDPROGRAM));
            }
            else
                eveError::log(DEBUG, QString("listening on port %1").arg(port));
        }
    }
}

/* \brief add a socket to the list
 *
 *  called if a new connection request has been received by TCP-Listener
 *  sends cached messages if any
 */
void eveNetObject::acceptSocket(){

	QTcpSocket *newSocket = netListener->nextPendingConnection();

	if (newSocket != 0) {
		eveSocket* eSocket = new eveSocket(newSocket, this);
		socketList.insert(eSocket);
        eveError::log(DEBUG, QString("eveNetObject: accepted socket"));
        eSocket->sendMessage(eveMessageFactory::getNewStream(new eveVersionMessage()));
        QList<eveMessage * > * calist = mFilter->getCache();
		foreach (eveMessage * message, *calist){
			QByteArray * sendByteArray = eveMessageFactory::getNewStream(message);
			eSocket->sendMessage(sendByteArray);
		}
		delete calist;
	}
}

/* \brief remove a socket from the list
 * \param socket the socket to be removed
 *
 */
void eveNetObject::removeSocket(eveSocket* socket){

	if (socket != 0) {
		socketList.remove(socket);
		eveError::log(DEBUG, QString("eveNetObject: removed socket"));
	}
}

/* \brief process current message
 * \param message current message
 *
 * call message filter, clear cached messages if engine is ready
 *
 */
void eveNetObject::handleMessage(eveMessage * message)
{
	if (!message) return;

	// clear the message cache when the scan is done
	if ((message->getType() == EVEMESSAGETYPE_ENGINESTATUS) &&
			(((eveEngineStatusMessage*)message)->getStatus() == eveEngIDLENOXML))
			mFilter->clearCache();

	bool cancelled = mFilter->checkMessage(message);
	if (cancelled) {
		delete message;
	}
	else {
		sendMessage(message);
	}

	return;
}

/**
 * \brief convert message to a stream, send the stream and call messageFilter to cache or delete message
 * \param message current message
 *
 * get a new message stream, send message and give message to filter for deletion or caching.
 * Usually called by messageFilter after filtering.
 */
void eveNetObject::sendMessage(eveMessage * message)
{
	if (!message) return;

	QByteArray * sendByteArray = eveMessageFactory::getNewStream(message);

	foreach (eveSocket *sock, socketList) {
		sock->sendMessage(sendByteArray);
	};
	delete sendByteArray;
	mFilter->queueMessage(message);
}

/**
 * \brief shutdown and close all pending connections
 *
 */
void eveNetObject::shutdown(){

	if (!shutdownPending){
		shutdownPending = true;
		eveError::log(DEBUG, "NetObject shutdown", EVEMESSAGEFACILITY_NETWORK);
        if (netListener) netListener->close();
		disableInput();
		eveMessageHub::getmHub()->unregisterChannel(channelId);
		foreach (eveSocket *sock, socketList) {
			sock->disconnectSocket();
		}
	}
	if (receiveQueueIsEmpty()){
		QThread::currentThread()->quit();
	}
	else {
		// call this again
		QTimer::singleShot(500, this, SLOT(shutdown()));
	}
}

//void eveNetObject::log(QString string){
//	eveError::log(INFO, string);
//}

void eveNetObject::sendError(int severity, int facility, int errorType,  QString errorString){
	// bypass filter and send error Message
	eveError::log(severity, errorString, EVEMESSAGEFACILITY_NETWORK);
	sendMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}
