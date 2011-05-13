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
#include "eveSMBaseDevice.h"
#include "eveEventProperty.h"
#include "eveAverage.h"
#include "eveCalc.h"

class eveScanModule;

enum eveChannelStatusT {eveCHANNELINIT, eveCHANNELIDLE, eveCHANNELTRIGGER, eveCHANNELTRIGGERREAD, eveCHANNELREAD, eveCHANNELREADUNIT, eveCHANNELSTOP};


/**
 * \brief device in a scanModule
 */
class eveSMChannel : public eveSMBaseDevice {

	Q_OBJECT

public:
	eveSMChannel(eveScanModule*, eveDetectorChannel*, QHash<QString, QString>, QList<eveEventProperty* >*);
	virtual ~eveSMChannel();
	void init();
//	void trigger(bool);
	void triggerRead(bool);
	void stop(bool);
	bool isDone(){return ready;};
	bool isOK(){return channelOK;};
	bool hasConfirmTrigger(){return confirmTrigger;};
	QString getUnit(){return unit;};
	eveDevInfoMessage* getDeviceInfo();
	eveDataMessage* getValueMessage();
	void sendError(int, int, int, QString);
	void addPositioner(eveCalc* pos){positionerList.append(pos);};
	void loadPositioner(int pc);


public slots:
	void transportReady(int);
	void newEvent(eveEventProperty*);

signals:
	void channelDone();

private:
	void initAll();
	void sendError(int, int, QString);
	void read(bool);
	void signalReady();
	bool redo;
	bool ready;
	bool haveValue;
	bool haveStop;
	bool haveTrigger;
	bool haveUnit;
	bool channelOK;
	int signalCounter;
	QList<eveCalc *> positionerList;
	QList<eveTransportT> transportList;
	QList<eveEventProperty* >* eventList;
	eveChannelStatusT channelStatus;
	QString unit;
	bool sendreadyevent;
	eveType channelType;
	eveVariant currentValue;
	eveAverage* valueCalc;
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
