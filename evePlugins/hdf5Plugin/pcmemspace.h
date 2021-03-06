#ifndef PCMEMSPACE_H
#define PCMEMSPACE_H
#include "eveMessage.h"

class PCmemSpace
{
public:
    PCmemSpace(int, bool);
    virtual ~PCmemSpace();
    void setPosCount(int);
    void setData(eveDataMessage* message);
    void clear(){if (memsize > 0) memset(pcstart, 0, memsize);};
    void* getBufferStartAddr(){return (void*)pcstart;};


private:
    int memsize;
    char* pcstart;
    char* datastart;
    bool longString;
};

#endif // PCMEMSPACE_H
