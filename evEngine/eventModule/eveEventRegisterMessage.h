/*
 * eveEventRegisterMessage.h
 *
 *  Created on: 21.09.2009
 *      Author: eden
 */

#ifndef EVEEVENTREGISTERMESSAGE_H_
#define EVEEVENTREGISTERMESSAGE_H_

#include "eveMessage.h"
#include "eveEventProperty.h"

/*
 *
 */
class eveEventRegisterMessage  : public eveMessage
{
public:
	eveEventRegisterMessage(bool, eveEventProperty* );
	eveEventRegisterMessage(const eveEventRegisterMessage&);
	virtual ~eveEventRegisterMessage();
	bool isRegisterEvent(){return registerEvent;};
	eveEventProperty* takeEventProperty();
	int getEventId();

private:
	bool registerEvent;			// register or unregister
	eveEventProperty* event;
};


#endif /* EVEEVENTREGISTERMESSAGE_H_ */
