/*
 * evePlayListManager.h
 *
 *  Created on: 05.09.2008
 *      Author: eden
 */

#ifndef EVEPLAYLISTMANAGER_H_
#define EVEPLAYLISTMANAGER_H_

#define PLAYLISTMAXENTRIES 500
#define MAX_XML_LOADED 501

#include <QString>
#include <QHash>
#include <QByteArray>
#include "eveMessageChannel.h"

/**
 * \brief struct of playlist data for exchange with eveManager class
 *
 */
struct evePlayListData {
	QString name;
	QString author;
	QByteArray data;
};

/**
 * \brief internal struct of class evePlayListManager
 *
 */
struct eveDataEntry {
	bool isLoaded;
	QByteArray data;
	QString filename;
};

/**
 * \brief manages the playlist
 *
 * all playlist entries are sent to PlayListManager.
 * If the entry count exceeds MAX_XML_LOADED, Entries are stored to temporary files
 * and loaded back to memory before execution.
 */
class evePlayListManager : public QObject
{
	Q_OBJECT

public:
	evePlayListManager();
	virtual ~evePlayListManager();
	void addEntry(QString, QString, QByteArray);
	void reorderEntry(int, int);
	void removeEntry(int);
	bool isEmpty(){return playlist.isEmpty();};
	evePlayListData* takeFirst();
	evePlayListMessage* getCurrentPlayList();

private:
	int lastId;
	QList<evePlayListEntry> playlist;
	QHash<int, eveDataEntry*> datahash;

};

#endif /* EVEPLAYLISTMANAGER_H_ */
