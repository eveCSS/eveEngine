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
#include <QMultiHash>
#include <QFile>
#include "eveFileWriter.h"

class eveAsciiFileWriter: public eveFileWriter {
public:
	eveAsciiFileWriter();
	virtual ~eveAsciiFileWriter();

	int init(QString, QString, QHash<QString, QString>&);
//	int setCols(int, QString, QString, QStringList);
	int addColumn(eveDevInfoMessage* message);
	int open();
	int addData(int, eveDataMessage* );
	int addMetaData(int, QString, QString);
	int close();
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
	QMultiHash<QString, QString> metaData;
	QFile *filePtr;
	QStringList colList;
	QHash<QString, QString> lineHash;
};

#endif /* EVEASCIIFILEWRITER_H_ */
