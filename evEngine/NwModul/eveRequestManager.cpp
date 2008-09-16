#include "eveRequestManager.h"

eveRequestManager::eveRequestManager()
{
	lastId = 0;
}

eveRequestManager::~eveRequestManager()
{
}

int eveRequestManager::newId(int mChannelId)
{
	++lastId;
	reqHash.insert(lastId, mChannelId);
	return lastId;
}

int eveRequestManager::takeId(int rId)
{
	if (reqHash.contains(rId))
		return reqHash.take(rId);
	else
		return 0;
}
