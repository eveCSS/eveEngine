TEMPLATE = app
TARGET = evEngine
QT += core \
    network \
    gui \
    xml
HEADERS += scanModule/evePosCalc.h \
    scanModule/eveMotorPosition.h \
    scanModule/eveDeviceList.h \
    scanModule/eveSMAxis.h \
    scanModule/eveTypes.h \
    scanModule/eveDevice.h \
    scanModule/eveXMLReader.h \
    scanModule/eveScanManager.h \
    scanModule/eveScanThread.h \
    scanModule/eveManagerThread.h \
    scanModule/eveManager.h \
    scanModule/eveStatusTracker.h \
    scanModule/eveScanModule.h \
    nwModule/eveParameter.h \
    nwModule/eveRequestManager.h \
    nwModule/eveMessageFilter.h \
    nwModule/eveError.h \
    nwModule/eveMessageFactory.h \
    nwModule/eveSocket.h \
    nwModule/eveNetObject.h \
    nwModule/eveMessage.h \
    nwModule/eveMessageChannel.h \
    nwModule/eveMessageHub.h \
    nwModule/eveNwThread.h \
    nwModule/evePlaylistManager.h
SOURCES += scanModule/evePosCalc.cpp \
    scanModule/eveMotorPosition.cpp \
    scanModule/eveDeviceList.cpp \
    scanModule/eveSMAxis.cpp \
    scanModule/eveDevice.cpp \
    scanModule/eveXMLReader.cpp \
    scanModule/eveScanManager.cpp \
    scanModule/eveScanThread.cpp \
    scanModule/eveManagerThread.cpp \
    scanModule/eveManager.cpp \
    scanModule/eveStatusTracker.cpp \
    scanModule/eveScanModule.cpp \
    nwModule/eveParameter.cpp \
    nwModule/eveRequestManager.cpp \
    nwModule/eveMessageFilter.cpp \
    nwModule/eveError.cpp \
    nwModule/eveMessageFactory.cpp \
    nwModule/eveSocket.cpp \
    nwModule/eveNetObject.cpp \
    nwModule/eveMessage.cpp \
    nwModule/eveMessageChannel.cpp \
    nwModule/eveMessageHub.cpp \
    nwModule/eveNwThread.cpp \
    main.cpp \
    nwModule/evePlayListManager.cpp
FORMS += 
RESOURCES += 
INCLUDEPATH += nwModule \
    /soft/epics/base-3.14.10/include \
    /soft/epics/base-3.14.10/include/os/Linux \
    scanModule
unix:LIBS += -L/opt/epics/base-3.14.10/lib/linux-x86 \
    -lca \
    -lCom
win32:LIBS += c:/epics/ca.lib \
    c:/epics/Com.lib
FORMS += 
