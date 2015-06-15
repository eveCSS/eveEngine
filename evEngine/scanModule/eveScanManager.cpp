/*
 * eveScanManager.cpp
 *
 *  Created on: 29.09.2008
 *      Author: eden
 */

#include <QTimer>
#include <QTime>
#include <QDate>
#include "eveScanThread.h"
#include "eveStartTime.h"
#include "eveScanManager.h"
#include "eveScanModule.h"
#include "eveMessageHub.h"
#include "eveError.h"
#include "eveEventProperty.h"
#include "eveEventRegisterMessage.h"
#include "eveRequestManager.h"

/**
 *
 * @param parser current XML parser object
 * @param chainid our chain id
 *
 */
eveScanManager::eveScanManager(eveManager *parent, eveXMLReader *parser, int chainid, int storageChan) :
    eveMessageChannel(parent)
{
    QDomElement rootElement;

    chainId = chainid;
    storageChannel = storageChan;
    posCounter = 0;
    totalPositions = 1;
    useStorage = false;
    manager = parent;
    shutdownPending = false;
    isStarted = false;
    nextEventId = chainId*100;
    // TODO register global events

    // TODO check startevent

    //parser->addScanManager(chainId, this);
    int rootId = parser->getRootId(chainId);
    if (rootId == 0)
        sendError(ERROR,0,"eveScanManager::eveScanManager: no root scanmodule found");
    else {
        // walk down the tree and create all ScanModules
        rootSM = new eveScanModule(this, parser, chainId, rootId, eveSmTypeROOT);
        connect (rootSM, SIGNAL(SMready()), this, SLOT(smDone()), Qt::QueuedConnection);
    }

    // register with messageHub
    channelId = eveMessageHub::getmHub()->registerChannel(this, 0);

    connect (manager, SIGNAL(startSMs()), this, SLOT(smStart()), Qt::QueuedConnection);
    connect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()), Qt::QueuedConnection);
    connect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()), Qt::QueuedConnection);
    connect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()), Qt::QueuedConnection);
    connect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()), Qt::QueuedConnection);

    // collect various data
    addToHash(chainHash, "confirmsave", parser);


    eventList = parser->getChainEventList(chainId);

    // collect all storage-related data
    savePluginHash = parser->getChainPlugin(chainId, "saveplugin");
    addToHash(savePluginHash, "savefilename", parser);
    addToHash(savePluginHash, "autonumber", parser);
    addToHash(savePluginHash, "confirmsave", parser);
    addToHash(savePluginHash, "savescandescription", parser);
    addToHash(savePluginHash, "comment", parser);

    if (savePluginHash.contains("savefilename")) useStorage = true;

    sendStatusTimer = new QTimer(this);
    sendStatusTimer->setInterval(5000);
    sendStatusTimer->setSingleShot(false);
    if (rootSM) totalPositions = rootSM->getTotalSteps();
    sendError(EVEMESSAGESEVERITY_SYSTEM, EVEERRORMESSAGETYPE_TOTALCOUNT, QString("%1").arg(totalPositions));
    connect(sendStatusTimer, SIGNAL(timeout()), this, SLOT(sendRemainingTime()));
    sendStatusTimer->start(3000);

}

eveScanManager::~eveScanManager() {
    //delete sendStatusTimer;

    // TODO
    // unregister events ? (or is it done elsewhere)
}

/**
 * \brief  initialization, called if thread starts
 *
 * connect signals from manager thread
 */
void eveScanManager::init() {

    if (!rootSM) {
        sendError(ERROR,0,"init: no root scanmodule found");
        shutdown();
        return;
    }
    // init StorageModule if we have a Datafilename
    if (useStorage) {
        sendMessage(new eveStorageMessage(chainId, channelId, savePluginHash, 0, storageChannel));
    }

    // init PosCountTimer
    eveDevInfoMessage* message = new eveDevInfoMessage("PosCountTimer", "", new QStringList("unit:msecs"), DMTmetaData, "");
    message->setChainId(chainId);
    sendMessage(message);
    sendError(DEBUG,0,"sent PosCountTimer message");

    rootSM->initialize();
    // init events
    bool haveRedoEvent = false;
    if (eventList != NULL){
        foreach (eveEventProperty* evprop, *eventList){
            sendError(DEBUG, 0, QString("registering chain event"));
            registerEvent(0, evprop, true);
            if (evprop->getActionType() == eveEventProperty::REDO) haveRedoEvent = true;
        }
        delete eventList;
    }
    if (haveRedoEvent) rootSM->activateRedo();
    eventList = NULL;
}

