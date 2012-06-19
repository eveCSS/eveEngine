/*
 * eveMonitorRegisterMessage.h
 *
 *  Created on: 16.05.2012
 *      Author: eden
 */

#ifndef EVEMONITORREGISTERMESSAGE_H_
#define EVEMONITORREGISTERMESSAGE_H_

#include "eveMessage.h"
#include "eveDeviceDefinitions.h"

class eveMonitorRegisterMessage: public eveMessage {
public:
	eveMonitorRegisterMessage(eveDeviceDefinition *, int);
	virtual ~eveMonitorRegisterMessage();
	QString getName(){return name;};
	QString getXMLId(){return xmlid;};
	int getStorageChannel(){return storageChannel;};
	eveTransportDefinition* getTransport(){return transport;};

private:
	eveTransportDefinition* transport;
	QString xmlid;
	QString name;
	int storageChannel;
};

#endif /* EVEMONITORREGISTERMESSAGE_H_ */
