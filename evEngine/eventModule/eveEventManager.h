/*
 * eveEventManager.h
 *
 *  Created on: 28.09.2009
 *      Author: eden
 */

#ifndef EVEEVENTMANAGER_H_
#define EVEEVENTMANAGER_H_

#include <QObject>
#include <QHash>
#include "eveMessageChannel.h"
#include "eveEventProperty.h"
#include "eveEventRegisterMessage.h"
#include "eveDeviceMonitor.h"

/*
 *
 */
class eveEventManager : public eveMessageChannel {
public:
	eveEventManager();
	virtual ~eveEventManager();
	void shutdown();
	void handleMessage(eveMessage *);
	void sendError(int, int, QString);
	void sendError(int, int, int, QString);

private:
	bool shutdownPending;
	void registerEvent(eveEventRegisterMessage*);
	void triggerSchedule(int, int, eveChainStatusMessage*);
	QHash<quint64, eveEventProperty* > scheduleHash;
	QHash<QString, eveEventProperty* > detectorHash;
	QHash<eveEventProperty*, eveDeviceMonitor* > monitorHash;
	QList<eveDeviceMonitor*> moniOnlyList;

};

#endif /* EVEEVENTMANAGER_H_ */
