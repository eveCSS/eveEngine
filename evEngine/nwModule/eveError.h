#ifndef EVEERROR_H_
#define EVEERROR_H_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QReadWriteLock>

class eveMessage;

class eveError: public QObject
{
    Q_OBJECT

public:
	eveError(QTextEdit *, int);
	virtual ~eveError();
	static void log(int, QString, int facility=0);
	void queueLog(unsigned int, int, QString);

	static eveError *errorOut;

public slots:
	void printLogMessage();

signals:
	void newlogMessage();

private:
	unsigned int loglevel;
	QTextEdit * textDisplay;
	QStringList logQueue;
	QReadWriteLock lock;
	int lineCount;
	char severityChars[10];
	QStringList facilityAbbr;
};

#endif /*EVEERROR_H_*/
