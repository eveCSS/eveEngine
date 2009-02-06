/*
 * eveBaseTransport.h
 *
 *  Created on: 13.01.2009
 *      Author: eden
 */

#ifndef EVEBASETRANSPORT_H_
#define EVEBASETRANSPORT_H_

#include <QObject>
#include "eveMessage.h"
#include "eveVariant.h"

class eveBaseTransport: public QObject{

	Q_OBJECT

public:
	eveBaseTransport();
	virtual ~eveBaseTransport();
	virtual int readData(bool)=0;
	virtual int writeData(eveVariant, bool)=0;
	virtual int connectTrans()=0;
	virtual bool isConnected()=0;
	virtual eveDataMessage *getData(){return newData;};


signals:
	void done(int);

protected:
	void setData(eveDataMessage *data){newData=data;};

private:
	eveDataMessage *newData;
	int readTimeout;		// in ms
	int writeTimeout;		// in ms
};

#endif /* EVEBASETRANSPORT_H_ */
