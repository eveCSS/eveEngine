#ifndef EVESTORAGETHREAD_H_
#define EVESTORAGETHREAD_H_

#include <QThread>
#include <QString>

class eveStorageThread : public QThread
{
public:
	eveStorageThread(QString);
	virtual ~eveStorageThread();
    void run();

private:
	fileName;
};

#endif /*EVESTORAGETHREAD_H_*/
