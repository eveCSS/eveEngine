TEMPLATE = lib
CONFIG += plugin
QMAKE_CXXFLAGS += -DH5_USE_16_API
INCLUDEPATH += ../../evEngine/storageModule/ \
    ../../evEngine/nwModule/ \
    ../../evEngine/scanModule/ \
    /soft/epics/base-3.14.12.1/include \
    /soft/epics/base-3.14.12.1/include/os/Linux \
#    /home/eden/src/hdf5/1.8.4/include/
	/home/eden/src/hdf5/1.6.10-32bit/include/
HEADERS = hdf5DataSet.h \
    hdf5Plugin.h
SOURCES = hdf5DataSet.cpp \
    hdf5Plugin.cpp \
    ../../evEngine/scanModule/eveTime.cpp
LIBS += -L/home/eden/src/hdf5/1.6.10-32bit/lib \
#LIBS += -L/home/eden/src/hdf5/1.8.4-32bit/lib-static/ \
    -lhdf5_cpp \
    -lhdf5 \
    -L/soft/epics/base-3.14.12.1/lib/linux-x86 \
    -lca \
    -lCom
TARGET = $$qtLibraryTarget(hdf5plugin)
DESTDIR = ../lib
