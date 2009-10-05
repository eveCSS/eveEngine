#ifndef EVEERROR_H_
#define EVEERROR_H_

#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QReadWriteLock>

class eveMessage;

class eveError: public QObject
{
    Q_OBJECT

public:
	eveError(QTextEdit *);
	virtual ~eveError();
	static void log(int, QString);
	void queueLog(int, QString);

	static eveError *errorOut;

private slots:
	void printLogMessage();

signals:
	void newlogMessage();

private:
	QTextEdit * textDisplay;
	QStringList logQueue;
	QReadWriteLock lock;
	int lineCount;
};

#endif /*EVEERROR_H_*/
