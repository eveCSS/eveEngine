#ifndef EVEMANAGERTHREAD_H_
#define EVEMANAGERTHREAD_H_

#include <QThread>


class eveManagerThread : public QThread
{
public:
	eveManagerThread();
	virtual ~eveManagerThread();
    void run();
};

#endif /*EVEMANAGERTHREAD_H_*/
