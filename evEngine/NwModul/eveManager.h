/*
 * eveManager.h
 *
 *  Created on: 08.09.2008
 *      Author: eden
 */

#ifndef EVEMANAGER_H_
#define EVEMANAGER_H_

#include "eveMessageChannel.h"
#include "evePlayListManager.h"

/**
 * \brief creates and controls all scans, manages playlist stuff
 */
class eveManager: public eveMessageChannel
{
	Q_OBJECT

public:
	eveManager();
	virtual ~eveManager();
	void handleMessage(eveMessage *);

private:
	// eveMessageHub * mHub;
	evePlayListManager *playlist;
	bool autoStart;
};

#endif /* EVEMANAGER_H_ */
