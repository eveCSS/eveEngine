#include "pcmemspace.h"
#include "hdf5DataSet.h"

PCmemSpace::PCmemSpace(int size)
{
    memsize = 0;
    datastart = NULL;
    if (size < 5) size = 5;
    pcstart = (char*) malloc(size);
    if (pcstart) {
        memsize = size;
        datastart = pcstart+4;
    }
}

PCmemSpace::~PCmemSpace()
{
    if (memsize > 0) free(pcstart);
    memsize = 0;
    pcstart = NULL;
    datastart = NULL;
}

void PCmemSpace::setData(eveDataMessage* message){

    if (memsize < 1) return;

    if ((message->getDataType() == eveEnum16T) || (message->getDataType() == eveStringT) || (message->getDataType() == eveDateTimeT)){
        int stringLength = hdf5DataSet::convertToHdf5Type(message->getDataType()).getSize();

        char *tmpstart = datastart;
        char *bufferend = datastart + memsize - 4;
        foreach(QString messageString, message->getStringArray()){
            if((tmpstart + stringLength) <= bufferend) {
                strncpy(tmpstart, messageString.toLocal8Bit().constData(), stringLength);
                tmpstart += stringLength;
            }
        }
    }
    else {
        void *buffer = message->getBufferAddr();
        int length = message->getBufferLength();
        if (length > memsize - 4) return;
        memcpy(datastart, buffer, length);
    }
}

void PCmemSpace::setPosCount(int pc){

    if (memsize < 4) return;

    int *intptr = (int*) pcstart;
    *intptr = pc;
}
