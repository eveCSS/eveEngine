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

// enum chainStatusT see eveMessage
// enum smStatusT {eveSmNOTSTARTED, eveSmINITIALIZING, eveSmEXECUTING, eveSmPAUSED, eveSmCHAINPAUSED, eveSmTRIGGERWAIT, eveSmAPPEND, eveSmDONE} ;

// allow following states for cstatus: eveSmINITIALIZING,  eveSmEXECUTING, eveSmPAUSED, eveSmDONE
// allow following additional substates: paused, redo
// which may be active at the same time
//

class eveChainStatus {
public:
	eveChainStatus();
	virtual ~eveChainStatus();
	chainStatusT getStatus(){return cstatus;};
    bool setStatus(smStatusT, int pause);
    void setChainStatus(chainStatusT status){cstatus=status;};

private:
	chainStatusT cstatus;
};

#endif /* EVECHAINSTATUS_H_ */
