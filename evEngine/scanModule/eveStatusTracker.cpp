/*
 * eveStatusTracker.cpp
 *
 *  Created on: 25.09.2008
 *      Author: eden
 */

#include "eveStatusTracker.h"

// from Message.h
//#define EVEENGINESTATUS_IDLENOXML 0x00 		// Idle, no XML-File loaded
//#define EVEENGINESTATUS_IDLEXML 0x01		// Idle, XML loaded
//#define EVEENGINESTATUS_LOADING 0x02		// loading XML
//#define EVEENGINESTATUS_EXECUTING 0x03		// executing
//


eveBasicStatusTracker::eveBasicStatusTracker() {
	loadedXML = false;
	engineStatus = eveEngIDLENOXML;


}

eveBasicStatusTracker::~eveBasicStatusTracker() {
	// TODO Auto-generated destructor stub
}

/**
 * \brief set chain status and signal an engineStatus change
 * \return true if engineStatus has changed, else false
 */
bool eveBasicStatusTracker::setChainStatus(eveChainStatusMessage* message) {

	int chainId = message->getChainId();
	chainStatusT newStatus = (chainStatusT)message->getStatus();
	bool AllDone = true;
	bool engStatusChng = false;

	if(chainStatus.value(chainId) == newStatus) return false;
	chainStatus.insert(chainId,newStatus);

	switch (newStatus) {
		case eveChainSmEXECUTING:
			if ((engineStatus == eveEngIDLEXML) || (engineStatus == eveEngLOADINGXML)){
				engineStatus = eveEngEXECUTING;
				engStatusChng=true;
			}
			break;
		case eveChainDONE:
		case eveChainSTORAGEDONE:
			foreach (int cid, chainStatus.keys()){
				//if (cid == chainId) continue;
				chainStatusT endStatus = eveChainDONE;
				if (storageList.contains(cid))
					endStatus = eveChainSTORAGEDONE;
				if (chainStatus.value(cid) != endStatus) AllDone = false;
			}
			if (AllDone){
				if (engineStatus != eveEngIDLENOXML){
					engineStatus = eveEngIDLENOXML;
					engStatusChng=true;
					loadedXML = false;
					XmlName.clear();
					chainStatus.clear();
					storageList.clear();
					emit engineIdle();
				}
			}
			break;
		default:
			break;
	}
}

eveEngineStatusMessage * eveBasicStatusTracker::getEngineStatusMessage() {
	// TODO change int engineStatus to engineStatusT
	return new eveEngineStatusMessage((int)engineStatus, XmlName);
}


eveStatusTracker::eveStatusTracker() {
	// TODO Auto-generated constructor stub
}

eveStatusTracker::~eveStatusTracker() {
	// TODO Auto-generated destructor stub
}

eveManagerStatusTracker::eveManagerStatusTracker() {
	autoStart = false;
}

eveManagerStatusTracker::~eveManagerStatusTracker() {
	// TODO Auto-generated destructor stub
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

	if ((engineStatus == eveEngEXECUTING) || (engineStatus == eveEngPAUSED)){
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

	engineStatus = eveEngHALTED;
	return true;
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

	int status = (int)engineStatus;
	if (autoStart) status |= EVEENGINESTATUS_AUTOSTART;
	return new eveEngineStatusMessage(status, XmlName);
}

