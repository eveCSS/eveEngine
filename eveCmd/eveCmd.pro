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

win32-g++ {
   UNAME = $$system(uname -s)
   WINVER = $$system(ver)
   contains( UNAME, [lL]inux ){
      message( Using Linux Mingw cross-compiler )
      EPICS_BASE = /soft/epics/base-3.14.12.2
   }
   contains( WINVER, Windows ){
      message( Using Windows Mingw compiler )
      EPICS_BASE = J:/epics/3.14/windows/base-3.14.12.2
   }
   TARGET_ARCH = win32-x86-mingw
   ARCH = WIN32
}

linux-g++-32 {
   EPICS_BASE = /soft/epics/base-3.14.12.2
   TARGET_ARCH = linux-x86
   ARCH = Linux
}

linux-g++ {
   EPICS_BASE = /soft/epics/base-3.14.12.2
   TARGET_ARCH = linux-x86_64
   ARCH = Linux
}

INCLUDEPATH += $$EPICS_BASE/include \
    $$EPICS_BASE/include/os/$$ARCH
LIBS += -L $$EPICS_BASE/lib/$$TARGET_ARCH \
    -lca \
    -lCom
QMAKE_RPATHDIR += $$EPICS_BASE/lib/$$TARGET_ARCH