/**
 * \brief shutdown associated chain and this thread
 */
void eveScanManager::shutdown(){

    eveError::log(DEBUG, QString("eveScanManager: shutdown"));

    if (!shutdownPending) {
        shutdownPending = true;
        disconnect (manager, SIGNAL(startSMs()), this, SLOT(smStart()));
        disconnect (manager, SIGNAL(stopSMs()), this, SLOT(smStop()));
        disconnect (manager, SIGNAL(breakSMs()), this, SLOT(smBreak()));
        disconnect (manager, SIGNAL(haltSMs()), this, SLOT(smHalt()));
        disconnect (manager, SIGNAL(pauseSMs()), this, SLOT(smPause()));

        // unregister events, do not delete them
        foreach(eveEventProperty* eprop, eventPropList){
            addMessage(new eveEventRegisterMessage(false, eprop));
        }
        eventPropList.clear();

        // stop input Queue
        disableInput();

        // delete all SMs
        if (rootSM) delete rootSM;
        rootSM = NULL;

        connect(this, SIGNAL(messageTaken()), this, SLOT(shutdown()) ,Qt::QueuedConnection);
    }

    // make sure mHub reads all outstanding messages before closing the channel
    if (shutdownThreadIfQueueIsEmpty()) eveError::log(DEBUG, QString("eveScanManager: shutdown done"));
}


/**
 * \brief  start SM (or continue if previously paused)
 *
 * usually called by a signal from manager thread
 * only the first Start event starts the chain, all others resume from pause
 */
void eveScanManager::smStart() {
    sendError(DEBUG,0,"eveScanManager::smStart: starting root");
    if (!isStarted) {
        sendStartTime();
        isStarted = true;
        eveEventProperty evprop(eveEventProperty::START, 0);
        if (rootSM) rootSM->newEvent(&evprop);
    }
    else {
        eveEventProperty evprop(eveEventProperty::PAUSE, 0);
        evprop.setOn(false);
        evprop.setDirection(eveDirectionONOFF);
        if (rootSM) rootSM->newEvent(&evprop);
    }
}

/**
 * \brief  Slot for halt signal; stop all motors, halt chain
 *
 * Emergency: stop all running motors, set all executing SM to last stage,
 * terminate chain, save data do not do postscan actions.
 *
 *
 */
void eveScanManager::smHalt() {
    eveEventProperty evprop(eveEventProperty::HALT, 0);
    if (rootSM) rootSM->newEvent(&evprop);
}

/**
 * \brief  stop current SM, do postscan actions, resume with the next SM.
 *
 * break after the current step is ready, resume with current postscan
 * do nothing, if we are halted or stopped
 */
void eveScanManager::smBreak() {
    eveEventProperty evprop(eveEventProperty::BREAK, 0);
    if (rootSM) rootSM->newEvent(&evprop);
}

/**
 * \brief  stop current SM, do postscan actions, terminate chain, save data
 *
 * stop after the current step is ready, resume with current postscan
 * terminate the chain.
 * do nothing, if we are already halted, clear a break
 */
void eveScanManager::smStop() {
    eveEventProperty evprop(eveEventProperty::STOP, 0);
    if (rootSM) rootSM->newEvent(&evprop);
}

/**
 * \brief Pause current SMs (continue with start command).
 *
 * pause after the current stage is ready, do not stop moving motors
 * do only if we are currently executing
 */
void eveScanManager::smPause() {
    eveEventProperty evprop(eveEventProperty::PAUSE, 0);
    if (rootSM) rootSM->newEvent(&evprop);
}


/**
 * \brief
 *
 */
