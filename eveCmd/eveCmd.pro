#-------------------------------------------------
#
# Project created by QtCreator 2012-11-12T14:51:19
#
#-------------------------------------------------

QT       += core
QT       += network
QT       += gui

TARGET = eveCmd
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    ../evEngine/nwModule/eveError.cpp \
    ../evEngine/nwModule/eveMessage.cpp \
    ../evEngine/nwModule/eveMessageFactory.cpp \
    ../evEngine/scanModule/eveVariant.cpp \
    ../evEngine/scanModule/eveDataStatus.cpp \
    ../evEngine/scanModule/eveStartTime.cpp \
    ../evEngine/scanModule/eveTime.cpp \
    Cmdclient.cpp \
    ../evEngine/nwModule/eveParameter.cpp

HEADERS += \
    ../evEngine/nwModule/eveMessage.h \
    ../evEngine/nwModule/eveMessageFactory.h \
    ../evEngine/nwModule/eveError.h \
    ../evEngine/scanModule/eveVariant.h \
    ../evEngine/scanModule/eveDataStatus.h \
    ../evEngine/scanModule/eveStartTime.h \
    ../evEngine/scanModule/eveTime.h \
    Cmdclient.h \
    ../evEngine/nwModule/eveParameter.h

INCLUDEPATH += \
    ../evEngine/nwModule \
    ../evEngine/scanModule \

linux-g++-32 {
  INCLUDEPATH += /soft/epics/base-3.14.12.2/include \
    /soft/epics/base-3.14.12.2/include/os/Linux
  LIBS += -L/soft/epics/base-3.14.12.2/lib/linux-x86 \
    -lca \
    -lCom
}
linux-g++-64 {
  INCLUDEPATH += /soft/epics/base-3.14.12.2/include \
    /soft/epics/base-3.14.12.2/include/os/Linux
  LIBS += -L/soft/epics/base-3.14.12.2/lib/linux-x86_64 \
    -lca \
    -lCom
}
win32:INCLUDEPATH += nwModule \
    J:\epics\3.14\windows\base-3.14.12.2\include \
    J:\epics\3.14\windows\base-3.14.12.2\include\os\WIN32 \
    scanModule
win32:LIBS += J:\epics\3.14\windows\base-3.14.12.2\lib\win32-x86-mingw\ca.lib \
    J:\epics\3.14\windows\base-3.14.12.2\lib\win32-x86-mingw\Com.lib \
    -lws2_32









