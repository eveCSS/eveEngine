#ifndef EVEREQUESTMANAGER_H_
#define EVEREQUESTMANAGER_H_

#include <QHash>

class eveRequestManager
{
public:
	eveRequestManager();
	virtual ~eveRequestManager();
	int newId(int);
	int takeId(int);
	static eveRequestManager* getRequestManager(){return reqMan;};

private:
	QHash<int, int> reqHash;
	int lastId;
	static eveRequestManager* reqMan;
};

#endif /*EVEREQUESTMANAGER_H_*/
