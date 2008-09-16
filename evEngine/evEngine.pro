TEMPLATE = app
TARGET = evEngine
QT += core \
    network \
    gui
HEADERS += scanModule/eveScanModule.h \
    NwModul/eveParameter.h \
    NwModul/eveManagerThread.h \
    NwModul/eveManager.h \
    NwModul/eveRequestManager.h \
    NwModul/eveMessageFilter.h \
    NwModul/eveError.h \
    NwModul/eveMessageFactory.h \
    NwModul/eveSocket.h \
    NwModul/eveNetObject.h \
    NwModul/eveMessage.h \
    NwModul/eveMessageChannel.h \
    NwModul/eveMessageHub.h \
    NwModul/eveNwThread.h \
    NwModul/evePlaylistManager.h
SOURCES += scanModule/eveScanModule.cpp \
    NwModul/eveParameter.cpp \
    NwModul/eveManagerThread.cpp \
    NwModul/eveManager.cpp \
    NwModul/eveRequestManager.cpp \
    NwModul/eveMessageFilter.cpp \
    NwModul/eveError.cpp \
    NwModul/eveMessageFactory.cpp \
    NwModul/eveSocket.cpp \
    NwModul/eveNetObject.cpp \
    NwModul/eveMessage.cpp \
    NwModul/eveMessageChannel.cpp \
    NwModul/eveMessageHub.cpp \
    NwModul/eveNwThread.cpp \
    main.cpp \
    NwModul/evePlayListManager.cpp
FORMS += 
RESOURCES += 
INCLUDEPATH += NwModul \
    /soft/epics/base-3.14.9/include \
    /soft/epics/base-3.14.9/include/os/Linux
unix:LIBS += -L/soft/epics/base-3.14.9/lib/linux-x86 \
    -lca \
    -lCom
win32:LIBS += c:/epics/ca.lib \
    c:/epics/Com.lib
FORMS += 
