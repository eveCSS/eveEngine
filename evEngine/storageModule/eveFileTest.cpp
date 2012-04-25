/*
 * eveFileTest.cpp
 *
 *  Created on: 18.09.2009
 *      Author: eden
 */

#include "eveFileTest.h"
#include <QDir>
#include <QChar>
#include <QRegExp>
#include <QStringList>


eveFileTest::eveFileTest() {
	// TODO Auto-generated constructor stub

}

eveFileTest::~eveFileTest() {
	// TODO Auto-generated destructor stub
}

/**
 * Replace a name.txt with name0001.txt, unless the filename already has a number.
 * If the new file already exists, then increment the highest existing number.
 * If no more free numbers are left, append another 5 digit block
 *
 * @param filename
 * @return new filename with the next 5 char number appended or replaced
 * 			which does not exist in the current directory
 */
QString eveFileTest::addNumber(QString filename) {

	QFileInfo info(filename);
	QDir directory(info.absolutePath());
	QString base(info.completeBaseName());
	QString suffix(info.suffix());
	QRegExp rx("[0-9]{5,5}$");
	QRegExp testrx("[0-9]*$");
	QString newName;
	QString testbase(base);
	QString originalBase(base);
	bool ok;

	base.remove(rx);
	testbase.remove(testrx);

	// if filename has more than 5 trailing digits append "_xxxxx"
	if (base.length() != testbase.length()){
		base = info.completeBaseName().append("_");
		originalBase = base;
	}
	// prepend a "." to suffix if we have a suffix or if we have a trailing "."
	if ((suffix.length() > 0) || filename.endsWith(".")) suffix.prepend(".");

	QStringList namelist = directory.entryList(QStringList(QString("%1[0-9][0-9][0-9][0-9][0-9]%2").arg(base).arg(suffix)), QDir::Files, QDir::Name);
	int index = 1;
	while (namelist.count() > 0){
		QFileInfo newInfo(namelist.last());
		index = newInfo.completeBaseName().right(5).toInt(&ok);
		if (index != 99999) break;
		base = base.append("99999_");
		originalBase = base;
		namelist = directory.entryList(QStringList(QString("%1[0-9][0-9][0-9][0-9][0-9]%2").arg(base).arg(suffix)), QDir::Files, QDir::Name);
	}

	if (namelist.count() > 0){
		QFileInfo newInfo(namelist.last());
		index = newInfo.completeBaseName().right(5).toInt(&ok);
		if (index == 99999) {
			index = 0;
			base.append("99999_");
		}
		++index;
		newName = QString("/%1%2%3").arg(base).arg(index,5,10,QChar('0')).arg(suffix);
	}
	else {
		QRegExp nmbr(".*[0-9]{5,5}$");
		if (nmbr.exactMatch(originalBase))
			newName = QString("/%1%2").arg(originalBase).arg(suffix);
		else
			newName = QString("/%0100001%2").arg(base).arg(suffix);
	}
	return newName.prepend(info.absolutePath());
}
/**
 *
 * @param filename
 * @return 	0: success
 * 			1: error: file already exists
 * 			2: error: directory does not exist
 * 			3: error: unable to open file in write mode
 * 			4: error: unable to delete test file
 */
int eveFileTest::createTestfile(QString filename) {

	QFileInfo info(filename);

	if (info.exists()) return 1;
	if (!info.dir().exists()) return 2;

	QFile testFile(filename);
	if (!testFile.open(QIODevice::WriteOnly)) return 3;
	testFile.close();

	if (!testFile.remove()) return 4;

	return 0;
}

