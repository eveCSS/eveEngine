
#include "eveError.h"
#include "eveMessage.h"
#include <QWriteLocker>
#include <iostream>

eveError::eveError(QTextEdit * textDispl, int loglvl)
{
	loglevel = loglvl;
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
	if (debug <= loglevel){
		QWriteLocker locker(&lock);
		logQueue.append(string);
		emit newlogMessage();
	}
}

void eveError::printLogMessage()
{
	QWriteLocker locker(&lock);
	while (!logQueue.isEmpty()){
		if (textDisplay != 0){
			if (lineCount > 1000) {
				lineCount = 0;
				textDisplay->clear();
			}
			textDisplay->append(logQueue.takeFirst());
			++lineCount;
		}
		else{
			std::cout <<  "> " << qPrintable(logQueue.takeFirst()) << "\n";
		}
	}
}
