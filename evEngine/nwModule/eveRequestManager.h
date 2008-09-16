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
	
private:
	QHash<int, int> reqHash;
	int lastId;
};

#endif /*EVEREQUESTMANAGER_H_*/
