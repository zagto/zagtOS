#ifndef TASK_HPP
#define TASK_HPP

#include <lib/List.hpp>
#include <lib/Lock.hpp>
#include <paging/PagingContext.hpp>
#include <tasks/ELF.hpp>
#include <tasks/Thread.hpp>
#include <tasks/MappedArea.hpp>
#include <paging/PageTableEntry.hpp>
#include <tasks/UUID.hpp>
#include <tasks/Message.hpp>

class Task {
private:
    friend class PagingContext;
    friend class Thread;
    friend class MMap;
    friend class MUnmap;
    vector<Thread *>threads;
    vector<Port *>ports;
    MappedAreaVector mappedAreas;

    vector<uint32_t> canUseTags;
    vector<uint32_t> ownTags;

    bool accessUserSpace(uint8_t *buffer,
                         size_t start,
                         size_t length,
                         PagingContext::AccessOperation accOp,
                         bool requireWritePermissions);

public:
    Lock pagingLock;
    Lock threadsLock;
    Lock portsLock;
    PagingContext *masterPageTable;
    Region runMessageRegion;

    Task(ELF elf, Thread::Priority initialPrioriy, UUID messageType, size_t messageSize);
    void activate();
    PhysicalAddress allocateFrame(UserVirtualAddress address,
                                  Permissions permissions);
    void freeFrame(UserVirtualAddress address);
    bool handlePageFault(UserVirtualAddress address);
    void removeThread(Thread *thread);

    bool copyFromOhterUserSpace(size_t destinationAddress,
                                Task *sourceTask,
                                size_t sourceAddress,
                                size_t length,
                                bool requireWriteAccessToDestination);
    bool copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions);
    bool copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions);
    bool verifyUserAccess(size_t address, size_t length, bool requireWritePermissions);

    //void receiveMessage(Message *msg);
    size_t runMessageAddress();
};

#endif // TASK_HPP
