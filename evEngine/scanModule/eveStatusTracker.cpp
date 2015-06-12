/*
 * eveStatusTracker.cpp
 *
 *  Created on: 25.09.2008
 *      Author: eden
 */

#include "eveStatusTracker.h"
#include "eveError.h"

// from Message.h
//#define EVEENGINESTATUS_IDLENOXML 0x00 		// Idle, no XML-File loaded
//#define EVEENGINESTATUS_IDLEXML 0x01		// Idle, XML loaded
//#define EVEENGINESTATUS_LOADING 0x02		// loading XML
//#define EVEENGINESTATUS_EXECUTING 0x03		// executing
//


eveBasicStatusTracker::eveBasicStatusTracker() {
	loadedXML = false;
	engineStatus = eveEngIDLENOXML;
	repeatCount = 0;
    XmlName = "unbekannt";
}

eveBasicStatusTracker::~eveBasicStatusTracker() {
	// Auto-generated destructor stub
}

/**
 * \brief set chain status and signal an engineStatus change
 * \return true if engineStatus has changed, else false
 */
bool eveBasicStatusTracker::setChainStatus(eveChainStatusMessage* message) {

    int chainId = message->getChainId();
    CHStatusT newStatus = message->getChainStatus();
    bool engStatusChng = false;
    bool AllDone = true;

    if(chainStatus.value(chainId) == newStatus) return false;
    chainStatus.insert(chainId,newStatus);

    switch (newStatus) {
    case CHStatusIDLE:
        if (!haveActiveMath.contains(chainId)) haveActiveMath.append(chainId);
        if (!haveActiveScan.contains(chainId)) haveActiveScan.append(chainId);
        break;
    case CHStatusExecuting:
        if ((engineStatus == eveEngIDLEXML) || (engineStatus == eveEngLOADINGXML)){
            engineStatus = eveEngEXECUTING;
            engStatusChng=true;
        }
        break;
    case CHStatusDONE:
    case CHStatusSTORAGEDONE:
    case CHStatusMATHDONE:
        if (newStatus == CHStatusSTORAGEDONE) haveActiveStorage.removeAll(chainId);
        else if (newStatus == CHStatusMATHDONE) haveActiveMath.removeAll(chainId);
        else if (newStatus == CHStatusDONE) haveActiveScan.removeAll(chainId);
        if (!(haveActiveMath.contains(chainId) || haveActiveStorage.contains(chainId) || haveActiveScan.contains(chainId))){
            chainStatus.insert(chainId, CHStatusALLDONE);
            eveError::log(DEBUG, QString("eveStatusTracker::Chain (math, storage, scan) done"));
        }
        foreach (int cid, chainStatus.keys()){
            if (chainStatus.value(cid) != CHStatusALLDONE) AllDone = false;
            eveError::log(DEBUG, QString("eveStatusTracker:: chainStatus %1").arg((quint32)chainStatus.value(cid)));
        }
        eveError::log(DEBUG, QString("eveStatusTracker:: All done? %1").arg(AllDone?"true":"false"));
        if (AllDone){
            eveError::log(DEBUG, QString("eveStatusTracker::Engine done, Idle, NoXML"));
            if (engineStatus != eveEngIDLENOXML){
                engineStatus = eveEngIDLENOXML;
                engStatusChng=true;
                // signal end of work, used to kill engine in batch mode
                if (repeatCount == 0)emit engineIdle();
            }
        }
        break;
    default:
        break;
    }

    return engStatusChng;
}

CHStatusT eveBasicStatusTracker::getChainStatus(int chid) {
    if (chainStatus.contains(chid))
        return chainStatus.value(chid);
    else
        return CHStatusUNKNOWN;
}

void eveBasicStatusTracker::reset(){

    loadedXML = false;
    XmlName.clear();
    chainStatus.clear();
    haveActiveStorage.clear();
    haveActiveScan.clear();
    haveActiveMath.clear();

    return;
}


/**
 * \brief set the repeatCount to a number in range 0-65535
 */
