/*
 * eveVariant.h
 *
 *  Created on: 20.01.2009
 *      Author: eden
 */

#ifndef EVEVARIANT_H_
#define EVEVARIANT_H_

#include <QVariant>
#include <QString>
#include "eveTypes.h"

class eveVariant : public QVariant {
public:
	eveVariant();
	eveVariant(int);
	eveVariant(double);
	eveVariant(QString);
	virtual ~eveVariant();
	void setType(eveType);
	eveType getType(){return varianttype;};
	bool setValue(int);
	bool setValue(double);
	bool setValue(QString);
	eveVariant operator + (const eveVariant&);
	eveVariant operator * (const eveVariant&);
	bool operator > (const eveVariant&);
	bool operator >= (const eveVariant&);
	bool operator < (const eveVariant&);
	bool operator <= (const eveVariant&);

private:
	eveType varianttype;

};

#endif /* EVEVARIANT_H_ */
