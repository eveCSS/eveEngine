#ifndef EVEVALIDATIONHANDLER_H
#define EVEVALIDATIONHANDLER_H

#include <QAbstractMessageHandler>
#include "eveManager.h"

class eveValidationHandler : public QAbstractMessageHandler
{
    Q_OBJECT
public:
    eveValidationHandler(eveManager *parent = 0);
    virtual void handleMessage(QtMsgType, const QString &, const QUrl &, const QSourceLocation & );

private:
    eveManager* manager;
};

#endif // EVEVALIDATIONHANDLER_H
