/*
 * eveScanModule.h
 *
 *  Created on: 16.09.2008
 *      Author: eden
 */

#ifndef EVESCANMODULE_H_
#define EVESCANMODULE_H_

#include <QObject>
#include <QDomElement>
#include <QHash>
#include "eveMessage.h"
#include "eveSMAxis.h"
#include "eveSMChannel.h"
#include "eveXMLReader.h"
#include "eveScanManager.h"
#include "eveCalc.h"
#include "eveSMStatus.h"
#include "eveSMDetector.h"
#include "eveSMMotor.h"

class eveScanManager;

/**
 * \brief process a ScanModule
 */

enum stageT {eveStgINIT, eveStgREADPOS, eveStgSTARTEXECUTING, eveStgGOTOSTART, eveStgPRESCAN,
             eveStgSETTLETIME, eveStgTRIGWAIT, eveStgTRIGREAD, eveStgNEXTPOS,
             eveStgPOSTSCAN, eveStgENDPOS, eveStgFINISH};

class eveScanModule: public QObject
{

    Q_OBJECT

public:
    eveScanModule(eveScanManager *, eveXMLReader *, int, int, smTypeT);
    virtual ~eveScanModule();
    bool isDone(){return myStatus.isDone();};
    bool isStageDone(){return currentStageReady;};
    //	bool isInitializing(){return ((currentStage == eveStgINIT) || (currentStage == eveStgREADPOS) || (currentStage == eveStgGOTOSTART));};
    //void setEventTrigger(bool val){eventTrigger = val;};
    bool isExecuting(){return myStatus.isExecuting();};
    void initialize();
    void sendError(int, int, int, QString);
    void sendMessage(eveMessage*);
    eveSMAxis* findAxis(QString);
    int getTotalSteps();
    int getChainId(){return chainId;};
    int getSmId(){return smId;};
    void readPos();
    bool newEvent(eveEventProperty*);
    void setDoRedo();

public slots:
    void execStage();

signals:
    void sigExecStage();
    void SMready();

private:
    void startExec();
    void stgInit();
    void eveStgStartExecuting();
    void stgGotoStart();
    void stgReadPos();
    void stgPrescan();
    void stgSettleTime();
    void stgTrigRead();
    void stgTrigWait();
    void stgNextPos();
    void stgPostscan();
    void stgEndPos();
    void stgFinish();
    void sendError(int, int, QString);
    void sendNextPos(){manager->nextPos();};
    smTypeT smType;
    int chainId;
    int smId;
    int expectedPositions;
    int currentPosition;
    eveSMStatus myStatus;
    stageT currentStage;
    engineStatusT engineStatus;
    QString storageHint;
    bool currentStageReady;
    int currentStageCounter;
    int signalCounter;
    int triggerRid;
    int triggerDetecRid;
    int triggerPosCount;
    bool eventTrigger;
    bool manualTrigger;
    bool manDetTrigger;
    bool doRedo;
    bool doBreak;
    bool setAxisOffset;
    int valuesPerPos;
    int perPosCount;
    //	bool catchedTrigger;
    //	bool catchedEventTrigger;
    //	bool catchedDetecTrigger;
    //	bool delayedStart;
    int settleDelay;
    int triggerDelay;
    QHash<stageT, void(eveScanModule::*)()> stageHash;
    eveScanManager* manager;
    eveScanModule* nestedSM;
    eveScanModule* appendedSM;
    QList<eveCalc *> positionerList;
    QList<QHash<QString, QString>* >* posPluginDataList;
    QList<eveSMDevice *> *preScanList;
    QList<eveSMDevice *> *postScanList;
    QList<eveSMAxis *> *axisList;
    QList<eveSMChannel *> *channelList;
    QList<eveEventProperty*>* eventList;
    QList<eveSMDetector *> detectorList;
    QList<eveSMMotor *> motorList;
    QTime triggerTime;
    QTime settleTime;
};

#endif /* EVESCANMODULE_H_ */
