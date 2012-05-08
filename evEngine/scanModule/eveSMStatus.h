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

// enum chainStatusT {eveChainSmIDLE=1, eveChainSmINITIALIZING, eveChainSmEXECUTING, eveChainSmPAUSED, eveChainSmTRIGGERWAIT, eveChainSmDONE, eveChainDONE, eveChainSTORAGEDONE};
// enum smStatusT {eveSmNOTSTARTED, eveSmINITIALIZING, eveSmEXECUTING, eveSmPAUSED, eveSmTRIGGERWAIT, eveSmAPPEND, eveSmDONE} ;


class eveSMStatus {
public:
	eveSMStatus();
	virtual ~eveSMStatus();
	bool setStatus(smStatusT);
	bool isExecuting();
	bool isDone();
	bool isRedo(){return (chainRedo || redo);};
	bool isPaused(){return (chainPause || pause);};
	smStatusT getStatus(){return status;};
	bool setEvent(eveEventProperty* evprop );
	void redoStart(){trackRedo = false;};
	bool redoStatus(){return trackRedo;};
	bool triggerManualStart(int);
	bool isManualTriggerWait(){return maTrigWait;};
	bool triggerEventStart();
	bool isTriggerEventWait(){return evTrigWait;};
	bool triggerDetecStart(int);
	bool isTriggerDetecWait(){return detTrigWait;};
	bool forceExecuting();

private:
	smStatusT status;
	bool isTriggerWait(){return (evTrigWait || maTrigWait || detTrigWait);};
	int  manualRid;
	int  detecRid;
	bool pause;
	bool redo;
	bool chainPause;
	bool chainRedo;
	bool trackRedo; // wird mit redoStart gelöscht, von redoEvent
	                // gesetzt und redoStatus gelesen
	bool evTrigWait;
	bool maTrigWait;
	bool detTrigWait;
};

#endif /* EVESMSTATUS_H_ */
