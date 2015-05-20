/*
 * eveSMStatus.cpp
 *
 *  Created on: 02.04.2012
 *      Author: eden
 */

#include "eveSMStatus.h"

// enum SMStatusT {SMStatusNOTSTARTED, SMStatusINITIALIZING, SMStatusEXECUTING, SMStatusPAUSE, SMStatusTRIGGERWAIT, SMStatusAPPEND, SMStatusDONE} ;
//enum SMReasonT {SMReasonNone, SMReasonREDOACTIVE, SMReasonEVENTPAUSE, SMReasonEVENTSKIPPED, SMReasonAPPEND, SMReasonDONE} ;


eveSMStatus::eveSMStatus() {

    status = SMStatusNOTSTARTED;
    reason = SMReasonNone;
    smPause = false;
    smRedo = false;
    chainPause = false;
    masterPause = false;
    chainRedo = false;
    trackRedo = false;
    evTrigWait = false;
    maTrigWait = false;
    detTrigWait = false;
    stopCondition = false;
    breakCondition = false;
}

eveSMStatus::~eveSMStatus() {
    // TODO Auto-generated destructor stub
}

/**
 *
 * @param newStatus status of a SM
 * @return true if status changed else false
 *
 * eveSmINITIALIZING is not used for status!
 */
//bool eveSMStatus::setStatus(SMStatusT newStatus ) {

//    bool retval = false;

//    if ((newStatus == SMStatusEXECUTING) && isPaused()){
//        retval = true;
//        status = SMStatusPAUSE;
//    }
//    else if (status != newStatus) {
//        retval=true;
//        // TODO move this check into extChainStatusMessage
//        // we are done, do not send status if previous status was smAppend
////        if ((status == SMStatusAPPEND ) && (newStatus == SMStatusDONE )) retval=false;
//        status = newStatus;
//    }
//    return retval;
//}
/**
 * @brief eveSMStatus::setStart start or restart of an SM
 * @return
 */
bool eveSMStatus::setStart() {

    breakCondition = false;
    stopCondition = false;
    freezeReason = false;
    reason = SMReasonNone;
    if (!isExecuting()){
        status = SMStatusEXECUTING;
    }
    return setStatus(status, reason);
}

bool eveSMStatus::setDone() {

    bool retval=true;

    //if ((status == SMStatusAPPEND ) || (status == SMStatusDONE ))  retval = false;
    if (status == SMStatusDONE )  retval = false;
    status = SMStatusDONE;
    return retval;
}

bool eveSMStatus::setAppend() {

    bool retval=true;

    if (status == SMStatusAPPEND)  retval = false;
    status = SMStatusAPPEND;
    return retval;
}

//int eveSMStatus::getPause() {

//    if (masterPause)
//        return 3;
//    else if (chainPause)
//        return 2;
//    else if (smPause)
//        return 1;
//    return 0;
//}


/**
 *
 * @return true if status was executing i.e. we are started and may be paused
 */
bool eveSMStatus::isExecuting() {
    if ((status == SMStatusEXECUTING) || (status == SMStatusTRIGGERWAIT) || (status == SMStatusPAUSE))
        return true;
    else
        return false;
}

/**
 *
 * @return true if status was "not executing"
 */
bool eveSMStatus::forceExecuting() {
    bool retval = false;
    if (!(status == SMStatusEXECUTING)) {
        retval = true;
        status = SMStatusEXECUTING;
    }
    smPause = false;
    masterPause = false;
    chainPause = false;
    evTrigWait = false;
    maTrigWait = false;
    detTrigWait = false;
    return retval;
}

/**
 *
 * @return true if we are done (an appended scan may still be running)
 */
bool eveSMStatus::isDone() {

    if ((status == SMStatusDONE) || (status == SMStatusAPPEND)) return true;
    return false;
}

bool eveSMStatus::triggerManualStart(int rid){

    manualRid=rid;
    maTrigWait=true;
    if (status != SMStatusTRIGGERWAIT){
        status = SMStatusTRIGGERWAIT;
        return true;
    }
    return false;
}

bool eveSMStatus::triggerEventStart(){

    evTrigWait=true;
    if (status != SMStatusTRIGGERWAIT){
        status = SMStatusTRIGGERWAIT;
        return true;
    }
    return false;
}

bool eveSMStatus::triggerDetecStart(int rid){

    detecRid=rid;
    detTrigWait=true;
    if (status != SMStatusTRIGGERWAIT){
        status = SMStatusTRIGGERWAIT;
        return true;
    }
    return false;
}

