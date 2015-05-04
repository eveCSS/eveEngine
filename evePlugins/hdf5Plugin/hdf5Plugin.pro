TEMPLATE = lib
CONFIG += plugin
QMAKE_CXXFLAGS += -DH5_USE_16_API
INCLUDEPATH += ../../evEngine/storageModule/ \
    ../../evEngine/nwModule/ \
    ../../evEngine/scanModule/
HEADERS = hdf5DataSet.h \
    hdf5Plugin.h
SOURCES = hdf5DataSet.cpp \
    hdf5Plugin.cpp \
    ../../evEngine/scanModule/eveTime.cpp

win32-g++ {
   UNAME = $$system(uname -s)
   WINVER = $$system(ver)
   contains( UNAME, [lL]inux ){
      message( Using Linux Mingw cross-compiler )
      EPICS_BASE = /soft/epics/base-3.14.12.2
      HDF_BASE = /home/eden/prog/mxe/mxe_stable/usr/i686-pc-mingw32
   }
   contains( WINVER, Windows ){
      message( Using Windows Mingw compiler )
      EPICS_BASE = J:/epics/3.14/windows/base-3.14.12.2
      HDF_BASE = J:/prog/hdf/hdf5-1.8.10_mxe
   }
   TARGET_ARCH = win32-x86-mingw
   ARCH = WIN32
}

linux-g++-32 {
   HDF_BASE = /home/eden/src/hdf5/1.6.10-32bit
   HDF_LIB = $$HDF_BASE/lib
   EPICS_BASE = /soft/epics/base-3.14.12.2
   TARGET_ARCH = linux-x86
   ARCH = Linux
}

linux-g++ {
   HDF_BASE = /home/eden/src/hdf5/1.6.10_x86_64
   HDF_LIB = $$HDF_BASE/lib64
   EPICS_BASE = /soft/epics/base-3.14.12.2
   TARGET_ARCH = linux-x86_64
   ARCH = Linux
}

INCLUDEPATH += $$EPICS_BASE/include \
    $$EPICS_BASE/include/os/$$ARCH \
    $$HDF_BASE/include

LIBS += -L $$EPICS_BASE/lib/$$TARGET_ARCH \
    -L$$HDF_LIB \
    -lhdf5_cpp \
    -lhdf5 \
    -lca \
    -lCom

TARGET = $$qtLibraryTarget(hdf5plugin)
