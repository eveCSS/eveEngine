TEMPLATE = app
TARGET = evEngine
QT += core \
    network \
    gui \
    xml \
    xmlpatterns

HEADERS += scanModule/eveDeviceDefinitions.h \
    scanModule/eveSMMotor.h \
    scanModule/eveSMDetector.h \
    scanModule/eveDataStatus.h \
    scanModule/eveStartTime.h \
    eventModule/eveMonitorRegisterMessage.h \
    scanModule/eveSMStatus.h \
    scanModule/eveChainStatus.h \
    storageModule/eveSimplePV.h \
    mathModule/eveCalc.h \
    mathModule/eveMathManager.h \
    mathModule/eveMathThread.h \
    scanModule/eveAverage.h \
    scanModule/eveSMBaseDevice.h \
    scanModule/eveCounter.h \
    scanModule/eveTime.h \
    scanModule/eveTimer.h \
    eventModule/eveDeviceMonitor.h \
    eventModule/eveEventRegisterMessage.h \
    eventModule/eveEventProperty.h \
    eventModule/eveEventManager.h \
    eventModule/eveEventThread.h \
    storageModule/eveFileTest.h \
    storageModule/eveAsciiFileWriter.h \
    storageModule/eveFileWriter.h \
    storageModule/eveDataCollector.h \
    storageModule/eveStorageManager.h \
    storageModule/eveStorageThread.h \
    scanModule/eveSMChannel.h \
    scanModule/eveSMDevice.h \
    scanModule/eveVariant.h \
    scanModule/eveCaTransport.h \
    scanModule/eveBaseTransport.h \
    scanModule/evePosCalc.h \
    scanModule/eveDeviceList.h \
    scanModule/eveSMAxis.h \
    scanModule/eveTypes.h \
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
    nwModule/evePlayListManager.h \
    mathModule/eveMath.h \
    mathModule/eveMathConfig.h \
    version.h \
    nwModule/evevalidationhandler.h
SOURCES += scanModule/eveDeviceDefinitions.cpp \
    scanModule/eveSMMotor.cpp \
    scanModule/eveSMDetector.cpp \
    scanModule/eveDataStatus.cpp \
    scanModule/eveStartTime.cpp \
    eventModule/eveMonitorRegisterMessage.cpp \
    scanModule/eveSMStatus.cpp \
    scanModule/eveChainStatus.cpp \
    storageModule/eveSimplePV.cpp \
    mathModule/eveCalc.cpp \
    mathModule/eveMathManager.cpp \
    mathModule/eveMathThread.cpp \
    scanModule/eveAverage.cpp \
    scanModule/eveSMBaseDevice.cpp \
    scanModule/eveCounter.cpp \
    scanModule/eveTime.cpp \
    scanModule/eveTimer.cpp \
    eventModule/eveDeviceMonitor.cpp \
    eventModule/eveEventRegisterMessage.cpp \
    eventModule/eveEventProperty.cpp \
    eventModule/eveEventManager.cpp \
    eventModule/eveEventThread.cpp \
    storageModule/eveFileTest.cpp \
    storageModule/eveAsciiFileWriter.cpp \
    storageModule/eveFileWriter.cpp \
    storageModule/eveDataCollector.cpp \
    storageModule/eveStorageManager.cpp \
    storageModule/eveStorageThread.cpp \
    scanModule/eveSMChannel.cpp \
    scanModule/eveSMDevice.cpp \
    scanModule/eveVariant.cpp \
    scanModule/eveCaTransport.cpp \
    scanModule/eveBaseTransport.cpp \
    scanModule/evePosCalc.cpp \
    scanModule/eveDeviceList.cpp \
    scanModule/eveSMAxis.cpp \
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
    nwModule/evePlayListManager.cpp \
    mathModule/eveMath.cpp \
    mathModule/eveMathConfig.cpp \
    nwModule/evevalidationhandler.cpp
INCLUDEPATH += nwModule \
    scanModule \
    storageModule \
    eventModule \
    mathModule

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

linux-g++-64 {
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

win32-g++ {
   contains( WINVER, Windows ){
      LIBS += -L C:\MinGW\lib
      LIBS += -lws2_32 -ladvapi32 -lshell32 -luser32 -lkernel32
    }
}
