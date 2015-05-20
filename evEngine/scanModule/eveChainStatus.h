/*
 * eveChainStatus.h
 *
 *  Created on: 30.03.2012
 *      Author: eden
 */

#ifndef EVECHAINSTATUS_H_
#define EVECHAINSTATUS_H_

#include "qhash.h"
#include "eveSMStatus.h"
#include "eveMessage.h"


class eveChainStatus {
public:
    eveChainStatus();
    eveChainStatus(CHStatusT cstat, QHash<int, quint32>& smstat);
    virtual ~eveChainStatus(){};
    CHStatusT getCStatus(){return ecstatus;};
    void setDone(){ecstatus=CHStatusDONE;};
    bool isDone(){return (ecstatus==CHStatusDONE);};
    bool setStatus(int smid, int rootsmid, eveSMStatus& smstatus);
    QHash<int, quint32>& getSMStatusHash(){return smhash;};
    int getLastSmId(){return lastSMsmid;};
    SMStatusT getLastSmStatus(){return lastSMStatus;};
    bool isDoneSM(){return lastSMisDone;};
    bool isSmStarting(){return lastSmStarted;};

private:
    CHStatusT ecstatus;
    QHash<int, quint32> smhash;
    SMStatusT toStatus(quint32 fullstatus){return (SMStatusT) (fullstatus >> 16);} ;

    int lastSMsmid;
    SMStatusT lastSMStatus;
    bool lastSMisDone;
    bool lastSmStarted;

};

#endif /* EVECHAINSTATUS_H_ */
