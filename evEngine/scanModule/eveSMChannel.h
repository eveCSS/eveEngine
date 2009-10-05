/*
 * eveSMChannel.h
 *
 *  Created on: 04.06.2009
 *      Author: eden
 */

#ifndef EVESMCHANNEL_H_
#define EVESMCHANNEL_H_

#include <QObject>
#include <QList>
#include <QHash>

#include "eveDevice.h"
#include "eveVariant.h"
#include "eveCaTransport.h"

class eveScanModule;

enum eveChannelStatusT {eveCHANNELINIT, eveCHANNELIDLE, eveCHANNELTRIGGER, eveCHANNELTRIGGERREAD, eveCHANNELREAD, eveCHANNELREADUNIT, eveCHANNELSTOP};


/**
 * \brief device in a scanModule
 */
class eveSMChannel : public QObject {

	Q_OBJECT

public:
	eveSMChannel(eveScanModule*, eveDetectorChannel*, QHash<QString, QString>);
	virtual ~eveSMChannel();
	void init();
	void trigger(bool);
	void triggerRead(bool);
	void stop(bool);
	void read(bool);
	bool isDone(){return ready;};
	bool isOK(){return channelOK;};
	QString getName(){return name;};
	QString getUnit(){return unit;};
	eveDevInfoMessage* getDeviceInfo();
	eveDataMessage* getValueMessage();

public slots:
	void transportReady(int);

signals:
	void channelDone();

private:
	void initAll();
	void sendError(int, int, QString);
	void signalReady();
	bool ready;
	bool haveValue;
	bool haveStop;
	bool haveTrigger;
	bool haveUnit;
	bool channelOK;
	int signalCounter;
	QList<eveTransportT> transportList;
	eveChannelStatusT channelStatus;
	QString xmlId;
	QString name;
	QString unit, readyeventId;
	eveType channelType;
	eveScanModule* scanModule;
	eveBaseTransport* valueTrans;
	eveBaseTransport* stopTrans;
	eveBaseTransport* triggerTrans;
	eveBaseTransport* unitTrans;
	eveDataMessage *curValue;
	eveVariant triggerValue;
	eveVariant stopValue;
	int averageCount, maxAttempts;
	double maxDeviation, minimum;
	bool confirmTrigger;

};



#endif /* EVESMCHANNEL_H_ */
