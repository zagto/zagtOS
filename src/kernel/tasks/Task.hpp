#ifndef TASK_HPP
#define TASK_HPP

#include <lib/List.hpp>
#include <lib/Lock.hpp>
#include <paging/PagingContext.hpp>
#include <tasks/ELF.hpp>
#include <tasks/Thread.hpp>
#include <tasks/MappedArea.hpp>
#include <paging/PageTableEntry.hpp>

class Object;

class Task {
private:
    friend class PagingContext;
    friend class Thread;
    friend class MMap;
    friend class MUnmap;
    vector<Thread *>threads;
    MappedAreaVector mappedAreas;

    bool accessUserSpace(uint8_t *buffer,
                         size_t start,
                         size_t length,
                         PagingContext::AccessOperation accOp,
                         bool requireWritePermissions);

public:
    Lock pagingLock;
    Lock threadsLock;
    PagingContext *masterPageTable;
    UserVirtualAddress heapStart;

    Task(ELF elf, Thread::Priority initialPrioriy, Object *message);
    void activate();
    PhysicalAddress allocateFrame(UserVirtualAddress address,
                                  Permissions permissions);
    void freeFrame(UserVirtualAddress address);
    bool handlePageFault(UserVirtualAddress address);
    void removeThread(Thread *thread);

    bool copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions);
    bool copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions);
    bool verifyUserAccess(size_t address, size_t length, bool requireWritePermissions);
};

#endif // TASK_HPP
