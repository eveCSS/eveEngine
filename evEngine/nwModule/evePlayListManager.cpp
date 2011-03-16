/*
 * evePlayListManager.cpp
 *
 *  Created on: 05.09.2008
 *      Author: eden
 */

#include "evePlayListManager.h"
#include "eveError.h"

evePlayListManager::evePlayListManager() {
	lastId = 0;
}

evePlayListManager::~evePlayListManager() {
	// Auto-generated destructor stub
}

/**
 * \brief add a playlist entry
 * \param name	name of XML-Description
 * \param author author@host
 * \param data XML-Text
 *
 */
void evePlayListManager::addEntry(QString name, QString author, QByteArray data) {

	evePlayListEntry entry;
	eveDataEntry * dataentry;

	if (playlist.count() > PLAYLISTMAXENTRIES) {
		eveError::log(4, "evePlayListManager::addEntry: Too many playlist entries");
		return;
	}
	if (lastId > 2000000000)
		lastId=0;
	else
		++lastId;

	if (data.length() < 10){
		eveError::log(4, "evePlayListManager::addEntry: XML-Data is empty");
		return;
	}

	entry.pid = lastId;
	entry.name = QString(name);
	entry.author = QString(author);
	if (entry.name.length() < 1) entry.name = "none";
	if (entry.author.length() < 1) entry.author = "none";

	dataentry = new eveDataEntry;

	if (playlist.count() > MAX_XML_LOADED) {
		//store xml to a temporary file
		// TODO
	}
	else {
		dataentry->isLoaded = 1;
		dataentry->data = QByteArray(data);
		dataentry->filename = "";
	}
	datahash.insert(entry.pid, dataentry);
	playlist.append(entry);

	return;
}

/**
 * \brief remove playlist entry
 * \param id the entry to be removed
 */
void evePlayListManager::removeEntry(int id){

	for (int i = 0; i < playlist.size(); ++i) {
		if (playlist.at(i).pid == id) {
			playlist.removeAt(i);
			break;
		}
	}
	datahash.remove(id);
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
			break;
		}
	}
	return;
}

/**
 * \brief returns the next playlist entry and deletes it from the list
 * \return pointer to next playlist entry or NULL if playlist is empty
 */
evePlayListData* evePlayListManager::takeFirst(){

	if (playlist.size() == 0) return NULL;

	evePlayListEntry entry = playlist.takeFirst();
	eveDataEntry * dataentry;

	if (datahash.contains(entry.pid)){
		dataentry = datahash.take(entry.pid);
	}
	else {
		eveError::log(4, "evePlayListManager::takeFirst: List inconsistency no data found");
		return NULL;
	}
	if (!dataentry->isLoaded) {
		// TODO
		//load data from temporary file
		return NULL;
	}
	evePlayListData* data = new evePlayListData;
	data->name = QString(entry.name);
	data->author = QString(entry.author);
	data->data = QByteArray(dataentry->data);
	return data;
}
/**
 * \brief returns a pointer to a message with the current playlist display data
 * \return pointer to evePlayListMessage
 */
evePlayListMessage* evePlayListManager::getCurrentPlayList(){
	return new evePlayListMessage(playlist);
}
