/*
 * eveChainStatus.h
 *
 *  Created on: 30.03.2012
 *      Author: eden
 */

#ifndef EVECHAINSTATUS_H_
#define EVECHAINSTATUS_H_

#include "eveMessage.h"
#include "eveEventProperty.h"

// enum chainStatusT {eveChainSmIDLE=1, eveChainSmINITIALIZING, eveChainSmEXECUTING, eveChainSmPAUSED, eveChainSmTRIGGERWAIT, eveChainSmDONE, eveChainDONE, eveChainSTORAGEDONE};
// enum smStatusT {eveSmNOTSTARTED, eveSmINITIALIZING, eveSmEXECUTING, eveSmPAUSED, eveSmTRIGGERWAIT, eveSmAPPEND, eveSmDONE} ;

// allow following states for cstatus: eveSmINITIALIZING,  eveSmEXECUTING, eveSmPAUSED, eveSmDONE
// allow following additional substates: paused, redo
// which may be active at the same time
//

class eveChainStatus {
public:
	eveChainStatus();
	virtual ~eveChainStatus();
	chainStatusT getStatus(){return cstatus;};
	bool setStatus(smStatusT);
	void setChainStatus(chainStatusT status){cstatus=status;};
	bool setEvent(eveEventProperty*);
	bool isPause(){return pause;};
	bool isRedo(){return redo;};
//	void setPause(bool onOff){pause = onOff;};
//	void setRedo(bool onOff){redo = onOff;};

private:
	chainStatusT cstatus;
	bool pause;
	bool redo;
};

#endif /* EVECHAINSTATUS_H_ */
