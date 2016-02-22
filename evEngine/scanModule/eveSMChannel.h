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

#include "eveDeviceDefinitions.h"
#include "eveVariant.h"
#include "eveCaTransport.h"
#include "eveSMBaseDevice.h"
#include "eveEventProperty.h"
#include "eveAverage.h"
#include "eveCalc.h"

class eveScanModule;
class eveSMDetector;

enum eveChannelStatusT {eveCHANNELINIT, eveCHANNELIDLE, eveCHANNELTRIGGERREAD, eveCHANNELREAD, eveCHANNELREADUNIT, eveCHANNELSTOP};


/**
 * \brief device in a scanModule
 */
class eveSMChannel : public eveSMBaseDevice {

    Q_OBJECT

public:
    eveSMChannel(eveScanModule*, eveSMDetector*, eveChannelDefinition*, QHash<QString, QString>, QList<eveEventProperty* >*, eveSMChannel*);
    virtual ~eveSMChannel();
    void init();
    void triggerRead(bool);
    void stop();
    bool isDone(){return ready;};
    bool isOK(){return channelOK;};
    bool isDeferred(){return deferredTrigger;};
    QString getUnit(){return unit;};
    virtual eveDevInfoMessage* getDeviceInfo(bool);
    eveDataMessage* getValueMessage();
    eveDataMessage* getNormValueMessage();
    eveDataMessage* getNormRawValueMessage();
    eveDataMessage* getAverageMessage();
    eveDataMessage* getAverageParamsMessage();
    eveDataMessage* getLimitMessage();
    void sendError(int, int, int, QString);
    void addPositioner(eveCalc* pos){positionerList.append(pos);};
    void loadPositioner(int pc);
    void setTimer(QDateTime start);
    bool readAtInit(){return timeoutShort;};
    eveSMDetector* getDetector(){return detector;};
    eveSMChannel* getNormalizeChannel(){return normalizeChannel;};

public slots:
    void transportReady(int);
    void normalizeChannelReady();
    void newEvent(eveEventProperty*);

signals:
    void channelDone();

private:
    virtual void signalReady();
    void sendError(int, int, QString);
    bool ready;
    bool delayedTrigger;
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
    bool isDetectorStop;
    bool isDetectorTrigger;
    bool isDetectorUnit;
    bool channelOK;
    bool deferredTrigger;
    bool longString;
    int signalCounter;
    QList<eveCalc *> positionerList;
    QList<eveTransportT> transportList;
    QList<eveEventProperty* >* eventList;
    eveChannelStatusT channelStatus;
    QString unit;
    bool sendreadyevent;
    eveType channelType;
    //	eveVariant currentValue;
    //        eveVariant normalizedValue;
    eveAverage* averageRaw;
    eveAverage* averageNormCalc;
    eveAverage* averageNormRaw;
    eveDataMessage* valueRawMsg;
    eveDataMessage* normRawMsg;
    eveDataMessage* normCalcMsg;
    eveDataMessage* averageMsg;
    eveDataMessage* averageParamsMsg;
    eveDataMessage* limitMsg;
    double valueRaw;
    double normCalc;
    double normRaw;
    eveScanModule* scanModule;
    eveBaseTransport* valueTrans;
    eveBaseTransport* stopTrans;
    eveBaseTransport* triggerTrans;
    eveBaseTransport* unitTrans;
    eveVariant triggerValue;
    eveVariant stopValue;
    int averageCount, maxAttempts;
    double maxDeviation, minimum;
    eveSMDetector* detector;

};

#endif /* EVESMCHANNEL_H_ */
