#include "evevalidationhandler.h"

eveValidationHandler::eveValidationHandler(eveManager *parent) :
    QAbstractMessageHandler(parent)
{
    manager = parent;
}

void eveValidationHandler::handleMessage( QtMsgType type, const QString & description, const QUrl & identifier, const QSourceLocation & sourceLocation ){
    int severity = ERROR;

    if (type == QtDebugMsg)
        severity = DEBUG;
    else if (type == QtWarningMsg)
        severity = MINOR;

    QString message=QString("Validation Error Line: %1, Column: %2").arg(sourceLocation.line()).arg(sourceLocation.column());
    manager->sendError(severity, EVEMESSAGEFACILITY_XMLVALIDATOR, 0, message);
}
