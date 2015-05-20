/*
 * eveStorageManager.cpp
 *
 *  Created on: 28.08.2009
 *      Author: eden
 */

#include <QTimer>
#include <QThread>
#include <QStringList>
#include "eveRequestManager.h"
#include "eveMessageHub.h"
#include "eveStorageManager.h"
#include "eveDataCollector.h"
#include "eveError.h"
#include "eveParameter.h"
#include "eveXMLReader.h"

eveStorageManager::eveStorageManager(QString filename, int chainId, eveXMLReader* parser, QByteArray* xmldata) {

    confirmSaveRid = 0;
    fileName = filename;
    shutdownPending = false;
    delayedStatus = NULL;
    channelId = eveMessageHub::getmHub()->registerChannel(this, EVECHANNEL_STORAGE);

    // collect all storage-related data
    QHash<QString, QString> pluginHash = parser->getChainPlugin(chainId, "saveplugin");
    addToHash(pluginHash, chainId, "autonumber", parser);
    addToHash(pluginHash, chainId, "savescandescription", parser);
    addToHash(pluginHash, chainId, "comment", parser);
    addToHash(pluginHash, chainId, "confirmsave", parser);
    pluginHash.insert("filename", filename);

    dc = new eveDataCollector(this, pluginHash, xmldata);
    QString param = eveParameter::getParameter("version");
    dc->addMetaData(0, "Version", param);
    param = eveParameter::getParameter("xmlversion");
    dc->addMetaData(0, "XMLversion", param);
    param = eveParameter::getParameter("location");
    dc->addMetaData(0, "Location", param);
}

eveStorageManager::~eveStorageManager() {
}

/**
 * \brief process messages sent to eveStorageManager
 * @param message pointer to the message which eveStorageManager received
 */
