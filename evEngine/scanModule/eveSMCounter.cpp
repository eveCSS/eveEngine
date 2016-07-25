
#include <stdio.h>
#include "eveSMCounter.h"
#include <QDateTime>
#include "eveError.h"

eveSMCounter::eveSMCounter(eveScanModule *scanm, eveSMBaseDevice *parent, QString xmlid, QString name, eveTransportDefinition* transdef) : eveBaseTransport(parent, xmlid, name) {

    scanModule = scanm;
    transStatus = eveUNDEFINED;
    currentAction = eveIDLE;
    haveMonitor = false;
    newData = NULL;
    accessname = transdef->getName();
}

eveSMCounter::~eveSMCounter() {
    // TODO Auto-generated destructor stub
}

int eveSMCounter::connectTrans(){
    emit done(0);
    return 0;
}

// TODO not yet implemented
int eveSMCounter::monitorTrans(){
    return 0;
}

/**
 * \brief get the data, may be a pointer to NULL
 * @return eveDataMessage or NULL
 */
eveDataMessage* eveSMCounter::getData(){
    eveDataMessage *return_data = newData;
    newData = NULL;
    return return_data;
};

/**
 * @param queue true, if request should be queued (needs execQueue() to actually start reading)
 * @return 0 if successful
 *
 * signals done if ready
 */
int eveSMCounter::readData(bool queue){

    newData = new eveDataMessage(xmlId, name, eveDataStatus(), DMTunmodified, eveTime::getCurrent(), QVector<int>(1,scanModule->getSMCount()));
    emit done(0);
    return 0;
}
/**
 * @param queue true, if request should be queued (needs execQueue() to actually start writing)
 * @return 0 if ok
 *
 * signals done if ready
 */
int eveSMCounter::writeData(eveVariant writedata, bool queue){

    sendError(ERROR, 0, "write data unsupported");
    emit done(0);

    return 0;
}

void eveSMCounter::sendError(int severity, int errorType,  QString message){

    scanModule->sendError(severity, EVEMESSAGEFACILITY_LOCALTIMER, errorType,
                       QString("SMCounter %1: %2").arg(name).arg(message));
}

/**
 *
 * @return a QStringList pointer which contains info
 */
QStringList* eveSMCounter::getInfo(){
    QStringList *sl = new QStringList();
    sl->append(QString("Access:local:%1").arg(accessname));
    return sl;
}
