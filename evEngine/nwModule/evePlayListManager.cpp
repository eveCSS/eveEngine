/*
 * evePlayListManager.cpp
 *
 *  Created on: 05.09.2008
 *      Author: eden
 */

#include <QProcess>
#include <QUuid>
#include <QList>
#include <QFileInfoList>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QBuffer>
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#include <QUrl>
#include "evePlayListManager.h"
#include "eveManager.h"
#include "eveParameter.h"
#include "eveError.h"
#include "evevalidationhandler.h"


evePlayListManager::evePlayListManager(eveManager* pmanager) {
    lastId = 0;
    modified = false;
    haveCurrent = false;
    manager = pmanager;
    playlistPath = QDir(getTempPath());
    dirFileName = playlistPath.absoluteFilePath("eveCacheDir.db");
    if (!playlistPath.exists()){
        eveError::log(DEBUG, QString("evePlayListManager: create cache dir %1").arg(playlistPath.absolutePath()));
        if (!playlistPath.mkpath(playlistPath.absolutePath()))
            eveError::log(ERROR, QString("evePlayListManager: unable to make temp path: %1").arg(playlistPath.absolutePath()));
    }
    else {
        // read a cache file if left over from last run
        QFile file(dirFileName);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                ++lastId;
                evePlayListEntry ple;
                QString line = in.readLine();
                line.chop(1);
                ple.name = line.mid(1).section("\";\"",0,0);
                ple.author = line.section("\";\"",1,1);
                ple.pid = lastId;
                QString scmlfilename = line.section("\";\"",2,2);
                if (QFileInfo(scmlfilename).exists()){
                    evePlayListData* pld = new evePlayListData();
                    pld->name = ple.name;
                    pld->author = ple.author;
                    pld->isLoaded = false;
                    pld->filename = scmlfilename;
                    datahash.insert(lastId, pld);
                    playlist.append(ple);
                    modified = true;
                    eveError::log(DEBUG, QString("evePlayListManager: add name %1, author %2, filename >%3<").arg(pld->name).arg(pld->author).arg(pld->filename));
                }
            }
            file.close();
        }
    }
}

evePlayListManager::~evePlayListManager() {

    if (haveCurrent){
        QFile file(currentFilename);
        if (file.exists()) file.remove();
    }
    foreach(evePlayListData* pld, datahash){
        QFile file(pld->filename);
        if (file.exists()) file.remove();
    }
    playlist.clear();
    flushPlaylist();
}

/**
 * \brief add a playlist entry
 * \param name	name of XML-Description
 * \param author author@host
 * \param data XML-Text
 *
 */
void evePlayListManager::addEntry(QString name, QString author, QByteArray data) {

    evePlayListEntry ple;
    evePlayListData* pld;
    bool saved = false;

    if (playlist.count() > PLAYLISTMAXENTRIES) {
        eveError::log(ERROR, "evePlayListManager::addEntry: Too many playlist entries");
        return;
    }
    if (data.length() < 10){
        eveError::log(ERROR, "evePlayListManager::addEntry: XML-Data is empty");
        return;
    }

    if (lastId > 0x7FFFFFFF)
        lastId=1;
    else
        ++lastId;

    ple.pid = lastId;
    ple.name = name;
    ple.author = author;
    if (ple.name.length() < 1) ple.name = "none";
    if (ple.author.length() < 1) ple.author = "none";

    eveError::log(ERROR, QString("evePlayListManager: got entry id: %1 name: %2").arg(ple.pid).arg(ple.name));

    // Caution XML verification uses QNetworkManager and is asynchronous
    // works only if this and all parent functions are reentrant!
    if (!xmlPassedVerification(data)) {
        sendError(ERROR, 0, QString("SCML Validation unsuccessful, Skip entry %1").arg(name));
        return;
    }

    // always save to temporary file
    QString filename = QUuid::createUuid().toString();
    filename = playlistPath.absolutePath() + "/" + filename.mid(1, filename.length()-2);
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream out(&file);
        out << data;
        file.close();
        saved = true;
    }
    else {
        eveError::log(ERROR, QString("evePlayListManager: unable to open temporary file: %1").arg(filename));
    }

    // fill dataentry
    pld = new evePlayListData;
    pld->filename = filename;
    pld->author = author;
    pld->name = name;
    if (saved && (playlist.count() > MAX_XML_LOADED)) {
        pld->isLoaded = false;
    }
    else {
        pld->isLoaded = true;
        pld->data = data;
    }
    insertEntry(ple, pld);
    flushPlaylist();

    return;
}
/**
 * \brief add playlist entry
 * \param ple playlistentry to insert
 * \param pld playlistdata to insert
 */
