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
#include "eveParameter.h"

class eveAsciiFileWriter: public eveFileWriter {
public:
	eveAsciiFileWriter();
	virtual ~eveAsciiFileWriter();

	int init(QString, QString, QHash<QString, QString>&);
	// this is not a real plugin
	QString getVersionString(){return eveParameter::getParameter("savepluginversion");};
	int addColumn(eveDevInfoMessage* message);
	int open();
	int addData(int, eveDataMessage* );
	int addMetaData(int, QString, QString);
	int close();
    int flush();
	int setXMLData(QByteArray*);
	QString errorText() {return errorString;};

private:
	QString errorString;
	bool initDone;
	bool fileOpen;
	QString fileName;
	QString fileFormat;
	QFile *filePtr;
};

#endif /* EVEASCIIFILEWRITER_H_ */