quint32 eveSMStatus::getFullStatus(){

    quint32 retstatus;
    if (status == SMStatusAPPEND)
        retstatus = (quint32) SMStatusDONE;
    else
        retstatus = (quint32) status;

    return (retstatus << 16) + (quint32)reason;
}

bool eveSMStatus::setStatus(SMStatusT oldstatus, SMReasonT oldreason) {

    if (isExecuting()){
        if ((reason == SMReasonGUISKIP) || (reason == SMReasonGUISKIP) || (reason == SMReasonGUISKIP)) {
                freezeReason=true;
        }
        else if (isPaused()){
            status = SMStatusPAUSE;
            if (!freezeReason){
                if (masterPause) reason = SMReasonGUIPAUSE;
                else if (chainPause) reason = SMReasonCHPAUSE;
                else reason = SMReasonSMPAUSE;
            }
        }
        else if (isTriggerWait()) {
            status = SMStatusTRIGGERWAIT;
            if (!freezeReason) reason = SMReasonNone;
        }
        else if (redoActive && trackRedo && chainRedo) {
            status = SMStatusEXECUTING;
            if (!freezeReason) reason = SMReasonCHREDOACTIVE;
        }
        else if (redoActive && trackRedo && chainRedo) {
            status = SMStatusEXECUTING;
            if (!freezeReason) reason = SMReasonSMREDOACTIVE;
        }
        else {
            status = SMStatusEXECUTING;
            if (!freezeReason) reason = SMReasonNone;
        }
    }
    if ((oldstatus == status) && (oldreason == reason))
        return false;
    else
        return true;
}

//enum SMReasonT {SMReasonNone, SMReasonSMREDOACTIVE, SMReasonCHREDOACTIVE, SMReasonSMPAUSE,
//      SMReasonCHTPAUSE, SMReasonGUIPAUSE, SMReasonSMSKIP, SMReasonCHSKIP, SMReasonGUISKIP,
//      SMReasonAPPEND, SMReasonDONE} ;

/**
 * @brief eveSMStatus::setEvent
 * @param evprop event property
 * @return true if status/reason changed
 */
bool eveSMStatus::setEvent(eveEventProperty* evprop ) {

    bool *setPause, *setRedo;
    SMStatusT oldstatus=status;
    SMReasonT oldreason=reason;

    if (evprop->isChainAction()){
        setPause = &chainPause;
        setRedo = &chainRedo;
    }
    else {
        setPause = &smPause;
        setRedo = &smRedo;
    }

    // only the first time the start button is a start event, then a pause event

    if (evprop->getEventType() == eveEventTypeGUI){
        if (evprop->getActionType() == eveEventProperty::PAUSE){
            if (evprop->isSwitchOn()){
                masterPause = true;
            }
            else if (evprop->isSwitchOff()) {
                smPause = false;
                chainPause = false;
                masterPause = false;
            }
        }
        else if (evprop->getActionType() == eveEventProperty::TRIGGER){
            // accept a trigger even if paused
            if (isTriggerWait()) {
                if (manualRid == evprop->getEventId())
                    maTrigWait = false;
            }
        }
        else if  (evprop->getActionType() == eveEventProperty::BREAK){
            reason = SMReasonGUISKIP;
            breakCondition = evprop->getOn();
        }
        else if  (evprop->getActionType() == eveEventProperty::STOP){
            reason = SMReasonGUISTOP;
            stopCondition = evprop->getOn();
        }
    }
    else {
        switch (evprop->getActionType()){
        case eveEventProperty::REDO:
            if (evprop->getOn()){
                trackRedo = true;
                *setRedo = true;
            }
            else {
                *setRedo = false;
            }
            break;
        case eveEventProperty::PAUSE:
            if (evprop->isSwitchOn()){
                *setPause = true;
            }
            else if (evprop->isSwitchOff()) {
                *setPause = false;
            }
            break;
        case eveEventProperty::BREAK:
            if (evprop->isChainAction()) {
                reason = SMReasonCHSKIP;
            }
            else {
                reason = SMReasonSMSKIP;
            }
            breakCondition = evprop->getOn();
            break;
        case eveEventProperty::TRIGGER:
            // accept a trigger even if paused
            if (isTriggerWait()) {
                if (evprop->getEventId() == 0)
                    evTrigWait = false;
                else if (manualRid == evprop->getEventId())
                    maTrigWait = false;
                else if (detecRid == evprop->getEventId())
                    detTrigWait = false;
            }
            break;
        case eveEventProperty::STOP:
        case eveEventProperty::HALT:
            // we have only chain stop events
            reason = SMReasonCHSTOP;
            stopCondition = evprop->getOn();
            break;
        default:
            break;
        }
    }

    return setStatus(oldstatus, oldreason);
}
