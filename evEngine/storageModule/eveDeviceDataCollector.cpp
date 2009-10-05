/*
 * eveDeviceDataCollector.cpp
 *
 *  Created on: 03.09.2009
 *      Author: eden
 */

#include "eveDeviceDataCollector.h"
#include "eveStorageManager.h"

eveDeviceDataCollector::eveDeviceDataCollector(eveStorageManager* sman, eveDevInfoMessage* imessage) {

	initialized = false;
	counter =0;
	xmlId = imessage->getChainId();
	name = imessage->getName();
	transInfo = new QStringList(*(imessage->getText()));
	manager = sman;
}

eveDeviceDataCollector::~eveDeviceDataCollector() {
	transInfo->clear();
	delete transInfo;
}

void eveDeviceDataCollector::addData(eveDataMessage* message) {

	// the first data message initializes the data-type
	if (!initialized){
		dataType = (eveType)message->getDataType();
		initialized = true;
	}

	if (dataType != (eveType)message->getDataType()){
		manager->sendError(ERROR, 0, QString("DeviceDataCollector: device %1 has changed datatype").arg(name));
	}
	else if (message->getDataMod() != DMTunmodified){
		// calculated data
		manager->sendError(INFO, 0, QString("DeviceDataCollector: modified data for device %1 not yet implemented").arg(name));
	}
	else if ( message->getArraySize() > 1) {
		manager->sendError(INFO, 0, QString("DeviceDataCollector: array data for %1, not yet implemented").arg(name));
	}
	else {
		switch (dataType) {
			case eveInt8T:					/* eveInt8 */
				dataArrayChar += message->getCharArray();
			break;
			case eveInt16T:					/* eveInt16 */
				dataArrayShort += message->getShortArray();
			break;
			case eveInt32T:					/* eveInt32 */
				dataArrayInt += message->getIntArray();
			break;
			case eveFloat32T:					/* eveFloat32 */
				dataArrayFloat += message->getFloatArray();
			break;
			case eveFloat64T:					/* eveFloat64 */
				dataArrayDouble += message->getDoubleArray();
			break;
			case eveStringT:					/* eveString */
				dataStrings += message->getStringArray();
			break;
			default:
				manager->sendError(INFO, 0, QString("DeviceDataCollector: unknown data type %1").arg(name));
			break;
		}
	}
}
