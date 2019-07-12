#ifndef TASK_HPP
#define TASK_HPP

#include <lib/List.hpp>
#include <lib/Lock.hpp>
#include <lib/SortedVector.hpp>
#include <paging/MasterPageTable.hpp>
#include <tasks/ELF.hpp>
#include <tasks/Thread.hpp>
#include <paging/PageTableEntry.hpp>

class MappedArea;
class Object;

class Task {
public:
    const static usize MAX_THREADS = 64;

private:
    friend class MasterPageTable;
    friend class Thread;
    Vector<Thread *>threads;
    SortedVector<MappedArea *>mappedAreas;
    UserVirtualAddress heapStart;

public:
    Lock pagingLock;
    Lock threadsLock;
    MasterPageTable *masterPageTable;

    Task(ELF elf, Thread::Priority initialPrioriy, Object *message);
    void activate();
    PhysicalAddress allocateFrame(UserVirtualAddress address,
                                  Permissions permissions);
    void freeFrame(UserVirtualAddress address);
    bool handlePageFault(UserVirtualAddress address);
    void removeThread(Thread *thread);

    Region findFreeRegion(usize size, bool &valid);
    MappedArea *findMappedArea(UserVirtualAddress address);
};

#endif // TASK_HPP
