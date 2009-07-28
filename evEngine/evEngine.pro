TEMPLATE = app
TARGET = evEngine
QT += core \
    network \
    gui \
    xml
HEADERS += scanModule/eveSMChannel.h \
    scanModule/eveSMDevice.h \
    scanModule/eveVariant.h \
    scanModule/eveSetValue.h \
    scanModule/eveData.h \
    scanModule/eveCaTransport.h \
    scanModule/eveBaseTransport.h \
    scanModule/evePosCalc.h \
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
SOURCES += scanModule/eveSMChannel.cpp \
    scanModule/eveSMDevice.cpp \
    scanModule/eveVariant.cpp \
    scanModule/eveSetValue.cpp \
    scanModule/eveData.cpp \
    scanModule/eveCaTransport.cpp \
    scanModule/eveBaseTransport.cpp \
    scanModule/evePosCalc.cpp \
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
INCLUDEPATH += nwModule \
    /soft/epics/base-3.14.10/include \
    /soft/epics/base-3.14.10/include/os/Linux \
    scanModule
unix:
unix:LIBS += -L/opt/epics/base-3.14.10/lib/linux-x86 \
    -lca \
    -lCom \
    -L/opt/test
win32:INCLUDEPATH += nwModule \
    J:\epics\3.14\windows\base-3.14.10\include \
    J:\epics\3.14\windows\base-3.14.10\include\os\WIN32 \
    scanModule
win32:LIBS += J:\epics\3.14\windows\base-3.14.10\lib\win32-x86-mingw\ca.lib \
    J:\epics\3.14\windows\base-3.14.10\lib\win32-x86-mingw\Com.lib \
    -lws2_32
FORMS += 
RESOURCES += 
