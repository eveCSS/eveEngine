/*
 * eveSMAxis.h
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#ifndef EVESMAXIS_H_
#define EVESMAXIS_H_

#include <QList>
#include "eveDevice.h"
#include "eveSetValue.h"
#include "evePosCalc.h"
#include "eveCaTransport.h"

/**
 * \brief motor axis for a specific SM
 *
 */

enum eveAxisStatusT {eveAXISINIT, eveAXISIDLE, eveAXISWRITEPOS, eveAXISREADPOS, eveAXISREADSTATUS};

class eveScanManager;

class eveSMAxis: public QObject {

	Q_OBJECT

public:
	eveSMAxis(eveScanModule *, eveMotorAxis *, evePosCalc *);
	virtual ~eveSMAxis();
	void gotoStartPos(bool);
	void gotoNextPos(bool);
	void gotoPos(eveVariant, bool);
	eveVariant getPos(){return currentPosition;};
	// bool isAtNextPos();
	void stop();
	bool isAtEndPos(){return posCalc->isAtEndPos();};
	bool isOK(){return axisOK;};
	//eveSetValue* getPos();
	void execQueue();
	void init();
	bool isDone(){return ready;};
	QString getName(){return name;};
	QString getxmlId(){return xmlId;};
	QString getUnit(){return unit;};
	eveDevInfoMessage* getDeviceInfo();
	eveDataMessage* getPositionMessage();

public slots:
	void transportReady(int);

signals:
	void axisDone();

private:
	void sendError(int, int, QString);
	void initAll();
	void signalReady();
	bool ready;
	bool inDeadband;
	bool axisStop;
	QString xmlId;
	QString name;
	QString unit;
	eveVariant currentPosition;
	eveVariant targetPosition;
	eveVariant deadbandValue;
	eveVariant stopValue;
	eveVariant triggerValue;
	QList<eveTransportT> transportList;
	eveDataMessage* curPosition;
	int signalCounter;
	eveAxisStatusT axisStatus;
	eveScanModule* scanModule;
	evePosCalc *posCalc;
	eveMotorAxis *axisDef;
	eveBaseTransport* gotoTrans;
	eveBaseTransport* posTrans;
	eveBaseTransport* stopTrans;
	eveBaseTransport* statusTrans;
	eveBaseTransport* triggerTrans;
	eveBaseTransport* deadbandTrans;
	eveBaseTransport* unitTrans;
	bool haveDeadband;
	bool haveTrigger;
	bool haveUnit;
	bool haveStatus;
	bool haveStop;
	bool havePos;
	bool haveGoto;
	bool axisOK;
	bool readUnit;
};

#endif /* EVESMAXIS_H_ */
