TEMPLATE = lib
CONFIG += plugin
INCLUDEPATH += ../../evEngine/storageModule/ \
    ../../evEngine/nwModule/ \
    ../../evEngine/scanModule/ \
    /soft/epics/base-3.14.12.1/include \
    /soft/epics/base-3.14.12.1/include/os/Linux \
    /home/eden/src/hdf5/1.8.4/include/
HEADERS = hdf5Plugin.h
SOURCES = hdf5Plugin.cpp \
    ../../evEngine/scanModule/eveTime.cpp
LIBS += -L/home/eden/src/hdf5/1.8.4-32bit/lib-static \
    -lhdf5_cpp \
    -lhdf5 \
    -L/soft/epics/base-3.14.12.1/lib/linux-x86 \
    -lca \
    -lCom
TARGET = $$qtLibraryTarget(hdf5plugin)
DESTDIR = ../lib
