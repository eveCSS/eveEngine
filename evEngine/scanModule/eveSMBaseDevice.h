/*
 * SMBaseDevice.h
 *
 *  Created on: 09.11.2009
 *      Author: eden
 */

#ifndef EVESMBASEDEVICE_H_
#define EVESMBASEDEVICE_H_

#include <QObject>

/*
 *
 */
class eveSMBaseDevice  : public QObject {

	Q_OBJECT

public:
	eveSMBaseDevice(QObject*);
	virtual ~eveSMBaseDevice();
	virtual void sendError(int, int, int, QString)=0;
	QString getName(){return name;};
	QString getXmlId(){return xmlId;};

protected:
	QString xmlId;
	QString name;
};

#endif /* EVESMBASEDEVICE_H_ */
