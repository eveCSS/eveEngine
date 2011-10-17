
#include <stdio.h>

#include <QtGui>
#include <QApplication>


#include <QMainWindow>
#include <QWidget>
#include <QGridLayout>
#include <QTextEdit>
#include <QAction>
#include <QMenu>
#include <QString>
#include <QStringList>
#include <QProcess>

#include "eveMessageHub.h"
#include "eveError.h"
#include "eveParameter.h"

// loglevel 0: send no error messages
// loglevel 1: send FATAL errors
// loglevel 2: send FATAL,ERROR errors
// loglevel 3: send FATAL,ERROR,MINOR errors (default)
// loglevel 4: send FATAL,ERROR,MINOR,INFO errors
// loglevel 5: send FATAL,ERROR,MINOR,INFO,DEBUG errors
#define DEFAULT_LOGLEVEL 3

int main(int argc, char *argv[])
{
	bool useGui = false;
	bool useNet = true;
	QString xmlFileName;
	QString logFileName;
	QString interfaces("all");
	QString eveRoot;
	int loglevel=-1;
	int debuglevel=DEFAULT_LOGLEVEL;
	int portNumber = 12345;
	bool skipOne = false;
	QTextEdit *textDisplay = 0;
	QMainWindow *mainWin = 0;
	QAction *exitAct =0;

	// poor mans argument parsing
	for ( int i = 1; i < argc; i++ ) {
		if (skipOne){
			skipOne = false;
			continue;
		}
		bool ok;
		QString argument = QString(argv[i]);
		QString argumentNext;
		QString parameter;
		if (i+1 < argc) argumentNext = QString(argv[i+1]);

		if ( argument.startsWith("-g" )){
			useGui = true;
			continue;
		}
		else if ( argument.startsWith("-n" )){
			useNet = false;
			continue;
		}
		else if ( argument.startsWith("-d") ){
			parameter = argument.remove(0,2).trimmed();
			if (parameter.isEmpty()) {
				parameter = argumentNext.trimmed();
				skipOne = true;
			}
			debuglevel = parameter.toInt(&ok);
			if (debuglevel < 0) debuglevel = 0 ;
			if (ok) continue;
		}
		else if ( argument.startsWith("-m") ){
			parameter = argument.remove(0,2).trimmed();
			if (parameter.isEmpty()) {
				parameter = argumentNext.trimmed();
				skipOne = true;
				loglevel = DEFAULT_LOGLEVEL+1;
			}
			loglevel = parameter.toInt(&ok);
			if (ok) continue;
		}
		else if ( argument.startsWith("-p") ){
			parameter = argument.remove(0,2).trimmed();
			if (parameter.isEmpty()) {
				parameter = argumentNext.trimmed();
				skipOne = true;
			}
			portNumber = parameter.toInt(&ok);
			if (ok) continue;
		}
		else if ( argument.startsWith("-f") ){
			xmlFileName = argument.remove(0,2).trimmed();
			if (xmlFileName.isEmpty()) {
				xmlFileName = argumentNext.trimmed();
				skipOne = true;
			}
			if (!xmlFileName.isEmpty()) continue;
		}
		else if ( argument.startsWith("-l") ){
			logFileName = argument.remove(0,2).trimmed();
			if (logFileName.isEmpty()) {
				logFileName = argumentNext.trimmed();
				skipOne = true;
			}
			if (!logFileName.isEmpty()) continue;
		}
		else if ( argument.startsWith("-e") ){
			eveRoot = argument.remove(0,2).trimmed();
			if (eveRoot.isEmpty()) {
				eveRoot = argumentNext.trimmed();
				skipOne = true;
			}
			if (!eveRoot.isEmpty()) continue;
		}
//		else if ( argument.startsWith("-i") ){
//			interfaces = argument.remove(0,2).trimmed();
//			if (interfaces.size() > 0) continue;
//		}

		printf ("\"%s\"?\n", argv[i]);
	    printf ("usage: %s [-d<number> => debuglevel, -m<number> => messagelevel, -f<xml-File>, -g => Gui on (default off), -n => no network, -p<port>, -e<root directory>\n",argv[0]);
	    return (1);
    }

	QApplication app(argc, argv, useGui);

	if (useGui) {
		mainWin = new QMainWindow();
		QWidget *myCenWid = new QWidget(mainWin);
		mainWin->setCentralWidget(myCenWid);

		textDisplay = new QTextEdit(myCenWid);
		QGridLayout *mgLayout = new QGridLayout(myCenWid);
		mgLayout->addWidget(textDisplay, 1, 0);

		mainWin->setWindowTitle("Debug Window");

		exitAct = new QAction("E&xit", mainWin);
		QMenu *fileMenu = mainWin->menuBar()->addMenu("&File");
		mainWin->menuBar()->addSeparator();
		fileMenu->addAction(exitAct);

		mainWin->resize(600, 800);
		mainWin->show();
	}

	// check if environment variable EVE_ROOT is set
	if (eveRoot.isEmpty()) {
		QStringList envList = QProcess::systemEnvironment();
		foreach (QString env, envList){
			if (env.startsWith("EVE_ROOT=")){
				eveRoot = env.remove("EVE_ROOT=");
				break;
			}
		}
	}

	// build parameter list
	eveParameter *paralist = new eveParameter();
	if (useNet)
		paralist->setParameter("use_network", "yes");
	else
		paralist->setParameter("use_network", "no");

	if ((loglevel < 0) || (loglevel > 5 )) loglevel = DEFAULT_LOGLEVEL;
	paralist->setParameter("loglevel",QString("%1").arg(loglevel));

	paralist->setParameter("port", QString().setNum(portNumber));
	paralist->setParameter("interfaces", interfaces);
	if (!eveRoot.isEmpty()) paralist->setParameter("eveRoot", eveRoot);
	if (!xmlFileName.isEmpty()) paralist->setParameter("startFile",xmlFileName );

	eveError *error = new eveError(textDisplay, debuglevel);
	eveMessageHub *mHub = new eveMessageHub(useGui, useNet);

	if (useGui) {
		mainWin->connect(exitAct, SIGNAL(triggered()), mHub, SLOT(close()), Qt::QueuedConnection);
		mainWin->connect(mHub, SIGNAL(closeParent()), mainWin, SLOT(close()), Qt::QueuedConnection);
	}
	mHub->init();

    int retval = app.exec();
    // make sure all has been logged
    if (!useGui) error->printLogMessage();

    exit(retval);
}
