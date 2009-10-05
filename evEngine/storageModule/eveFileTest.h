/*
 * eveFileTest.h
 *
 *  Created on: 18.09.2009
 *      Author: eden
 */

#ifndef EVEFILETEST_H_
#define EVEFILETEST_H_

#include <QString>

/*
 *
 */
class eveFileTest {
public:
	eveFileTest();
	virtual ~eveFileTest();
	static QString addNumber(QString);
	static int createTestfile(QString);
	static bool isWriteable(QString);
};

#endif /* EVEFILETEST_H_ */
