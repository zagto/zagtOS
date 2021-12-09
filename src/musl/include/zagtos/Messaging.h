#ifndef __ZAGTOS_MESSAGING_H
#define __ZAGTOS_MESSAGING_H

#include <zagtos/ZBON.h>
#include <uuid/uuid.h>


static const uint32_t INVALID_HANDLE = (uint32_t)-1;

struct ZoMessageInfo {
    uuid_t type;
    /* index into the ports array of a ReceiveMessage call */
    size_t portIndex;
    struct ZbonEncodedData data;
};

struct ZoMessageInfo *ZoGetRunMessage(void);


int ZoCreateDMASharedMemory(size_t deviceMax, size_t length, size_t *deviceAddresses);
int ZoCreatePhysicalSharedMemory(size_t physicalAddress, size_t length);
int ZoCreateStandardSharedMemory(size_t length);
void ZoDeleteHandle(int handle);
void ZoUnmapWhole(void *pointer);

#endif
