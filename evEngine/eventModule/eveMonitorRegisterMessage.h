/*
 * eveMonitorRegisterMessage.h
 *
 *  Created on: 16.05.2012
 *      Author: eden
 */

#ifndef EVEMONITORREGISTERMESSAGE_H_
#define EVEMONITORREGISTERMESSAGE_H_

#include "eveMessage.h"
#include "eveDevice.h"

class eveMonitorRegisterMessage: public eveMessage {
public:
	eveMonitorRegisterMessage(eveDevice *, int);
	virtual ~eveMonitorRegisterMessage();
	QString getName(){return name;};
	QString getXMLId(){return xmlid;};
	int getStorageChannel(){return storageChannel;};
	eveTransportDef* getTransport(){return transport;};

private:
	eveTransportDef* transport;
	QString xmlid;
	QString name;
	int storageChannel;
};

#endif /* EVEMONITORREGISTERMESSAGE_H_ */
