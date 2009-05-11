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

enum eveAxisStatusT {eveAXISINIT, eveAXISIDLE, eveAXISWRITEPOS, eveAXISREADPOS, eveAXISREADDEADBAND};

class eveScanManager;

class eveSMAxis: public QObject {

	Q_OBJECT

public:
	eveSMAxis(eveMotorAxis *, evePosCalc *);
	virtual ~eveSMAxis();
	void gotoStartPos(bool);
	void gotoNextPos(bool);
	// bool isAtNextPos();
	bool isAtEndPos();
	//eveSetValue* getPos();
	void execQueue();
	void init();
	bool isDone(){return ready;};
	QString getName(){return name;};
	QString getUnit(){return unit;};

public slots:
	void transportReady(int);

signals:
	void axisDone();

private:
	void sendError(int, int, QString);
	void initReady();
	void gotoPos(eveVariant, bool);
	bool ready;
	bool inDeadband;
	QString id;
	QString name;
	QString unit;
	eveVariant currentPosition;
	eveVariant targetPosition;
	eveVariant deadbandValue;
	QList<eveTransportT> transportList;
	int signalCounter;
	eveAxisStatusT axisStatus;
	eveScanManager* scanManager;
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
};

#endif /* EVESMAXIS_H_ */
