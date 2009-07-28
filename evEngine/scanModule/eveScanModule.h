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
	bool resumeSM();
	bool breakNestedSM();
	void initialize();
	void sendError(int, int, int, QString);
	void gotoStart();
	void readPos();

public slots:
	void execStage();

	void startSM(bool);
	void stopSM(bool);
	void breakSM(bool);
	void pauseSM(bool);
	void haltSM(bool);
	void redoSM(bool);

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