void evePlayListManager::insertEntry(evePlayListEntry ple, evePlayListData* pld){

    eveError::log(ERROR, QString("evePlayListManager: insert entry id: %1 name: %2").arg(ple.pid).arg(ple.name));

//    QListIterator<evePlayListEntry> itera(playlist);
    bool insertDone=false;
    QList<evePlayListEntry>::iterator itera;
    for (itera = playlist.begin(); itera != playlist.end(); ++itera) {
        if (itera->pid < ple.pid) continue;
        playlist.insert(itera, ple);
        insertDone = true;
        break;
    }
//    for (int i = playlist.size()-1; i >=0 ; --i) {
//        eveError::log(ERROR, QString("evePlayListManager: loop count: %1 entry id: %2 (%3)").arg(i).arg(ple.pid).arg(playlist.size()));
//        if (playlist.at(i).pid > ple.pid) continue;
//        playlist.insert(i, ple);
//        insertDone = true;
//        break;
//    }
    if (!insertDone) playlist.append(ple);
    datahash.insert(ple.pid, pld);


}


/**
 * \brief remove playlist entry
 * \param id of the entry to be removed
 */
void evePlayListManager::removeEntry(int id){

    for (int i = 0; i < playlist.size(); ++i) {
        if (playlist.at(i).pid == id) {
            playlist.removeAt(i);
            modified = true;
            break;
        }
    }
    if (datahash.contains(id)){
        evePlayListData* entry = datahash.take(id);
        QFile tmpfile(entry->filename);
        if (tmpfile.exists() && entry->filename.startsWith(playlistPath.absolutePath()))
            tmpfile.remove();
        else
            eveError::log(ERROR, QString("evePlayListManager: cannot find file: %1").arg(entry->filename));

        delete entry;
        if (modified) flushPlaylist();
    }
}

/**
 * \brief reorder the playlist entries
 * \param id the entry to be moved
 * \param steps positive/negative count of steps to move entry down/up
 */
void evePlayListManager::reorderEntry(int id, int steps){

    if (steps == 0) return;

    for (int i = 0; i < playlist.size(); ++i) {
        if (playlist.at(i).pid == id) {
            int pos = steps + i;
            if (pos > playlist.size()) pos = playlist.size();
            else if (pos < 0) pos = 0;
            evePlayListEntry entry = playlist.takeAt(i);
            playlist.insert(pos,entry);
            modified = true;
            break;
        }
    }
    if (modified) flushPlaylist();
    return;
}

/**
 * \brief returns the next playlist entry and deletes it from the list
 * \return pointer to next playlist entry or NULL if playlist is empty
 */
evePlayListData* evePlayListManager::takeFirst(){

    if (playlist.size() == 0) return NULL;

    // flush before taking the next entry, so the current entry is still on disk
    removeCurrentEntry();

    currentPLE = playlist.takeFirst();
    evePlayListData *pld;

    if (datahash.contains(currentPLE.pid)){
        pld = datahash.take(currentPLE.pid);
    }
    else {
        eveError::log(ERROR, "evePlayListManager::takeFirst: List inconsistency no data found");
        return NULL;
    }
    currentFilename = pld->filename;
    haveCurrent=true;

    if (!pld->isLoaded) {
        QFile file(pld->filename);
        if (file.open(QIODevice::ReadOnly)) {
            pld->data.clear();
            pld->data = file.readAll();
            file.close();
            pld->isLoaded = true;
        }
        else {
            eveError::log(ERROR, QString("evePlayListManager: cannot find file: %1").arg(pld->filename));
            return NULL;
        }
    }
    return pld;
}

/**
 * \brief remove current entry from cache file
 */
