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
    void grow();
    void shrink();

public:
    mutex lock;

    FutexManager();
    ~FutexManager();
    FutexManager(FutexManager &) = delete;

    /* These addresses can be physical addresses in the system-wide futex manger or virtual in the
     * process-wide one, so just use size_t */
    void wait(size_t address, shared_ptr<Thread> &thread);
    size_t wake(size_t address, size_t numWake, bool wakeAll);
    void cancelWaiting(size_t address, shared_ptr<Thread> &thread);
    void ensureNoFutexOnPage(PhysicalAddress page);
};