void eveStorageManager::handleMessage(eveMessage *message){
    
    if (shutdownPending){
        delete message;
        return;
    }
    
    switch (message->getType()) {
    case EVEMESSAGETYPE_STORAGECONFIG:
        sendError(DEBUG,0,"eveStorageManager::handleMessage: got STORAGECONFIG-message");
        if (((eveStorageMessage*)message)->getFileName() == fileName){
            if (!configStorage((eveStorageMessage*)message))
                sendError(ERROR,0,QString("eveStorageManager::handleMessage: unable to init StorageObject for File %1 (%2)").arg(fileName).arg(message->getType()));
        }
        else {
            sendError(ERROR,0,QString("eveStorageManager::handleMessage: wrong Filename %1, expected %2").arg(((eveStorageMessage*)message)->getFileName()).arg(fileName));
        }
        break;
    case EVEMESSAGETYPE_CHAINSTATUS:
      if (((eveChainStatusMessage*)message)->getChainStatus() == CHStatusMATHDONE){
        int id = ((eveChainStatusMessage*)message)->getChainId();
        if (chainIdChannelHash.remove(id) == 0){
          sendError(DEBUG,0,QString("handleMessage: unable to remove not existing chainId %1 from chainList").arg(id));
        }
        else {
          delayedStatus = new eveChainStatusMessage(id, CHStatusSTORAGEDONE);
          delayedStatus->addDestinationFacility(EVECHANNEL_NET | EVECHANNEL_MANAGER);
          if (chainIdChannelHash.isEmpty()) {
            // no chains left, shut down
            if (!dc->isConfirmSave()){
              initShutdown();
              eveError::log(DEBUG, QString("eveStorageManager: shutdown, send StorageDone message"));
              addMessage(delayedStatus);
              delayedStatus = NULL;
              addMessage(new eveMessageInt(EVEMESSAGETYPE_STORAGEDONE, channelId));
              shutdown();
            }
            else {
              confirmSaveRid = eveRequestManager::getRequestManager()->newId(channelId);
              addMessage(new eveRequestMessage(confirmSaveRid, EVEREQUESTTYPE_YESNO, "Keep the data file?"));
            }
          }
          else {
            addMessage(delayedStatus);
            delayedStatus = NULL;
          }
        }
      }
      break;
    case EVEMESSAGETYPE_DATA:
    {
       int id = ((eveDataMessage*)message)->getChainId();
       if ((id == 0) || chainIdChannelHash.contains(id)){
            dc->addData((eveDataMessage*)message);
        }
        else
            sendError(ERROR, 0, QString("handleMessage: received data message with invalid chainId: %1").arg(id));
    }
        break;
    case EVEMESSAGETYPE_DEVINFO:
    {
        sendError(DEBUG, 0, QString("Device Info: (%1/%2) Name: %3 (%4), %5").arg(((eveDevInfoMessage*)message)->getChainId())
                  .arg(((eveDevInfoMessage*)message)->getSmId())
                  .arg(((eveDevInfoMessage*)message)->getName())
                  .arg(((eveDevInfoMessage*)message)->getXmlId())
                  .arg((((eveDevInfoMessage*)message)->getText())->value(0, QString())));
        int id = ((eveDevInfoMessage*)message)->getChainId();
        if (chainIdChannelHash.contains(id)){
            sendError(DEBUG, 0, "found DataCollector, calling with new device info");
            dc->addDevice((eveDevInfoMessage*)message);
        }
        else
            sendError(ERROR, 0, QString("handleMessage: received data message with invalid chainId: %1").arg(id));
    }
        break;
    case EVEMESSAGETYPE_LIVEDESCRIPTION:
        sendError(DEBUG, 0, QString("got Livedescription: %1").arg(((eveMessageText*)message)->getText()));
        dc->addMetaData(0, QString("Live-Comment"), ((eveMessageText*)message)->getText());
        break;
    case EVEMESSAGETYPE_METADATA:
    {
        int id = ((eveMessageTextList*)message)->getChainId();
        QStringList strlist = ((eveMessageTextList*)message)->getText();
        while (strlist.size() > 1){
            QString attribute = strlist.takeFirst();
            QString strval = strlist.takeFirst();
            if (attribute.isEmpty()) continue;
            if ((id == 0) || chainIdChannelHash.contains(id)){
                sendError(DEBUG, 0, QString("sending attribute %1: %2, id: %3").arg(attribute).arg(strval).arg(id));
                dc->addMetaData(id, attribute, strval);
            }
            else
                sendError(ERROR, 0, QString("handleMessage: received metadata message with invalid chainId: %1").arg(id));
        }
    }
        break;
    case EVEMESSAGETYPE_REQUESTANSWER:
    {
        int reqType = ((eveRequestAnswerMessage*)message)->getReqType();
        if ((((eveRequestAnswerMessage*)message)->getReqId() == confirmSaveRid) &&
                ((reqType == EVEREQUESTTYPE_YESNO) || (reqType == EVEREQUESTTYPE_OKCANCEL))){
            dc->setKeepFile(((eveRequestAnswerMessage*)message)->getAnswerBool());
            initShutdown();
            addMessage(delayedStatus);
            delayedStatus = NULL;
            addMessage(new eveMessageInt(EVEMESSAGETYPE_STORAGEDONE, channelId));
            shutdown();
        }
    }
        break;
    default:
        sendError(ERROR,0,QString("eveStorageManager::handleMessage: unknown message, type: %1").arg(message->getType()));
        break;
    }
    delete message;
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveStorageManager::sendError(int severity, int errorType,  QString errorString){
    sendError(severity, EVEMESSAGEFACILITY_STORAGE, errorType, errorString);
}
/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveStorageManager::sendError(int severity, int facility, int errorType,  QString errorString){
    addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}

/**
 * \brief shutdown StorageManager and this thread
 */
void eveStorageManager::shutdown(){

    eveError::log(DEBUG, QString("eveStorageManager: shutdown"));
    if (!shutdownPending) initShutdown();

    // make sure mHub reads all outstanding messages before closing the channel
    shutdownThreadIfQueueIsEmpty();
}

/**
 * \brief shutdown StorageManager and this thread
 */
void eveStorageManager::initShutdown(){

    if (!shutdownPending) {
        disableInput();
        eveError::log(DEBUG, QString("eveStorageManager::initShutdown"));
        sendError(DEBUG, 0, QString("eveStorageManager: initShutdown"));
        shutdownPending = true;
        delete dc;
        eveError::log(DEBUG, QString("eveStorageManager::initShutdown dc deleted"));
        connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
    }
}
/**
 * \brief create a DataCollector for the chain of this message
 * @param message storageConfiguration message from scanmodule chain
 * @return true if initialization was successful
 */
bool eveStorageManager::configStorage(eveStorageMessage* message){

    int chainId = message->getChainId();
    if ((chainId > 0) && !chainIdChannelHash.contains(chainId)){
        chainIdChannelHash.insert(chainId, message->getChannelId());
        dc->addChain(message);
        // TODO add the parameters from  message->getHash()
        return true;
    }
    return false;
}

void eveStorageManager::addToHash(QHash<QString, QString>& hash, int chainId, QString key, eveXMLReader* parser){

    QString value = parser->getChainString(chainId, key);
    if (!value.isEmpty()) hash.insert(key, value);

}


