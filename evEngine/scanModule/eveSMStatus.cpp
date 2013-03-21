/*
 * eveSMStatus.cpp
 *
 *  Created on: 02.04.2012
 *      Author: eden
 */

#include "eveSMStatus.h"

eveSMStatus::eveSMStatus() {

	status = eveSmNOTSTARTED;
	pause = false;
	redo = false;
	chainPause = false;
    masterPause = false;
    chainRedo = false;
	trackRedo = false;
	evTrigWait = false;
	maTrigWait = false;
	detTrigWait = false;
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
bool eveSMStatus::setStatus(smStatusT newStatus ) {

	bool retval = false;

	if ((newStatus == eveSmEXECUTING) && isPaused()){
        retval = true;
        status = eveSmPAUSED;
	}
	else if (status != newStatus) {
		retval=true;
		// we are done, do not send status if previous status was smAppend
		if ((status == eveSmAPPEND ) && (newStatus == eveSmDONE )) retval=false;
		status = newStatus;
	}

	return retval;
}

int eveSMStatus::getPause() {

    if (masterPause)
        return 3;
    else if (chainPause)
        return 2;
    else if (pause)
        return 1;
    return 0;
}


/**
 *
 * @return true if status was executing i.e. we are started and may be paused
 */
bool eveSMStatus::isExecuting() {
    if ((status == eveSmEXECUTING) || (status == eveSmTRIGGERWAIT) || (status == eveSmPAUSED))
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
	if (!(status == eveSmEXECUTING)) {
		retval = true;
		status = eveSmEXECUTING;
	}
	pause = false;
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

	if ((status == eveSmDONE) || (status == eveSmAPPEND)) return true;
	return false;
}

bool eveSMStatus::triggerManualStart(int rid){

	manualRid=rid;
	maTrigWait=true;
	if (status != eveSmTRIGGERWAIT){
		status = eveSmTRIGGERWAIT;
		return true;
	}
	return false;
}

bool eveSMStatus::triggerEventStart(){

	evTrigWait=true;
    if (status != eveSmTRIGGERWAIT){
		status = eveSmTRIGGERWAIT;
		return true;
	}
	return false;
}

bool eveSMStatus::triggerDetecStart(int rid){

	detecRid=rid;
	detTrigWait=true;
	if (status != eveSmTRIGGERWAIT){
		status = eveSmTRIGGERWAIT;
		return true;
	}
	return false;
}

bool eveSMStatus::setEvent(eveEventProperty* evprop ) {

    bool changed = false;
    bool *setPause, *leavePause, *setRedo;

    if (evprop->isChainAction()){
        setPause = &chainPause;
        leavePause = &pause;
        setRedo = &chainRedo;
    }
    else {
        leavePause = &chainPause;
        setPause = &pause;
        setRedo = &redo;
    }

    if (evprop->getEventType() == eveEventTypeGUI){
        if (evprop->getActionType() == eveEventProperty::PAUSE){
            if (evprop->isSwitchOn()){
                if (isExecuting() && !isPaused()) {
                    changed = true;
                    status = eveSmPAUSED;
                }
                masterPause = true;
            }
            else if (evprop->isSwitchOff()) {
                if (isExecuting() && isPaused()) {
                    changed = true;
                    if (isTriggerWait())
                        status = eveSmTRIGGERWAIT;
                    else
                        status = eveSmEXECUTING;
                }
                pause = false;
                chainPause = false;
                masterPause = false;
            }

        }
        else if (evprop->getActionType() == eveEventProperty::TRIGGER){
            // accept a trigger even if paused
            if (isTriggerWait()) {
                if (manualRid == evprop->getEventId())
                    maTrigWait = false;
                else if (detecRid == evprop->getEventId())
                    detTrigWait = false;
                if ( isExecuting() && !isPaused() && !isTriggerWait()) {
                    status = eveSmEXECUTING;
                    changed = true;
                }
            }
        }
    }
    else {
        switch (evprop->getActionType()){
        case eveEventProperty::REDO:
            if (evprop->getOn()){
                if (!trackRedo) changed = true;
                trackRedo = true;
                *setRedo = true;
            }
            else {
                if (*setRedo) changed = true;
                *setRedo = false;
            }
            break;
        case eveEventProperty::PAUSE:
            if (evprop->isSwitchOn()){
                if (isExecuting() && !isPaused()) {
                    changed = true;
                    status = eveSmPAUSED;
                }
                *setPause = true;
            }
            else if (evprop->isSwitchOff()) {
                if (isExecuting() && *setPause && !*leavePause && !masterPause) {
                    changed = true;
                    if (isTriggerWait())
                        status = eveSmTRIGGERWAIT;
                    else
                        status = eveSmEXECUTING;
                }
                *setPause = false;
            }
            break;
        case eveEventProperty::START:
            // resume from pause
            if (!*leavePause && !masterPause && (status == eveSmPAUSED)){
                changed = true;
                if (isTriggerWait())
                    status = eveSmTRIGGERWAIT;
                else
                    status = eveSmEXECUTING;
            }
            *setPause = false;
            break;
        case eveEventProperty::BREAK:
            if (isExecuting())	status = eveSmEXECUTING;
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

                if ( isExecuting() && !isPaused() && !isTriggerWait()) {
                    status = eveSmEXECUTING;
                    changed = true;
                }
            }
            break;
        default:
            break;
        }
    }

    return changed;
}

