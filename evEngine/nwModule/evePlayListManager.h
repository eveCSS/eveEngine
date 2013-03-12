/*
 * evePlayListManager.h
 *
 *  Created on: 05.09.2008
 *      Author: eden
 */

#ifndef EVEPLAYLISTMANAGER_H_
#define EVEPLAYLISTMANAGER_H_

#define PLAYLISTMAXENTRIES 5000
#define MAX_XML_LOADED 2

#include <QString>
#include <QHash>
#include <QByteArray>
#include <QDir>
#include "eveMessageChannel.h"

/**
 * \brief struct of playlist data for exchange with eveManager class
 *
 */
struct evePlayListData {
    bool isLoaded;
    int pid;
    QString name;
    QString author;
    QByteArray data;
    QString filename;
};

/**
 * \brief internal struct of class evePlayListManager
 *
 */
//struct eveDataEntry {
//	bool isLoaded;
//	QByteArray data;
//	QString filename;
//};

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
    void removeCurrentEntry();
    bool isEmpty(){return playlist.isEmpty();};
    evePlayListData* takeFirst();
    evePlayListMessage* getCurrentPlayList();

private:
    bool modified;
    bool haveCurrent;
    int lastId;
    void flushPlaylist();
    evePlayListEntry currentPLE;
    QString currentFilename;
    QString getTempPath();
    QString dirFileName;
    QDir playlistPath;
    QList<evePlayListEntry> playlist;
    QHash<int, evePlayListData*> datahash;

};

#endif /* EVEPLAYLISTMANAGER_H_ */
