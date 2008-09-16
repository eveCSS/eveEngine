
#include "eveError.h"
#include "eveMessage.h"
#include <QWriteLocker>

eveError::eveError(QTextEdit * textDispl)
{
	errorOut = this;
	textDisplay = textDispl;
	connect (this, SIGNAL(newlogMessage()), this, SLOT(printLogMessage()), Qt::QueuedConnection);
}

eveError::~eveError()
{
}

eveError* eveError::errorOut = 0;

void eveError::log(int debug, QString string)
{
	if (errorOut != 0) errorOut->queueLog(debug, string);
}

void eveError::queueLog(int debug, QString string)
{
	QWriteLocker locker(&lock);
	logQueue.append(string);
	emit newlogMessage();
}

void eveError::printLogMessage()
{
	QWriteLocker locker(&lock);
	while (!logQueue.isEmpty())textDisplay->append(logQueue.takeFirst());
}
