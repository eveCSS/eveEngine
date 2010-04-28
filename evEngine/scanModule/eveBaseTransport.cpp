/*
 * eveBaseTransport.cpp
 *
 *  Created on: 13.01.2009
 *      Author: eden
 */

#include "eveBaseTransport.h"

eveBaseTransport::eveBaseTransport(eveSMBaseDevice *parent, QString xmlid, QString devname) : QObject(parent) {
	xmlId = xmlid;
	name = devname;
}

eveBaseTransport::~eveBaseTransport() {
	// TODO Auto-generated destructor stub
}
