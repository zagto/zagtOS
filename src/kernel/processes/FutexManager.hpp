#pragma once

#include <common/common.hpp>
#include <mutex>
#include <vector>

template<typename T> class shared_ptr;
class Thread;
struct Futex;

class FutexManager {
private:
    static constexpr size_t MIN_BITS = 4;

    vector<Futex> data;
    size_t numBits;
    size_t numElements;

    constexpr size_t numAllocated();
    constexpr uint32_t getHash(size_t address);
    constexpr uint32_t reduceHash(uint32_t hash);
    void moveData(vector<Futex> &newData);
    Status grow();
    void shrink();
    size_t indexForNew(size_t address);
    void removeElement(size_t index);

public:
    mutex lock;

    FutexManager(hos_v1::Futex *futexes,
                 size_t numFutexes,
                 const vector<shared_ptr<Thread>> &allThreads,
                 Status &status);
    FutexManager(Status &status);
    ~FutexManager();
    FutexManager(FutexManager &) = delete;

    /* These addresses can be physical addresses in the system-wide futex manger or virtual in the
     * process-wide one, so just use size_t */
    Status wait(uint64_t id, Thread *&thread);
    size_t wake(PhysicalAddress address, size_t numWake);
    void cancelWaiting(PhysicalAddress address, Thread *&thread);
    void ensureNoFutexOnPage(PhysicalAddress page);
};
