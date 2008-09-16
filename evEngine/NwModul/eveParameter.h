/*
 * eveParameter.h
 *
 *  Created on: 15.09.2008
 *      Author: eden
 */

#ifndef EVEPARAMETER_H_
#define EVEPARAMETER_H_

#include <QString>
#include <QHash>

/**
 *  \brief holds a list with all startup parameters
 *
 */
class eveParameter {
public:
	eveParameter();
	virtual ~eveParameter();
	static QString getParameter(QString key){return paraHash.value(key);};
	void setParameter(QString key, QString value){paraHash.insert(key, value);};

private:
	static QHash<QString, QString> paraHash;

};

#endif /* EVEPARAMETER_H_ */