void evePlayListManager::removeCurrentEntry(){

    if (haveCurrent){
        haveCurrent = false;
        QFile file(currentFilename);
        if (file.exists()) file.remove();
        flushPlaylist();
    }
    return;
}

/**
 * \brief returns a pointer to a message with the current playlist display data
 * \return pointer to evePlayListMessage
 */
evePlayListMessage* evePlayListManager::getCurrentPlayList(){
    return new evePlayListMessage(playlist);
}

/**
 * \brief find the directory path to store playlist entries temporarily
 * \return string with path
 */
QString evePlayListManager::getTempPath(){

    QString user = "unknown";
    QString tmpdir = "/tmp";
    bool usernamefound=false;
    bool tmpdirfound=false;

    QStringList environment = QProcess::systemEnvironment();
    foreach (QString line, environment){
        if (line.startsWith("USER=")) {
            user=line.remove(0,5);
            usernamefound = true;
        }
        if (line.startsWith("TMPDIR=")) {
            tmpdir=line.remove(0,7);
            tmpdirfound = true;
        }
    }
    foreach (QString line, environment){
        if (!usernamefound && line.startsWith("USERNAME=")) user=line.remove(0,9);
        if (!tmpdirfound && line.startsWith("TMP=")) tmpdir=line.remove(0,4);
        if (!tmpdirfound && line.startsWith("TEMP=")) tmpdir=line.remove(0,5);
    }

    QFileInfo path = QFileInfo(tmpdir + "/eve-" + user);
    return path.absoluteFilePath();
}

/**
 * \brief write current playlist to cache file
 */
void evePlayListManager::flushPlaylist(){

    QFile file(dirFileName);
    int playListSize = playlist.size();

    if (file.exists()) file.remove();
    if (haveCurrent) ++playListSize;
    if ((playListSize > 0) && file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (haveCurrent)
            out << "\"" << currentPLE.name << "\";\"" << currentPLE.author << "\";\"" << currentFilename << "\"\n";
        foreach (evePlayListEntry ple, playlist){
            if (datahash.contains(ple.pid))
                out << "\"" << ple.name << "\";\"" << ple.author << "\";\"" << datahash.value(ple.pid)->filename << "\"\n";
        }
        file.close();
    }
    modified = false;
    return;
}

bool evePlayListManager::xmlPassedVerification(QByteArray &xmldata) {

    QString schemaFile = QString("scml-%1.xsd").arg(eveParameter::getParameter("xmlparserversion"));

    QString schemaPath = eveParameter::getParameter("schemapath");
    QFileInfo schemaPathInfo = QFileInfo(schemaPath);
    // note: the "/" will be translated to "\" on windows
    if (!schemaPathInfo.completeBaseName().isEmpty()) schemaPath += "/";
    QFileInfo schemaFileInfo(schemaPath + schemaFile);

    if (!schemaFileInfo.exists()){
        // warn and continue without validation
        sendError(MINOR, 0, QString("Unable to find schema file: %1. Resuming without validation!").arg(schemaFileInfo.absoluteFilePath()));
        return true;
    }

    QXmlSchema schema;
    QFile file(schemaFileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)){
        // warn and continue without validation
        sendError(MINOR, 0, QString("Unable to open schema file: %1. Resuming without validation!").arg(schemaFileInfo.absoluteFilePath()));
        return true;
    }

    schema.load(&file, QUrl::fromLocalFile(schemaFileInfo.absoluteFilePath()));

    bool retval = false;
    if (schema.isValid()) {
        QBuffer buffer(&xmldata);
        buffer.open(QIODevice::ReadOnly);
        QXmlSchemaValidator validator(schema);
        eveValidationHandler* validationHandler = new eveValidationHandler(manager);
        validator.setMessageHandler (validationHandler);
        if (validator.validate(&buffer)) retval = true;
        delete validationHandler;
    }
    else
        sendError(ERROR, 0, QString("invalid schema file: %1").arg(schemaFileInfo.absoluteFilePath()));

    return retval;
}

void evePlayListManager::sendError(int severity, int errorType,  QString errorString){
    manager->sendError(severity, EVEMESSAGEFACILITY_PLAYLIST, errorType, errorString);
}
