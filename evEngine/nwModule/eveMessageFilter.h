#ifndef EVEMESSAGEFILTER_H_
#define EVEMESSAGEFILTER_H_

#define EVEMESSAGEFILTER_LOWLIMIT 60
#define EVEMESSAGEFILTER_HIGHLIMIT 250
#define EVEMESSAGEFILTER_TIMEOUT 1000	// msecs
#define EVEMESSAGEFILTER_QUEUELENGTH 20

#include <QTimer>
#include <QList>
#include "eveMessage.h"
// #include "eveNetObject.h"
class eveNetObject;

/**
 * \brief filter and cache messages before sending them to viewers
 *
 * MessageFilter is a filter and caching class to prevent
 * flooding the viewers with error messages.
 * It caches the last status and error messages to update
 * newly opened viewers.
 *
 */
class eveMessageFilter: public QObject
{
    Q_OBJECT

public:
	eveMessageFilter(eveNetObject *);
	virtual ~eveMessageFilter();

	bool checkMessage(eveMessage *);
	void queueMessage(eveMessage *);
	QList<eveMessage * > * getCache();
	void clearCache();

private slots:
	void timeout();

private:
	int skippedMessageCount;
	int messageCount;

	eveNetObject * netObject;
	QTimer *seqTimer;
	QList<eveMessage*> errorMessageCacheList;
	eveChainStatusMessage *chainStatusCache;
	eveEngineStatusMessage *engineStatusCache;
	evePlayListMessage *playlistCache;
	eveCurrentXmlMessage *currentXmlCache;
	eveErrorMessage *filenameCache;
    eveErrorMessage *maxPosCountCache;

};

#endif /*EVEMESSAGEFILTER_H_*/
