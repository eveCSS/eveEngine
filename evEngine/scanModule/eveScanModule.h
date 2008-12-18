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
#include "eveScanManager.h"
#include "eveXMLReader.h"
//#include "eveMessageChannel.h"

/**
 * \brief process a ScanModule
 */
enum smStatusT {eveSmNOTSTARTED, eveSmEXECUTING, eveSmPAUSED, eveSmBROKEN, eveSmHALTED, eveSmDONE} ;

enum stageT {eveStgINIT, eveStgMOTORINITIAL, eveStgGOTOSTART, eveStgPRESCAN,
			eveStgSETTLETIME, eveStgNEXT, eveStgTRIGREAD, eveStgPOSCHECK,
			eveStgPOSTSCAN, eveStgFINISH};

class eveScanModule: public QObject
{

	Q_OBJECT

public:
	eveScanModule(eveScanManager *, eveXMLReader *, QDomElement);
	virtual ~eveScanModule();
	bool isDone(){return (smstatus==eveSmDONE)?true:false;}
	bool resumeSM();
	void startSM();
	void initialize();

public slots:
	void stgReadMotorInitial();
	void stgInit();
	void stgGotoStart();
	void stgPrescan();
	void stgSettleTime();
	void stgNext();
	void stgTrigRead();
	void stgPosCheck();
	void stgPostscan();
	void stgFinish();
	void execStage();

signals:
	void sigExecStage();
	void SMready();

private:
	int chainId;
	int smId;
	smStatusT smstatus;
	stageT currentStage;
	engineStatusT engineStatus;
	bool currentStageReady;
	int currentStageCounter;
	bool isRoot;
	QHash<stageT, void(eveScanModule::*)()> stageHash;
	eveScanManager* manager;
	eveScanModule* nestedSM;
	eveScanModule* appendedSM;

};

#endif /* EVESCANMODULE_H_ */