void eveBasicStatusTracker::setRepeatCount(int count) {
    if (count < 0)
        repeatCount=0;
    else if (count > 0xffff)
        repeatCount = 0xffff;
    else
        repeatCount = count;
}

/**
 * \brief set the repeatCount to a number in range 0-65535
 * \return a message containing the current engine status
 */
eveEngineStatusMessage * eveBasicStatusTracker::getEngineStatusMessage() {
    unsigned int status = ((unsigned int)engineStatus) | (repeatCount << 16);
    return new eveEngineStatusMessage(status, XmlName);
}


eveManagerStatusTracker::eveManagerStatusTracker() {
    autoStart = false;
}

eveManagerStatusTracker::~eveManagerStatusTracker() {
    // Auto-generated destructor stub
}

/**
 * \brief tell tracker that start command has been received
 * \return true if command may be set, else false
 */
bool eveManagerStatusTracker::setStart() {

    if ((engineStatus == eveEngIDLEXML) || (engineStatus == eveEngPAUSED)){
        engineStatus = eveEngEXECUTING;
        return true;
    }
    return false;
}

/**
 * \brief tell tracker that stop command has been received
 * \return true if command may be set, else false
 */
bool eveManagerStatusTracker::setStop() {

    if ((engineStatus == eveEngIDLEXML) || (engineStatus == eveEngLOADINGXML) ||
            (engineStatus == eveEngEXECUTING) || (engineStatus == eveEngPAUSED)	||
            (engineStatus == eveEngSTOPPED)){
        engineStatus = eveEngSTOPPED;
        return true;
    }
    return false;
}

/**
 * \brief tell tracker that break command has been received
 * \return true if command may be set, else false
 */
bool eveManagerStatusTracker::setBreak() {

    if ((engineStatus == eveEngEXECUTING) || (engineStatus == eveEngPAUSED)){
        return true;
    }
    return false;
}

/**
 * \brief tell tracker that HALT command has been received
 * \return always true, since this is an emergency command
 */
bool eveManagerStatusTracker::setHalt() {

    if ((engineStatus == eveEngIDLEXML) || (engineStatus == eveEngLOADINGXML) ||
            (engineStatus == eveEngEXECUTING) || (engineStatus == eveEngPAUSED)	||
            (engineStatus == eveEngSTOPPED)){
        engineStatus = eveEngHALTED;
        return true;
    }
    return false;
}
/**
 * \brief tell tracker that stop command has been received
 * \return true if command may be set, else false
 */
bool eveManagerStatusTracker::setPause() {

    if (engineStatus == eveEngEXECUTING){
        engineStatus = eveEngPAUSED;
        return true;
    }
    return false;
}

/**
 * \brief tell tracker we will load XML now
 * \return true if command may be set, else false
 */
bool eveManagerStatusTracker::setLoadingXML(QString xmlname) {

    if (engineStatus == eveEngIDLENOXML){
        engineStatus = eveEngLOADINGXML;
        XmlName = xmlname;
        return true;
    }
    return false;
}
/**
 * \brief tell tracker that loading of XML was successful / unsuccessful
 * \param status true if loading of XML was successful, else false
 * \return true if command may be set, else false
 */
bool eveManagerStatusTracker::setXMLLoaded(bool status) {

    if (engineStatus == eveEngLOADINGXML){
        if (status) {
            engineStatus = eveEngIDLEXML;
            loadedXML = true;
        }
        else {
            engineStatus = eveEngIDLENOXML;
            XmlName.clear();
        }
        return true;
    }
    return false;
}
/**
 * \brief switch autostart on or off
 * \param autostart new value for autostart
 * \return true if autostart changed, else false
 */
bool eveManagerStatusTracker::setAutoStart(bool autostart) {

    if (autoStart == autostart) return false;
    autoStart = autostart;
    return true;
}

eveEngineStatusMessage * eveManagerStatusTracker::getEngineStatusMessage() {

    unsigned int status = ((unsigned int)engineStatus) | (repeatCount << 16);
    if (autoStart) status |= EVEENGINESTATUS_AUTOSTART;
    return new eveEngineStatusMessage(status, XmlName);
}
