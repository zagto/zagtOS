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

    constexpr size_t numAllocated() noexcept;
    constexpr uint32_t getHash(size_t address) noexcept;
    constexpr uint32_t reduceHash(uint32_t hash) noexcept;
    void moveData(vector<Futex> &newData) noexcept;
    void grow();
    void shrink() noexcept;
    size_t indexForNew(size_t address) noexcept;
    void removeElement(size_t index) noexcept;

public:
    mutex lock;

    FutexManager(hos_v1::Futex *futexes,
                 size_t numFutexes,
                 const vector<shared_ptr<Thread>> &allThreads);
    FutexManager();
    ~FutexManager();
    FutexManager(FutexManager &) = delete;

    /* These addresses can be physical addresses in the system-wide futex manger or virtual in the
     * process-wide one, so just use size_t */
    void wait(uint64_t id);
    size_t wake(uint64_t id, size_t numWake) noexcept;
};
