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
    void reset();
    bool setChainStatus(eveChainStatusMessage*);
    void setActiveStorage(int id){ haveActiveStorage.append(id);};
    void setActiveScan(int id){ haveActiveScan.append(id);};
    void setActiveMath(int id){ haveActiveMath.append(id);};
    virtual eveEngineStatusMessage* getEngineStatusMessage();
	engineStatusT getEngineStatus(){return engineStatus;};
    CHStatusT getChainStatus(int chid);

	int getRepeatCount(){return repeatCount;};
	void setRepeatCount(int count);
	void decrRepeatCount(){if (repeatCount > 0) --repeatCount;};
	//bool smChanged();
	//bool chainChanged();
	//bool isPaused();

signals:
	void engineIdle();

protected:
    QHash<int, CHStatusT> chainStatus;
    QList<int> haveActiveStorage;	// list of chains with a running storage module
    QList<int> haveActiveMath;	    // list of chains with a running math module
    QList<int> haveActiveScan;	    // list of chains with a running scan module
    engineStatusT engineStatus;
	QString XmlName;
	bool loadedXML;
	unsigned int repeatCount;

};


/**
 * \brief tracks chain status and commands for manager
 *
 * Engine status is set to EXECUTING or IDLE by the appropriate
 * chainStatus Message.
 * Commands are accepted only, if engineStatus is EXECUTING or IDLE.
 * Exception: HALT is always accepted. (README_Controls)
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
