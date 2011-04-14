/*
 * eveAsciiFileWriter.h
 *
 *  Created on: 10.09.2009
 *      Author: eden
 */

#ifndef EVEASCIIFILEWRITER_H_
#define EVEASCIIFILEWRITER_H_

/*
 *
 */
#include <QHash>
#include <QFile>
#include "eveFileWriter.h"

class eveAsciiFileWriter: public eveFileWriter {
public:
	eveAsciiFileWriter();
	virtual ~eveAsciiFileWriter();

	int init(int, QString, QString, QHash<QString, QString>*);
	int setCols(int, QString, QString, QStringList);
	int open(int);
	int addData(int, eveDataMessage* );
	int addComment(int, QString);
	int close(int);
	int setXMLData(QByteArray*);
	QString errorText() {return errorString;};

private:
	class columnInfo {
		public:
		columnInfo(QString, QString, QStringList);
		QString id;
		QString name;
		QStringList info;
	};
	void nextPosition();
	QHash<QString, columnInfo* > colHash;
	QString errorString;
	int setId;
	int currentIndex;
	bool initDone;
	bool fileOpen;
	QString fileName;
	QString fileFormat;
	QString comment;
	QFile *filePtr;
	QStringList colList;
	QHash<QString, QString> lineHash;
};

#endif /* EVEASCIIFILEWRITER_H_ */
