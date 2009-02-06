/*
 * eveSetValue.h
 *
 *  Created on: 15.12.2008
 *      Author: eden
 */

#ifndef EVESETVALUE_H_
#define EVESETVALUE_H_

#include <QString>
#include "eveTypes.h"

// TODO remove this
class eveSetValue;

class eveSetValue {
public:
	eveSetValue();
	virtual void setValue(int)=0;
	virtual void setValue(double)=0;
	virtual bool setValue(QString)=0;
	virtual ~eveSetValue();
	eveType getType(){return stype;};
	void operator = (eveSetValue*);
	void operator += (eveSetValue*);
//	virtual eveSetValue* operator + (eveSetValue*)=0;

protected:
	eveType stype;
};

class eveIntSetValue : public eveSetValue {
public:
	eveIntSetValue(int);
	void setValue(int ival){value = ival;};
	void setValue(double dval){value = (int)dval;};
	bool setValue(QString sval){bool ok; value = sval.toInt(&ok);return ok;};
	int getValue(){return value;};
	virtual ~eveIntSetValue();

private:
	int value;
};

class eveDoubleSetValue : public eveSetValue {
public:
	eveDoubleSetValue(double);
	void setValue(int ival){value = (double)ival;};
	void setValue(double dval){value = dval;};
	bool setValue(QString sval){bool ok; value = sval.toDouble(&ok);return ok;};
	double getValue(){return value;};
	virtual ~eveDoubleSetValue();
private:
	double value;
};

class eveStringSetValue : public eveSetValue {
public:
	eveStringSetValue(QString);
	void setValue(int ival){value = QString().arg(ival);};
	void setValue(double dval){value = QString().arg(dval);};
	bool setValue(QString sval){value = sval; return true;};
	QString getValue(){return value;};
	virtual ~eveStringSetValue();
private:
	QString value;
};
#endif /* EVESETVALUE_H_ */
