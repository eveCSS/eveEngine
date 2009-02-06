/*
 * eveVariant.cpp
 *
 *  Created on: 20.01.2009
 *      Author: eden
 */

#include "eveVariant.h"

eveVariant::eveVariant() {
}
eveVariant::eveVariant(int value) : QVariant(value) {
}
eveVariant::eveVariant(double value) : QVariant(value) {
}
eveVariant::eveVariant(QString value) : QVariant(value) {
}

eveVariant::~eveVariant() {
}

/**
 * \brief set the base type of variant
 */
void eveVariant::setType(eveType type) {
	varianttype = type;
}

/**
 * \brief set the variant value without changing the base type
 */
bool eveVariant::setValue(int value) {
	bool ok=true;
	if (varianttype == eveSTRING)
		((QVariant *)this)->setValue(QString().setNum(value));
	else if (varianttype == eveDOUBLE)
		((QVariant *)this)->setValue((double)value);
	else
		((QVariant *)this)->setValue(value);
	return ok;
}
/**
 * \brief set the variant value without changing the base type
 */
bool eveVariant::setValue(double value) {
	bool ok=true;
	if (varianttype == eveSTRING)
		((QVariant *)this)->setValue(QString().setNum(value));
	else if (varianttype == eveDOUBLE)
		((QVariant *)this)->setValue(value);
	else
		((QVariant *)this)->setValue((int)value);
	return ok;
}
/**
 * \brief set the variant value without changing the base type
 */
bool eveVariant::setValue(QString value) {
	bool ok=true;
	if (varianttype == eveSTRING)
		((QVariant *)this)->setValue(value);
	else if (varianttype == eveDOUBLE)
		((QVariant *)this)->setValue(value.toDouble(&ok));
	else
		((QVariant *)this)->setValue(value.toInt(&ok));
	return ok;
}

eveVariant eveVariant::operator+ (const eveVariant& other){
	eveVariant result;

	if (varianttype == eveDOUBLE){
		result.setType(eveDOUBLE);
		result.setValue(toDouble() + other.toDouble());
	}
	else if (varianttype == eveINT){
		result.setType(eveINT);
		result.setValue(toInt() + other.toInt());
	}
	return result;
}

bool eveVariant::operator> (const eveVariant& other){

	if (varianttype == eveDOUBLE){
		if (toDouble() > other.toDouble()) return true;
	}
	else if (varianttype == eveINT){
		if (toInt() > other.toInt()) return true;
	}
	return false;
}

bool eveVariant::operator< (const eveVariant& other){

	if (varianttype == eveDOUBLE){
		if (toDouble() < other.toDouble()) return true;
	}
	else if (varianttype == eveINT){
		if (toInt() < other.toInt()) return true;
	}
	return false;
}

bool eveVariant::operator<= (const eveVariant& other){

	if ((*this < other) || (*this == other)) return true;
	return false;
}

bool eveVariant::operator>= (const eveVariant& other){

	if ((*this > other) || (*this == other)) return true;
	return false;
}

eveVariant eveVariant::operator* (const eveVariant& other){

	eveVariant result;
	if (varianttype == eveDOUBLE){
		result.setType(eveDOUBLE);
		result.setValue(toDouble() * other.toDouble());
	}
	else if (varianttype == eveINT){
		result.setType(eveINT);
		result.setValue(toInt() * other.toInt());
	}
	return result;
}
