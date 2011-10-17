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

class eveScanManager;

/**
 * \brief process a ScanModule
 */

enum stageT {eveStgINIT, eveStgREADPOS, eveStgGOTOSTART, eveStgPRESCAN,
			eveStgSETTLETIME, eveStgTRIGREAD, eveStgNEXTPOS,
			eveStgPOSTSCAN, eveStgENDPOS, eveStgFINISH};

class eveScanModule: public QObject
{

	Q_OBJECT

public:
	eveScanModule(eveScanManager *, eveXMLReader *, int, int, smTypeT);
	virtual ~eveScanModule();
	bool isDone(){return (smStatus==eveSmDONE)?true:false;};
	bool initDone(){return (currentStage==eveStgREADPOS)?true:false;};
	// smStatusT getStatus(){return smStatus;};
	void initialize();
	void sendError(int, int, int, QString);
	void sendMessage(eveMessage*);
	eveSMAxis* findAxis(QString);
	int getRemainingTime();
	void gotoStart();
	void readPos();

	bool startSM(int);
	bool stopSM(int);
	bool breakSM(int);
	bool pauseSM(int);
	bool resumeSM(int);
	bool haltSM(int);
	bool triggerSM(int, int);
	bool redoSM(int);

	void startChain();
	void stopChain();
	bool breakChain();
	void pauseChain();
	bool resumeChain();
	void haltChain();
	void redoChain();

public slots:
	void execStage();

signals:
	void sigExecStage();
	void SMready();

private:
	void start();
	void stgInit();
	void stgGotoStart();
	void stgReadPos();
	void stgPrescan();
	void stgSettleTime();
	void stgTrigRead();
	void stgNextPos();
	void stgPostscan();
	void stgEndPos();
	void stgFinish();
	void sendError(int, int, QString);
	void sendNextPos(){manager->nextPos();};
	smTypeT smType;
	int chainId;
	int smId;
	int totalSteps;
	int currentPosition;
	smStatusT smStatus, smLastStatus;
	stageT currentStage;
	engineStatusT engineStatus;
	bool currentStageReady;
	int currentStageCounter;
	int signalCounter;
	int triggerRid;
	int triggerDetecRid;
	int triggerPosCount;
	bool triggerConfirm;
	bool triggerDetecConf;
	bool catchedRedo;
	bool catchedTrigger;
	bool catchedDetecTrigger;
	bool delayedStart;
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
	QTime scanTimer;
	QTime triggerTime;
	QTime settleTime;
};

#endif /* EVESCANMODULE_H_ */
