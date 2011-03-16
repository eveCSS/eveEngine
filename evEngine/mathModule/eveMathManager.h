/*
 * eveMathManager.h
 *
 *  Created on: 20.05.2010
 *      Author: eden
 */

#ifndef EVEMATHMANAGER_H_
#define EVEMATHMANAGER_H_

/*
 *
 */
#include <QMultiHash>
#include "eveMessageChannel.h"
#include "eveMathConfig.h"
#include "eveMath.h"

class eveMathManager: public eveMessageChannel {
public:
	eveMathManager(int , QList<eveMathConfig*>* );
	virtual ~eveMathManager();
	void sendError(int, int, QString);
	void sendMessage(eveDataMessage* message){addMessage(message);};
	void shutdown();

private:
	void handleMessage(eveMessage *);
	int chid;
	QMultiHash<int, eveMath*> mathHash;
	bool shutdownPending;

};

#endif /* EVEMATHMANAGER_H_ */
