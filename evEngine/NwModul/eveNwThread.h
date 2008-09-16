#ifndef EVENWTHREAD_H_
#define EVENWTHREAD_H_

#include <QThread>


class eveNwThread : public QThread
{
public:
	eveNwThread();
	virtual ~eveNwThread();
    void run();
};

#endif /*EVENWTHREAD_H_*/
