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
	eveSMChannel(eveScanModule*, eveChannelDefinition*, QHash<QString, QString>, QList<eveEventProperty* >*, eveSMChannel*);
	virtual ~eveSMChannel();
	void init();
	void triggerRead(bool);
	void stop(bool);
	bool isDone(){return ready;};
	bool isOK(){return channelOK;};
	bool hasConfirmTrigger(){return confirmTrigger;};
	QString getUnit(){return unit;};
	virtual eveDevInfoMessage* getDeviceInfo();
	eveDataMessage* getValueMessage();
	void sendError(int, int, int, QString);
	void addPositioner(eveCalc* pos){positionerList.append(pos);};
	void loadPositioner(int pc);
	void setTimer(QDateTime start);
	bool readAtInit(){return timeoutShort;};
	eveSMDetector* getDetector(){return detector;};

public slots:
	void transportReady(int);
	void normalizeChannelReady();
	void newEvent(eveEventProperty*);

signals:
	virtual void channelDone();

private:
	virtual void signalReady();
	void sendError(int, int, QString);
	bool ready;
	void initAll();
	void read(bool);
	bool retrieveData();
	eveSMChannel* normalizeChannel;
	bool timeoutShort;
	bool isTimer;
	bool redo;
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
	bool isDetectorTrigger;
	bool isDetectorUnit;
	eveSMDetector* detector;

};



#endif /* EVESMCHANNEL_H_ */
