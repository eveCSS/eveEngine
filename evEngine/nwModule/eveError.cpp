
#include "eveError.h"
#include "eveMessage.h"
#include <QWriteLocker>
#include <iostream>

eveError::eveError(QTextEdit * textDispl, int loglvl)
{
	strcpy(severityChars, "0FEWID");
	loglevel = loglvl;
	if (loglevel+1 > strlen(severityChars)) loglevel = 5;
	errorOut = this;
	textDisplay = textDispl;
	facilityAbbr << "NONE " << "MFILT" << "CPARS" << "NETWK" << "MHUB "
			<< "PLAYL" << "MANAG" << "XMLPR" << "SCANC" << "POSCA" << "SMDEV"
			<< "CATRA" << "SCANM" << "STORG" << "EVENT" << "LOTIM" << "MATH";
	connect (this, SIGNAL(newlogMessage()), this, SLOT(printLogMessage()), Qt::QueuedConnection);
}

eveError::~eveError()
{
}

eveError* eveError::errorOut = 0;

void eveError::log(int debug, QString string, int facility)
{
	if (errorOut != 0) errorOut->queueLog(debug, facility, string);
}

void eveError::queueLog(unsigned int debug, int facility, QString string)
{
	if (debug <= loglevel){
		int stringIndex= facility-8;
		if ((stringIndex < 0) || (stringIndex >= facilityAbbr.size())) stringIndex=0;
		QString logText = QString("%1-%2: %3").arg(severityChars[debug]).arg(facilityAbbr.at(stringIndex)).arg(string);
		QWriteLocker locker(&lock);
		logQueue.append(logText);
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
			std::cout << qPrintable(logQueue.takeFirst()) << "\n";
		}
	}
}
