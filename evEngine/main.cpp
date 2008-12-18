#include <QtGui>
#include <QApplication>

#include <QMainWindow>
#include <QWidget>
#include <QGridLayout>
#include <QTextEdit>
#include <QAction>
#include <QMenu>
#include <QString>

#include "eveMessageHub.h"
#include "eveError.h"
#include "eveParameter.h"

int main(int argc, char *argv[])
{
	bool useGui = true;
	bool useNet = true;
	char xmlFileName[151];
	char logFileName[151];
	char interfaces[151] = "all";
	int loglevel;
	int portNumber = 12345;

	// poor mans argument parsing
	for ( int i = 1; i < argc; i++ ) {
		if ( QString(argv[i]) == "-g" )
			useGui = true;
		else if ( sscanf ( argv[i],"-d %u", &loglevel ) == 1 )
			continue;
		else if ( sscanf ( argv[i],"-p %u", &portNumber ) == 1 )
			continue;
		else if ( QString(argv[i]) == "-n" )
			useNet=false;
		else if ( sscanf ( argv[i], "-f %150s", xmlFileName ) == 1 )
        	continue;
		else if ( sscanf ( argv[i], "-i %150s", interfaces ) == 1 )
        	continue;
		else if ( sscanf ( argv[i], "-l %150s", logFileName ) == 1 )
			continue;
		else {
			printf ("\"%s\"?\n", argv[i]);
	        printf ("usage: %s [-d<debug level> => loglevel, -f<xml-File>, -g => Gui on (default), -n => no network, -p<port>, -l<interface> => default all\n",argv[0]);
	        return (1);
		}
    }

	QApplication app(argc, argv, useGui);

    QMainWindow *mainWin = new QMainWindow();
	QWidget *myCenWid = new QWidget(mainWin);
	mainWin->setCentralWidget(myCenWid);

	QTextEdit *textDisplay = new QTextEdit(myCenWid);
	QGridLayout *mgLayout = new QGridLayout(myCenWid);
	mgLayout->addWidget(textDisplay, 1, 0);

	mainWin->setWindowTitle("Test");

	QAction *exitAct = new QAction("E&xit", mainWin);
	QMenu *fileMenu = mainWin->menuBar()->addMenu("&File");
	mainWin->menuBar()->addSeparator();
	fileMenu->addAction(exitAct);


	mainWin->resize(600, 800);
	mainWin->show();

	// build parameter list
	eveParameter *paralist = new eveParameter();
	if (useNet)
		paralist->setParameter("use_network", "yes");
	else
		paralist->setParameter("use_network", "no");

	paralist->setParameter("port", QString().setNum(portNumber));
	paralist->setParameter("interfaces", QString(interfaces));


	eveError *error = new eveError(textDisplay);
	eveMessageHub *mHub = new eveMessageHub();
	mHub->init();
	mainWin->connect(exitAct, SIGNAL(triggered()), mHub, SLOT(close()), Qt::QueuedConnection);
	mainWin->connect(mHub, SIGNAL(closeParent()), mainWin, SLOT(close()), Qt::QueuedConnection);

    return app.exec();
}
