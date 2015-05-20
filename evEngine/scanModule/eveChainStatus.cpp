/*
 * eveChainStatus.cpp
 *
 *  Created on: 30.03.2012
 *      Author: eden
 */

#include "eveChainStatus.h"

eveChainStatus::eveChainStatus() {

    ecstatus = CHStatusIDLE;
    lastSMsmid = 0;
    lastSMStatus = SMStatusNOTSTARTED;
}

eveChainStatus::eveChainStatus(CHStatusT cstat, QHash<int, quint32>& smstat) {

    ecstatus = cstat;
    lastSMsmid = 0;
    lastSMStatus = SMStatusNOTSTARTED;
    smhash = smstat;
    lastSMisDone = false;
    lastSmStarted = false;
}

/**
 * @brief set the chain status: Idle, Executing, Done
 * Idle: after start, never again
 * Executing: after the first scanmodule has been started
 * Done: all scanmodules are done
 *
 * @param smstatus status of a scanmodule
 * @return
 */
bool eveChainStatus::setStatus(int smid, int rootsmid, eveSMStatus& smstatus ) {

    bool changed = false;
    CHStatusT ecstatusold = ecstatus;

    lastSMisDone = false;
    lastSmStarted = false;

    if ((ecstatus == CHStatusIDLE) && smstatus.isExecuting()) ecstatus = CHStatusExecuting;

    if (smhash.contains(smid)) {
        if (smhash.value(smid) != smstatus.getFullStatus()){
            changed = true;
            SMStatusT oldStatus = toStatus(smhash.value(smid));
            smhash.insert(smid, smstatus.getFullStatus());
            lastSMsmid = smid;
            lastSMStatus = smstatus.getStatus();
            if (((lastSMStatus == SMStatusAPPEND) || (lastSMStatus == SMStatusDONE))
                    && (oldStatus != SMStatusAPPEND)) lastSMisDone = true;
            if ((lastSMStatus == SMStatusEXECUTING) && ((oldStatus == SMStatusINITIALIZING) ||
                                                        (oldStatus == SMStatusNOTSTARTED) ||
                                                        (oldStatus == SMStatusDONE))) lastSmStarted = true;
        }
    }
    else {
        changed = true;
        smhash.insert(smid, smstatus.getFullStatus());
    }
    if ((smid == rootsmid) && smstatus.isDone() && !smstatus.isAppend()) {
        ecstatus = CHStatusDONE;
    }
    if (ecstatusold != ecstatus) changed = true;
    return changed;
}
