/*
 * eveSMAxis.h
 *
 *  Created on: 11.12.2008
 *      Author: eden
 */

#ifndef EVESMAXIS_H_
#define EVESMAXIS_H_

#include "eveDevice.h"
#include "eveMotorPosition.h"

/**
 * \brief motor axis for a specific SM
 */
class eveSMAxis: public eveMotorAxis {
public:
	eveSMAxis(const eveMotorAxis&);
	virtual ~eveSMAxis();
	void gotoStartPos();
	void gotoNextPost();
	bool isAtNextPos();
	bool isAtEndPos();
	eveMotorPosition getPos();

private:
	eveMotorPosition currentPosition;
	class evePosCalc;
};

#endif /* EVESMAXIS_H_ */
