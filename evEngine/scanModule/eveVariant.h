/*
 * eveVariant.h
 *
 *  Created on: 20.01.2009
 *      Author: eden
 */

#ifndef EVEVARIANT_H_
#define EVEVARIANT_H_

#include <QVariant>
#include <QDateTime>
#include <QVariant>
#include "eveTypes.h"

class eveVariant : public QVariant {
public:
	eveVariant();
	eveVariant(int);
	eveVariant(double);
	eveVariant(QString);
	eveVariant(QVariant);
	virtual ~eveVariant();
	void setType(eveType);
	eveType getType() const {return varianttype;};
	bool setValue(int);
	bool setValue(double);
	bool setValue(QString);
	bool setValue(QDateTime);
	static quint64 getMangled(unsigned int val1, unsigned int val2);
	eveVariant abs();
	eveVariant operator + (const eveVariant&);
	eveVariant operator - (const eveVariant&);
	eveVariant operator * (const eveVariant&);
	bool operator > (const eveVariant&);
	bool operator >= (const eveVariant&);
	bool operator < (const eveVariant&);
	bool operator <= (const eveVariant&);

private:
	eveType varianttype;

};

#endif /* EVEVARIANT_H_ */
