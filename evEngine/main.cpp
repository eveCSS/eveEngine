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
    QApplication app(argc, argv);

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
	paralist->setParameter("use_network", "yes");
	paralist->setParameter("port", "12345");
	paralist->setParameter("interfaces", "all");


	eveError *error = new eveError(textDisplay);
	eveMessageHub *mHub = new eveMessageHub();
	mHub->init();
	mainWin->connect(exitAct, SIGNAL(triggered()), mHub, SLOT(close()));
	mainWin->connect(mHub, SIGNAL(closeParent()), mainWin, SLOT(close()));

    return app.exec();
}
