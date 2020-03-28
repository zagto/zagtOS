#pragma once

#include <lib/List.hpp>
#include <mutex>
#include <paging/PagingContext.hpp>
#include <processes/ELF.hpp>
#include <processes/Thread.hpp>
#include <processes/MappedArea.hpp>
#include <paging/PageTableEntry.hpp>
#include <processes/UUID.hpp>
#include <processes/Message.hpp>
#include <processes/HandleManager.hpp>

class Process {
private:
    friend class PagingContext;
    friend class Thread;
    friend class MMap;
    friend class MUnmap;
    vector<Thread *>threads;
    MappedAreaVector mappedAreas;
    HandleManager handleManager;

    vector<uint32_t> canUseTags;
    vector<uint32_t> ownTags;

    bool accessUserSpace(uint8_t *buffer,
                         size_t start,
                         size_t length,
                         PagingContext::AccessOperation accOp,
                         bool requireWritePermissions);

public:
    mutex pagingLock;
    mutex threadsLock;
    /*needed ? mutex portsLock;*/
    PagingContext *masterPageTable;
    Region runMessageRegion;

    Process(ELF elf, Thread::Priority initialPrioriy, UUID messageType, size_t messageSize);
    void activate();
    PhysicalAddress allocateFrame(UserVirtualAddress address,
                                  Permissions permissions);
    void freeFrame(UserVirtualAddress address);
    bool handlePageFault(UserVirtualAddress address);
    void removeThread(Thread *thread);

    bool copyFromOhterUserSpace(size_t destinationAddress,
                                Process *sourceProcess,
                                size_t sourceAddress,
                                size_t length,
                                bool requireWriteAccessToDestination);
    bool copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions);
    bool copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions);
    bool verifyUserAccess(size_t address, size_t length, bool requireWritePermissions);

    //void receiveMessage(Message *msg);
    size_t runMessageAddress();
};
