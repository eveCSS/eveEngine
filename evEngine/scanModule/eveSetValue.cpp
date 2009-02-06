/*
 * eveSetValue.cpp
 *
 *  Created on: 15.12.2008
 *      Author: eden
 */

#include "eveSetValue.h"

eveSetValue::eveSetValue() {
	// TODO Auto-generated constructor stub
}

eveSetValue::~eveSetValue() {
	// TODO Auto-generated destructor stub
}

void eveSetValue::operator= (eveSetValue* other){
	switch (other->getType()) {
		case eveDOUBLE:
			setValue(((eveDoubleSetValue*)other)->getValue());
			break;
		case eveSTRING:
			setValue(((eveStringSetValue*)other)->getValue());
			break;
		default:
			setValue(((eveIntSetValue*)other)->getValue());
			break;
	}
}

void eveSetValue::operator+= (eveSetValue* other){
	switch (other->getType()) {
		case eveDOUBLE:
			if (stype == eveDOUBLE)
				setValue(((eveDoubleSetValue*)this)->getValue() + ((eveDoubleSetValue*)other)->getValue());
			else if (stype == eveINT)
				setValue(((eveIntSetValue*)this)->getValue() + (int)(((eveDoubleSetValue*)other)->getValue()));
			break;
		case eveINT:
			if (stype == eveDOUBLE)
				setValue(((eveDoubleSetValue*)this)->getValue() + (double)(((eveIntSetValue*)other)->getValue()));
			else if (stype == eveINT)
				setValue(((eveIntSetValue*)this)->getValue() + ((eveIntSetValue*)other)->getValue());
			break;
		default:
			break;
	}
}

eveIntSetValue::eveIntSetValue(int ival) {
	value = ival;
	stype = eveINT;
}

eveIntSetValue::~eveIntSetValue() {
	// TODO Auto-generated destructor stub
}

eveDoubleSetValue::eveDoubleSetValue(double dval) {
	value = dval;
	stype = eveDOUBLE;
}
eveDoubleSetValue::~eveDoubleSetValue() {
	// TODO Auto-generated destructor stub
}

eveStringSetValue::eveStringSetValue(QString sval) {
	value = sval;
	stype = eveSTRING;
}

eveStringSetValue::~eveStringSetValue() {
	// TODO Auto-generated destructor stub
}
