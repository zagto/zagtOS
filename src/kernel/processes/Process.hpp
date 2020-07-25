#pragma once

#include <mutex>
#include <queue>
#include <memory>
#include <utility>
#include <paging/PagingContext.hpp>
#include <processes/ELF.hpp>
#include <processes/Thread.hpp>
#include <processes/MappedArea.hpp>
#include <paging/PageTableEntry.hpp>
#include <processes/UUID.hpp>
#include <processes/Message.hpp>
#include <processes/HandleManager.hpp>
#include <processes/FutexManager.hpp>

class Process {
private:
    friend class PagingContext;
    friend class Thread;
    friend class MMap;
    friend class MProtect;
    friend class MUnmap;
    friend class Message;
    /* For returnToUserMode - TODO: review if there is a better design for this */
    //friend class Interrupts;
    MappedAreaVector mappedAreas;
    HandleManager handleManager;
    vector<uint8_t> logName;

    bool accessUserSpace(uint8_t *buffer,
                         size_t start,
                         size_t length,
                         PagingContext::AccessOperation accOp,
                         bool requireWritePermissions);

public:
    mutex pagingLock;
    mutex threadsLock;
    PagingContext *pagingContext;
    FutexManager futexManager;

    Process(ELF elf, Thread::Priority initialPrioriy, Message &runMessage, vector<uint8_t> logName);
    ~Process();
    void activate();
    PhysicalAddress allocateFrame(UserVirtualAddress address,
                                  Permissions permissions);
    void freeFrame(UserVirtualAddress address);
    bool handlePageFault(UserVirtualAddress address);

    bool copyFromOhterUserSpace(size_t destinationAddress,
                                Process &sourceProcess,
                                size_t sourceAddress,
                                size_t length,
                                bool requireWriteAccessToDestination);
    bool copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions);
    bool copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions);
    bool verifyUserAccess(size_t address, size_t length, bool requireWritePermissions);
    bool verifyMessageAccess(size_t address, size_t length, size_t numHandles);

    size_t runMessageAddress();
    void crash(const char *message);
    void exit();
};
