/*
 * eveMathThread.h
 *
 *  Created on: 20.05.2010
 *      Author: eden
 */

#ifndef EVEMATHTHREAD_H_
#define EVEMATHTHREAD_H_

#include <QThread>
#include "eveMathConfig.h"
#include "eveMathManager.h"

/*
 *
 */
class eveMathThread : public QThread
{
public:
	eveMathThread(int, int, QList<eveMathConfig*>* );
	virtual ~eveMathThread();
	void run();

private:
	int chid;
	int storageChannel;
	QList<eveMathConfig*>* mathConfigList;
};

#endif /* EVEMATHTHREAD_H_ */
