#ifndef TASK_HPP
#define TASK_HPP

#include <lib/List.hpp>
#include <lib/Lock.hpp>
#include <paging/MasterPageTable.hpp>
#include <tasks/ELF.hpp>
#include <tasks/Thread.hpp>
#include <tasks/MappedArea.hpp>
#include <paging/PageTableEntry.hpp>

class Object;

class Task {
public:
    const static usize MAX_THREADS = 64;

private:
    friend class MasterPageTable;
    friend class Thread;
    Vector<Thread *>threads;
    MappedAreaVector mappedAreas;

    bool accessUserSpace(u8 *buffer,
                         usize start,
                         usize length,
                         MasterPageTable::AccessOpertion accOp,
                         bool requireWritePermissions);

public:
    Lock pagingLock;
    Lock threadsLock;
    MasterPageTable *masterPageTable;
    UserVirtualAddress heapStart;

    Task(ELF elf, Thread::Priority initialPrioriy, Object *message);
    void activate();
    PhysicalAddress allocateFrame(UserVirtualAddress address,
                                  Permissions permissions);
    void freeFrame(UserVirtualAddress address);
    bool handlePageFault(UserVirtualAddress address);
    void removeThread(Thread *thread);

    bool copyFromUser(u8 *destination, usize address, usize length, bool requireWritePermissions);
    bool copyToUser(usize address, const u8 *source, usize length, bool requireWritePermissions);
    bool verifyUserAccess(usize address, usize length, bool requireWritePermissions);
};

#endif // TASK_HPP
