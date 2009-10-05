#ifndef EVEMANAGERTHREAD_H_
#define EVEMANAGERTHREAD_H_

#include <QThread>
#include <QMutex>
#include <QWaitCondition>


class eveManagerThread : public QThread
{
public:
	eveManagerThread(QWaitCondition*, QMutex *);
	virtual ~eveManagerThread();
    void run();

private:
	QMutex *waitMutex;
	QWaitCondition* channelRegistered;
};

#endif /*EVEMANAGERTHREAD_H_*/
