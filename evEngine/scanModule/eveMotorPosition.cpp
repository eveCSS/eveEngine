/*
 * eveMotorPosition.cpp
 *
 *  Created on: 15.12.2008
 *      Author: eden
 */

#include "eveMotorPosition.h"

eveMotorPosition::eveMotorPosition() {
	// TODO Auto-generated constructor stub

}

eveMotorPosition::~eveMotorPosition() {
	// TODO Auto-generated destructor stub
}

eveIntMotorPosition::eveIntMotorPosition(int value) {
	position = value;

}
eveIntMotorPosition::~eveIntMotorPosition() {
	// TODO Auto-generated destructor stub
}

eveDoubleMotorPosition::eveDoubleMotorPosition(double value) {
	position = value;

}
eveDoubleMotorPosition::~eveDoubleMotorPosition() {
	// TODO Auto-generated destructor stub
}

eveStringMotorPosition::eveStringMotorPosition(QString value) {
	position = value;

}

eveStringMotorPosition::~eveStringMotorPosition() {
	// TODO Auto-generated destructor stub
}
