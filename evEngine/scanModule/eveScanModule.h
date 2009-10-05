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
//#include "eveMessageChannel.h"

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
	eveScanModule(eveScanManager *, eveXMLReader *, int, int);
	virtual ~eveScanModule();
	bool isDone(){return (smStatus==eveSmDONE)?true:false;}
	void initialize();
	void sendError(int, int, int, QString);
	void gotoStart();
	void readPos();
	void start();

	bool startSM(int);
	bool stopSM(int);
	bool breakSM(int);
	bool pauseSM(int);
	bool resumeSM(int);
	bool haltSM(int);
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
	void sendMessage(eveBaseDataMessage*);
	void sendNextPos(){manager->nextPos();};
	int chainId;
	int smId;
	smStatusT smStatus, smLastStatus;
	stageT currentStage;
	engineStatusT engineStatus;
	bool currentStageReady;
	int currentStageCounter;
	int signalCounter;
	bool isRoot;
	bool catchedRedo;
	double triggerDelay;
	QHash<stageT, void(eveScanModule::*)()> stageHash;
	eveScanManager* manager;
	eveScanModule* nestedSM;
	eveScanModule* appendedSM;
	QList<eveSMDevice *> *preScanList;
	QList<eveSMDevice *> *postScanList;
	QList<eveSMAxis *> *axisList;
	QList<eveSMChannel *> *channelList;

};

#endif /* EVESCANMODULE_H_ */
