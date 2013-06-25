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
linux-g++-32 {
  INCLUDEPATH += /soft/epics/base-3.14.12.2/include \
    /soft/epics/base-3.14.12.2/include/os/Linux \
#    /home/eden/src/hdf5/1.8.4/include/
    /home/eden/src/hdf5/1.6.10-32bit/include/

  LIBS += -L/soft/epics/base-3.14.12.2/lib/linux-x86 \
    -L/home/eden/src/hdf5/1.6.10-32bit/lib \
#   -L/home/eden/src/hdf5/1.8.4-32bit/lib-static/ \
    -lhdf5_cpp \
    -lhdf5 \
    -lca \
    -lCom
  DESTDIR = ../lib
}
linux-g++-64 {
  INCLUDEPATH += /soft/epics/base-3.14.12.2/include \
    /soft/epics/base-3.14.12.2/include/os/Linux \
#    /home/eden/src/hdf5/1.8.4/include/
    /home/eden/src/hdf5/1.6.10_x86_64/include/
  LIBS += -L/soft/epics/base-3.14.12.2/lib/linux-x86_64 \
    -L/home/eden/src/hdf5/1.6.10_x86_64/lib64 \
#   -L/home/eden/src/hdf5/1.8.4-32bit/lib-static/ \
    -lhdf5_cpp \
    -lhdf5 \
    -lca \
    -lCom
  DESTDIR = ../lib64
}
TARGET = $$qtLibraryTarget(hdf5plugin)
