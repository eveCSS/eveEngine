/*
 * eveVariant.cpp
 *
 *  Created on: 20.01.2009
 *      Author: eden
 */

#include <QDate>
#include <QTime>
#include <QRegExp>
#include "eveVariant.h"

eveVariant::eveVariant() {
}
eveVariant::eveVariant(int value) : QVariant(value) {
	varianttype = eveINT;
}
eveVariant::eveVariant(double value) : QVariant(value) {
	varianttype = eveDOUBLE;
}
eveVariant::eveVariant(QString value) : QVariant(value) {
	varianttype = eveSTRING;
}
eveVariant::eveVariant(QVariant value) : QVariant(value) {
	varianttype = eveUnknownT;
	if (value.type() == QVariant::Int)
		varianttype = eveINT;
	else if (value.type() == QVariant::Double)
		varianttype = eveDOUBLE;
	else if (value.type() == QVariant::DateTime)
		varianttype = eveDateTimeT;
	else if (value.type() == QVariant::String)
		varianttype = eveSTRING;
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
	else if (varianttype == eveDateTimeT)
		((QVariant *)this)->setValue(QDateTime::fromString(QString("0001-01-01 00:00:00"), QString("yyyy-MM-dd hh:mm:ss")).addMSecs(value*1000));
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
	else if (varianttype == eveDateTimeT)
		((QVariant *)this)->setValue(QDateTime::fromString(QString("0001-01-01 00:00:00"), QString("yyyy-MM-dd hh:mm:ss")).addMSecs((int)(value*1000.)));
	else
		((QVariant *)this)->setValue((int)value);
	return ok;
}
/**
 * \brief set the variant value without changing the base type
 */
bool eveVariant::setValue(QString value) {
	bool ok=true;
	QString format;
	if (varianttype == eveSTRING)
		((QVariant *)this)->setValue(value);
	else if (varianttype == eveDOUBLE)
		((QVariant *)this)->setValue(value.toDouble(&ok));
	else if (varianttype == eveDateTimeT){
		// this is always an absolute DateTime, if date is not present, use today
		if (value.contains(QRegExp("\\d{4}-\\d{2}-\\d{2} "))){
			if (value.contains("."))
				format = "yyyy-MM-dd HH:mm:ss.z";
			else
				format = "yyyy-MM-dd HH:mm:ss";
			QDateTime dt = QDateTime::fromString(value, format);
			if (!dt.isValid()){
				dt = QDateTime::currentDateTime();
				ok = false;
			}
			((QVariant *)this)->setValue(dt);
		}
		else if (value.contains(QRegExp("\\d{2}:\\d{2}:\\d{2}"))){
			if (value.contains("."))
				format = "HH:mm:ss.z";
			else
				format = "HH:mm:ss";
			QTime qtim = QTime::fromString(value, format);
			if (!qtim.isValid()){
				qtim = QTime();
				ok = false;
			}
			((QVariant *)this)->setValue(QDateTime(QDate::currentDate(), qtim));
		}
		else
			ok = false;
	}
	else
		((QVariant *)this)->setValue(value.toInt(&ok));
	return ok;
}

/**
 * \brief set the variant value without changing the base type
 */
bool eveVariant::setValue(QDateTime value) {
	bool ok=true;
	if (varianttype == eveSTRING)
		((QVariant *)this)->setValue(value.toString("yyyy-MM-dd hh:mm:ss"));
	else if (varianttype == eveDOUBLE)
		ok = false;
	else if (varianttype == eveDateTimeT)
		((QVariant *)this)->setValue(value);
	else
		ok = false;
	return ok;
}

eveVariant eveVariant::abs() {
	if (*this < 0.0)
		return *this * (-1.0);
	else
		return *this;
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
	else if ((varianttype == eveDateTimeT) && (other.getType() == eveDateTimeT)){
		// we assume other is a relative time without date
		result.setType(eveDateTimeT);
		int relativeMSecs = -1 * other.toTime().msecsTo(QTime());
		result.setValue(toDateTime().addMSecs((qint64)relativeMSecs));
	}
	else if ((varianttype == eveDateTimeT) && (other.getType() == eveDOUBLE)){
		result.setType(eveDateTimeT);
		result.setValue(toDateTime().addMSecs((qint64)(1000.0 * other.toDouble())));
	}
	else if ((varianttype == eveDateTimeT) && (other.getType() == eveINT)){
		result.setType(eveDateTimeT);
		result.setValue(toDateTime().addSecs(other.toInt()));
	}
	return result;
}

eveVariant eveVariant::operator- (const eveVariant& other){
	eveVariant result;

	if (varianttype == eveDOUBLE){
		result.setType(eveDOUBLE);
		result.setValue(toDouble() - other.toDouble());
	}
	else if (varianttype == eveINT){
		result.setType(eveINT);
		result.setValue(toInt() - other.toInt());
	}
	else if ((varianttype == eveDateTimeT) && (other.getType() == eveDateTimeT)){
		result.setType(eveDateTimeT);
		int relativeMSecs = other.toTime().msecsTo(QTime());
		result.setValue(toDateTime().addMSecs((qint64)relativeMSecs));
	}
	else if ((varianttype == eveDateTimeT) && (other.getType() == eveDOUBLE)){
		result.setType(eveDateTimeT);
		result.setValue(toDateTime().addMSecs((qint64)(-1000.0 * other.toDouble())));
	}
	else if ((varianttype == eveDateTimeT) && (other.getType() == eveINT)){
		result.setType(eveDateTimeT);
		result.setValue(toDateTime().addSecs(-1 * other.toInt()));
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
	else if (varianttype == eveDateTimeT){
		if (toDateTime() > other.toDateTime()) return true;
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
	else if (varianttype == eveDateTimeT){
		if (toDateTime() < other.toDateTime()) return true;
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

quint64 eveVariant::getMangled(unsigned int val1, unsigned int val2){
	quint64 longval = val1;
	longval = (longval << 32) + val2;
	return longval;
}
