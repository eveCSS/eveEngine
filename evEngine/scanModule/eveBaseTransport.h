/*
 * eveBaseTransport.h
 *
 *  Created on: 13.01.2009
 *      Author: eden
 */

#ifndef EVEBASETRANSPORT_H_
#define EVEBASETRANSPORT_H_

#include <QObject>
#include "eveSMBaseDevice.h"
#include "eveMessage.h"
#include "eveVariant.h"

enum eveTransActionT {eveCONNECT, eveREAD, eveWRITE, eveIDLE };
enum eveTransStatusT {eveCONNECTED, eveNOTCONNECTED, eveTIMEOUT, eveUNDEFINED };

class eveBaseTransport: public QObject{

	Q_OBJECT

public:
	eveBaseTransport(eveSMBaseDevice *);
	virtual ~eveBaseTransport();
	virtual int readData(bool)=0;
	virtual int writeData(eveVariant, bool)=0;
	virtual int connectTrans()=0;
	virtual int monitorTrans()=0;
	virtual bool isConnected()=0;
	virtual bool haveData()=0;
	virtual eveDataMessage *getData()=0;
	virtual QStringList* getInfo()=0;

signals:
	void done(int);
	void valueChanged(eveVariant*);

protected:
	void setData(eveDataMessage *data){newData=data;};
	eveDataMessage *newData;

private:
	int readTimeout;		// in ms
	int writeTimeout;		// in ms
};

#endif /* EVEBASETRANSPORT_H_ */