void eveScanManager::smDone() {

    if (currentStatus.isDone()){
        shutdown();
    }
}

void eveScanManager::sendMessage(eveMessage *message){
    // a message with destination zero is sent to all storageModules
    if (message->getType() == EVEMESSAGETYPE_DEVINFO) {
        message->setDestinationChannel(storageChannel);
        message->setDestinationFacility(EVECHANNEL_STORAGE);
    }
    else if (message->getType() == EVEMESSAGETYPE_DATA) {
        message->setDestinationChannel(storageChannel);
        message->setDestinationFacility(EVECHANNEL_STORAGE);
        if (((eveDataMessage*)message)->getDataMod() != DMTaverageParams)
            message->addDestinationFacility(EVECHANNEL_MATH);
        if (((eveDataMessage*)message)->getPositionCount() == 0)
            ((eveDataMessage*)message)->setPositionCount(posCounter);
    }
    addMessage(message);
}

void eveScanManager::sendRemainingTime(){

    if (isStarted){
        if ((totalPositions > 2) && (posCounter > 1)){
            if (totalPositions < posCounter) totalPositions = posCounter;
            int elapsed = startTime.secsTo(QDateTime::currentDateTime());
            int remaining = elapsed * totalPositions / posCounter - elapsed;
            sendError(DEBUG, 0, QString("remaining %4, elapsed %1, totalPositions %2, posCounter %3").arg(elapsed).arg(totalPositions).arg(posCounter).arg(remaining));
            eveChainProgressMessage* message = new eveChainProgressMessage(chainId, posCounter, eveTime::getCurrent(), remaining);
            addMessage(message);
        }
    }
}

/**
 * \brief process messages sent to eveScanManager
 * @param message pointer to the message which eveScanManager received
 */
void eveScanManager::handleMessage(eveMessage *message){

    eveError::log(DEBUG, "eveScanManager: message arrived");
    switch (message->getType()) {
    case EVEMESSAGETYPE_REQUESTANSWER:
    {
        int rid = ((eveRequestAnswerMessage*)message)->getReqId();
        if (requestHash.contains(rid)) {
            eveEventProperty evprop(eveEventProperty::TRIGGER, rid);
            if (rootSM) rootSM->newEvent(&evprop);
            //				if (rootSM) rootSM->triggerSM(requestHash.value(rid), rid);
            requestHash.remove(rid);
        }
    }
        break;
    default:
        sendError(ERROR,0,"eveScanManager::handleMessage: unknown message");
        break;
    }
    try {
        delete message;
    }
    catch (std::exception& e)
    {
        sendError(FATAL, 0, QString("C++ Exception while deleting message %1").arg(e.what()));
    }
}

/**
 *
 * @param severity error severity (info, error, fatal, etc.)
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveScanManager::sendError(int severity, int errorType,  QString errorString){
    sendError(severity, EVEMESSAGEFACILITY_SCANCHAIN, errorType, errorString);
}
/**
 * \brief add an error message
 * @param severity error severity (info, error, fatal, etc.)
 * @param facility who sends this errormessage
 * @param errorType predefined error type or 0
 * @param errorString String describing the error
 */
void eveScanManager::sendError(int severity, int facility, int errorType,  QString errorString){
    addMessage(new eveErrorMessage(severity, facility, errorType, errorString));
}

void eveScanManager::sendStartTime() {
    startTime = QDateTime::currentDateTime();
    eveMessageTextList* mtl = new eveMessageTextList(EVEMESSAGETYPE_METADATA, (QStringList() << "StartTime" << startTime.time().toString("hh:mm:ss")));
    mtl->setChainId(chainId);
    addMessage(mtl);
    mtl = new eveMessageTextList(EVEMESSAGETYPE_METADATA, (QStringList() << "StartDate" << startTime.date().toString("dd.MM.yyyy")));
    mtl->setChainId(chainId);
    addMessage(mtl);
}

/**
 *
 * @param smid id of scanmodule
 * @param status current chain status
 */
