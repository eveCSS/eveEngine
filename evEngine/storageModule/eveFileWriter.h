/*
 * eveFileWriter.h
 *
 *  Created on: 10.09.2009
 *      Author: eden
 */

#ifndef EVEFILEWRITER_H_
#define EVEFILEWRITER_H_

#include <QHash>
#include <QString>
#include <QStringList>
#include "eveMessage.h"

/*
 *
 */
class eveFileWriter {
public:
	// eveFileWriter();
	virtual ~eveFileWriter() {};
	virtual int init(int, QString, QString, QHash<QString, QString>*) = 0;
	virtual int setCols(int, QString, QString, QStringList) = 0;
	virtual int open(int) = 0;
	virtual int addData(int, eveDataMessage*) = 0;
	virtual int addComment(int, QString)=0;
	virtual int close(int) = 0;
	virtual int setXMLData(QByteArray*) = 0;
	virtual QString errorText() = 0;
};

QT_BEGIN_NAMESPACE

Q_DECLARE_INTERFACE(eveFileWriter,"de.ptb.epics.eve.FileWriterInterface/1.0");

QT_END_NAMESPACE


#endif /* EVEFILEWRITER_H_ */
