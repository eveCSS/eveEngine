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

// enum chainStatusT see eveMessage.h
// enum smStatusT see eveMessage.h


class eveSMStatus {
public:
	eveSMStatus();
	virtual ~eveSMStatus();
	bool setStatus(smStatusT);
	bool isExecuting();
	bool isDone();
    bool isRedo(){return (redoActive && (chainRedo || redo));};
    bool isPaused(){return (chainPause || pause || masterPause);};
	smStatusT getStatus(){return status;};
    int getPause();
	bool setEvent(eveEventProperty* evprop );
	void redoStart(){trackRedo = false;};
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
	smStatusT status;
	bool isTriggerWait(){return (evTrigWait || maTrigWait || detTrigWait);};
	int  manualRid;
	int  detecRid;
	bool pause;
	bool redo;
	bool chainPause;
    bool masterPause;
	bool chainRedo;
	bool trackRedo; // wird mit redoStart gelöscht, von redoEvent
	                // gesetzt und redoStatus gelesen
	bool evTrigWait;
	bool maTrigWait;
	bool detTrigWait;
    bool stopCondition;
    bool breakCondition;
    bool redoActive;
};

#endif /* EVESMSTATUS_H_ */