void eveScanManager::setStatus(int smid, eveSMStatus& smstatus){
    int rootsmid = 0;
    if (rootSM) rootsmid = rootSM->getSmId();
    if (currentStatus.setStatus(smid, rootsmid, smstatus)) sendStatus ();
    if (currentStatus.isDone())
        sendError(DEBUG,0,"eveScanManager::setStatus: all SMs done, ready to shutdown");

}
/**
 *
 * @param smid id of scanmodule
 * @param status current chain status
 * @param remainTime remaining time until scan is done (defaults to -1)
 */
void eveScanManager::sendStatus(){

    // fill in our storage channel
    eveChainStatusMessage* ecmessage = new eveChainStatusMessage(chainId, currentStatus);
    ecmessage->addDestinationFacility(EVECHANNEL_MATH);
    addMessage(ecmessage);
    if (currentStatus.getCStatus() == CHStatusDONE) sendRemainingTime();
}

void eveScanManager::addToHash(QHash<QString, QString>& hash, QString key, eveXMLReader* parser){

    QString value;
    value = parser->getChainString(chainId, key);
    if (!value.isEmpty()) hash.insert(key, value);

}

/**
 * increment position counter and send next-position-message
 */
void eveScanManager::nextPos(){
    ++posCounter;
    eveDataMessage* message = new eveDataMessage("PosCountTimer", "", eveDataStatus(), DMTmetaData, eveTime::getCurrent(), QVector<int>(1, eveStartTime::getMSecsSinceStart()));
    message->setDestinationChannel(storageChannel);
    message->setDestinationFacility(EVECHANNEL_STORAGE);
    message->setPositionCount(posCounter);
    message->setChainId(chainId);
    addMessage(message);
}

/**
 * \brief connect this ScanManagers newEvent-Slot with the EventProperty object and send
 * the EventProperty to EventManager.
 *
 * @param smid
 * @param evproperty the eveEventProperty Object to be registered
 * @param chain true if it is a chain event, else false
 */
void eveScanManager::registerEvent(int smid, eveEventProperty* evproperty, bool chain) {

    if (evproperty == NULL){
        sendError(ERROR, 0, "Not registering event; invalid event property");
        return;
    }
    if (nextEventId > 999999) {
        delete evproperty;
        sendError(ERROR, 0, "Not registering event; eventId overflow");
        return;
    }
    sendError(DEBUG, 0, "registering event");
    if (chain){
        ++nextEventId;
        evproperty->setEventId(nextEventId);
    }
    evproperty->setSmId(smid);
    evproperty->setChainAction(chain);
    connect(evproperty, SIGNAL(signalEvent(eveEventProperty*)), this, SLOT(newEvent(eveEventProperty*)), Qt::QueuedConnection);
    eventPropList.append(evproperty);
    eveEventRegisterMessage* regmessage = new eveEventRegisterMessage(true, evproperty);
    addMessage(regmessage);
}

void eveScanManager::newEvent(eveEventProperty* evprop) {

    if (shutdownPending) return;

    if (evprop == NULL){
        sendError(ERROR,0,"eveScanManager: invalid event property");
        return;
    }

    if (evprop->getEventType() == eveEventTypeSCHEDULE )
        sendError(DEBUG, 0, QString("eveScanManager: received new schedule event, action type %1").arg(evprop->getActionType()));
    else
        sendError(DEBUG, 0, QString("eveScanManager: received new monitor event"));

    if (evprop->isChainAction()){
        if ((evprop->getActionType() == eveEventProperty::START) && (!isStarted)){
            sendStartTime();
            isStarted = true;
        }
    }
    if (rootSM) rootSM->newEvent(evprop);
}

int eveScanManager::sendRequest(int smid, QString text){
    int newrid = eveRequestManager::getRequestManager()->newId(channelId);
    requestHash.insert(newrid, smid);
    addMessage(new eveRequestMessage(newrid,EVEREQUESTTYPE_TRIGGER, text));
    return newrid;
}

void eveScanManager::cancelRequest(int rid){
    if (requestHash.contains(rid)) {
        requestHash.remove(rid);
        addMessage(new eveRequestCancelMessage(rid));
    }
}
