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
    unsigned int pid;
    QString name;
    QString author;
    QByteArray data;
    QString filename;
};

class eveManager;

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
    evePlayListManager(eveManager*);
    virtual ~evePlayListManager();
    void addEntry(QString, QString, QByteArray);
    void reorderEntry(int, int);
    void removeEntry(int);
    void removeCurrentEntry();
    bool isEmpty(){return playlist.isEmpty();};
    evePlayListData* takeFirst();
    evePlayListMessage* getCurrentPlayList();

private:
    eveManager* manager;
    bool modified;
    bool haveCurrent;
    unsigned int lastId;
    void flushPlaylist();
    evePlayListEntry currentPLE;
    QString currentFilename;
    QString getTempPath();
    QString dirFileName;
    QDir playlistPath;
    QList<evePlayListEntry> playlist;
    QHash<unsigned int, evePlayListData*> datahash;
    void sendError(int, int, QString);
    bool xmlPassedVerification(QByteArray & );
    void insertEntry(evePlayListEntry, evePlayListData*);

};

#endif /* EVEPLAYLISTMANAGER_H_ */
