/*
 * eveStatusTracker.h
 *
 *  Created on: 25.09.2008
 *      Author: eden
 */

#ifndef EVESTATUSTRACKER_H_
#define EVESTATUSTRACKER_H_

#include <QHash>
#include "eveMessage.h"


/**
 * \brief keep track of all incoming chainStatus- and engineStatus messages
 *
 */
class eveBasicStatusTracker : public QObject{

	Q_OBJECT

public:
	eveBasicStatusTracker();
	virtual ~eveBasicStatusTracker();
	bool setChainStatus(eveChainStatusMessage*);
	void addStorageId(int id){ chidWithStorageList.append(id);};
	virtual eveEngineStatusMessage* getEngineStatusMessage();
	engineStatusT getEngineStatus(){return engineStatus;};

	//bool smChanged();
	//bool chainChanged();
	//bool isPaused();

signals:
	void engineIdle();

protected:
	QHash<int, chainStatusT> chainStatus;
	QList<int> chidWithStorageList;	// all chains with a storage module
	engineStatusT engineStatus;
	QString XmlName;
	bool loadedXML;

};

/**
 * \brief tracks chain status for Storage
 *
 * unsure if we need this
 */
class eveStatusTracker : public eveBasicStatusTracker {
public:
	eveStatusTracker();
	virtual ~eveStatusTracker();

};

/**
 * \brief tracks chain status and commands for manager
 *
 * If START, PAUSE, HALT, STOP command is received, engine status changes to
 * STARTED, PAUSED, HALTED, STOPPED.
 * Engine status is set to EXECUTING or IDLE by the appropriate
 * chainStatus Message.
 * Commands are accepted only, if engineStatus is EXECUTING or IDLE.
 * Exception: HALT will is always accepted. (README_Controls)
 */
class eveManagerStatusTracker : public eveBasicStatusTracker {

public:
	eveManagerStatusTracker();
	virtual ~eveManagerStatusTracker();
	virtual eveEngineStatusMessage* getEngineStatusMessage();

	bool setStart();
	bool setStop();
	bool setBreak();
	bool setHalt();
	bool setPause();
	bool setXMLLoaded(bool);
	bool setLoadingXML(QString);
	bool isXmlLoaded(){return loadedXML;};
	bool isNoXmlLoaded(){return engineStatus == eveEngIDLENOXML;};
	bool setAutoStart(bool);
	bool isAutoStart(){return autoStart;};

private:
	//bool engineStatusChanged;
	bool autoStart;

};

#endif /* EVESTATUSTRACKER_H_ */
