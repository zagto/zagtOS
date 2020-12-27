#pragma once

#include <mutex>
#include <queue>
#include <memory>
#include <utility>
#include <paging/PagingContext.hpp>
#include <processes/Thread.hpp>
#include <processes/MappedArea.hpp>
#include <paging/PageTableEntry.hpp>
#include <processes/UUID.hpp>
#include <processes/Message.hpp>
#include <processes/HandleManager.hpp>
#include <processes/FutexManager.hpp>
#include <syscalls/SpawnProcess.hpp>

class Process {
private:
    friend class PagingContext;
    friend class Thread;
    friend class MMap;
    friend class MProtect;
    friend class MUnmap;
    friend class Message;
    friend struct hos_v1::System;
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
    void coreDump(Thread *crashedThread);

public:
    mutex pagingLock;
    mutex threadsLock;
    PagingContext *pagingContext;
    FutexManager futexManager;

    /* The handover constructor is the only constructor you should take the result of and put it
     * into a shared pointer to pass it to the Threads. The other constructors do this by
     * themselves. DO NEVER create a shared_ptr of their results to prevent shared_ptr
     * duplication */
    Process(const hos_v1::Process &handOver,
            const vector<shared_ptr<Thread>> &allThreads,
            const vector<shared_ptr<Port>> &allPorts,
            const vector<shared_ptr<MemoryArea>> &allMemoryAreas);
    Process(Process &sourceProcess,
            vector<SpawnProcessSection> &sections,
            optional<SpawnProcessSection> &TLSSection,
            UserVirtualAddress entryAddress,
            Thread::Priority initialPrioriy,
            Message &runMessage,
            vector<uint8_t> logName);
    ~Process();
    void activate();
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

    bool canAccessPhysicalMemory() const;

    size_t runMessageAddress();
    void crash(const char *message, Thread *crashedThread);
    void exit();
};
