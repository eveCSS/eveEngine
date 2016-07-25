#ifndef EVESMCOUNTER_H
#define EVESMCOUNTER_H

#include <QString>
#include "eveBaseTransport.h"
#include "eveDeviceDefinitions.h"
#include "eveScanModule.h"

class eveSMCounter : public eveBaseTransport
{
    Q_OBJECT
public:
    eveSMCounter(eveScanModule* , eveSMBaseDevice *,QString, QString, eveTransportDefinition*);
    virtual ~eveSMCounter();
    int readData(bool);
    int writeData(eveVariant, bool);
    int connectTrans();
    int monitorTrans();
    bool isConnected(){return true;};
    bool haveData(){return true;};
    eveDataMessage *getData();
    QStringList* getInfo();
    void sendError(int, int,  QString);
    int execQueue();

private:
    bool haveMonitor;
    eveScanModule* scanModule;
    QString accessname;
    eveTransStatusT transStatus;
    eveTransActionT currentAction;
};

#endif // EVESMCOUNTER_H

