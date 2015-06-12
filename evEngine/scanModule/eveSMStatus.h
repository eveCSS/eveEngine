/*
 * eveSMStatus.h
 *
 *  Created on: 02.04.2012
 *      Author: eden
 */

#ifndef EVESMSTATUS_H_
#define EVESMSTATUS_H_

#include "eveMessage.h"
#include "eveEventProperty.h"


class eveSMStatus {
public:
	eveSMStatus();
	virtual ~eveSMStatus();
	bool isExecuting();
	bool isDone();
    bool setStart();
    bool setDone();
    bool setAppend();
    void reset();
    bool isAppend(){return (status == SMStatusAPPEND); };
    bool isNotStarted(){return (!isStarted); };
    bool isBeforeExecuting(){return ((status == SMStatusNOTSTARTED)||(status == SMStatusINITIALIZING)); };
    bool isRedo(){return (redoActive && (chainRedo || smRedo));};
    bool isPaused(){return (chainPause || smPause || masterPause);};
    SMStatusT getStatus(){return status;};
    SMReasonT getReason(){return reason;};
    quint32 getFullStatus();
    int getPause();
	bool setEvent(eveEventProperty* evprop );
    void redoStart(){trackRedo = false;};
    bool checkStatus(){return setStatus(status, reason);};
    bool redoStatus(){return redoActive && trackRedo;};
	bool triggerManualStart(int);
	bool isManualTriggerWait(){return maTrigWait;};
	bool triggerEventStart();
	bool isTriggerEventWait(){return evTrigWait;};
	bool triggerDetecStart(int);
	bool isTriggerDetecWait(){return detTrigWait;};
    bool forceExecuting();
    bool haveStopCondition(){return stopCondition;};
    bool haveBreakCondition(){return breakCondition;};
    void activateRedo(){redoActive = true;};
    bool redoIsActive() {return redoActive;};

private:
    bool setStatus(SMStatusT, SMReasonT);
    SMStatusT status;
    SMReasonT reason;
    bool isTriggerWait(){return (evTrigWait || maTrigWait || detTrigWait);};
	int  manualRid;
	int  detecRid;
    bool smPause;
	bool chainPause;
    bool masterPause;     // pause set by GUI
    bool smRedo;          // smRedoEvent sets / unsets this
    bool chainRedo;       // chainRedoEvent sets / unsets this
    bool trackRedo;       // wird mit redoStart gelöscht, von redoEvent
                          // gesetzt und redoStatus gelesen
	bool evTrigWait;
	bool maTrigWait;
	bool detTrigWait;
    bool stopCondition;
    bool breakCondition;
    bool redoActive;
    bool freezeReason;
    bool isStarted;
};

#endif /* EVESMSTATUS_H_ */
