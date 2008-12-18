/*
 * eveMotorPosition.h
 *
 *  Created on: 15.12.2008
 *      Author: eden
 */

#ifndef EVEMOTORPOSITION_H_
#define EVEMOTORPOSITION_H_

#include <QString>
#include "eveTypes.h"

// TODO remove this
class eveMotorPosition;

class eveMotorPosition {
public:
	eveMotorPosition();
	virtual ~eveMotorPosition();
private:
	eveType type;
};

class eveIntMotorPosition : public eveMotorPosition {
public:
	eveIntMotorPosition(int);
	void setPosition(int pos){position = pos;};
	int getPosition(){return position;};
	virtual ~eveIntMotorPosition();

private:
	int position;
};

class eveDoubleMotorPosition : public eveMotorPosition {
public:
	eveDoubleMotorPosition(double);
	void setPosition(double pos){position = pos;};
	double getPosition(){return position;};
	virtual ~eveDoubleMotorPosition();
private:
	double position;
};

class eveStringMotorPosition : public eveMotorPosition {
public:
	eveStringMotorPosition(QString);
	void setPosition(QString pos){position = pos;};
	QString getPosition(){return position;};
	virtual ~eveStringMotorPosition();
private:
	QString position;
};
#endif /* EVEMOTORPOSITION_H_ */
