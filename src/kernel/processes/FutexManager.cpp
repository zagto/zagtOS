#include <memory>
#include <queue>
#include <processes/FutexManager.hpp>
#include <processes/Thread.hpp>
#include <processes/Process.hpp>


struct Futex {
    bool active;
    size_t address;
    /* 32-bit hashes will give us lots of collisions when there are close to 4G entries. But that
     * many active mutexes mean that many threads which means there are worse problems than that */
    uint32_t hash;
    queue<shared_ptr<Thread>> threads;

    Futex():
        active{false},
        address{-1ul},
        hash{-1u} {}

    void destroy() {
        while (!threads.empty()) {
            shared_ptr<Thread> thread = threads.top();
            threads.pop();
            thread->process->crash("Waiting on futex in memory region that no longer exists");
        }
        *this = {};
    }
};


constexpr size_t FutexManager::numAllocated() {
    return 1u << numBits;
}

constexpr uint32_t FutexManager::getHash(size_t address) {
    /* hash all addresses in same page to same hash so we can find them to enusre there is no futex
     * left on a specific page */
    address >>= PAGE_SHIFT;
    if constexpr (PLATFORM_BITS == 64) {
        address &= address >> 32u;
    }
    /* Knuths multiplicative hash */
    return static_cast<uint32_t>(address) * 2654435761u;
}

constexpr uint32_t FutexManager::reduceHash(uint32_t hash) {
    return hash & (32u - numBits);
}

void FutexManager::moveData(vector<Futex> &newData) {
    for (Futex &old: data) {
       if (old.active) {
            uint32_t newIndex = reduceHash(old.hash);
            while (newData[newIndex].active) {
                newIndex = (newIndex + 1) % numAllocated();
            }
            data[newIndex] = move(old);
        }
    }
    data = move(newData);
}

void FutexManager::grow() {
    if (!(numElements >= (numAllocated() / 4) * 3)) {
        return;
    }
    vector<Futex> newData(numAllocated() * 2);
    /* at this point there can be no more exceptions, start changing the data */
    numBits++;
    moveData(newData);
}

void FutexManager::shrink() {
    if (!(numElements < (numAllocated() / 3) && numBits > MIN_BITS)) {
        return;
    }
    vector<Futex> newData(numAllocated() / 2);
    /* at this point there can be no more exceptions, start changing the data */
    numBits--;
    moveData(newData);
}

FutexManager::FutexManager():
    data(1ul << MIN_BITS),
    numBits{MIN_BITS},
    numElements{0} {}


FutexManager::~FutexManager() {
    assert(!lock.isLocked());

    for (Futex &element: data) {
        if (element.active) {
            element.destroy();
        }
    }
}

void FutexManager::wait(size_t address, shared_ptr<Thread> &thread) {
    assert(lock.isLocked());
    assert(thread->status == Thread::Status::RUNNING);

    uint32_t hash = getHash(address);
    size_t index = reduceHash(hash);

    while (data[index].active) {
        if (data[index].address == address) {
            /* found - add thread to queue */
            data[index].threads.push_back(thread);
            numElements++;
            return;
        }
        index = (index + 1) % numAllocated();
    }

    /* nobody waits on this address yet - create new data element */
    grow();
    Futex &element = data[index];
    assert(element.threads.empty());
    element.threads.push_back(thread);

    element.active = true;
    element.address = address;
    element.hash = hash;
    numElements++;
}

size_t FutexManager::wake(size_t address, size_t numWake, bool wakeAll) {
    assert(lock.isLocked());

    size_t index = reduceHash(getHash(address));
    while (data[index].active) {
        Futex &element = data[index];
        if (element.address == address) {
            /* found - wake thread(s) */

            while (numWake > 0 || wakeAll) {
                shared_ptr<Thread> thread = element.threads.top();

                assert(thread->status == Thread::Status::FUTEX_PUBLIC
                       || thread->status == Thread::Status::FUTEX_PRIVATE);


                element.threads.pop();
                numWake--;
            }
        }
    }

    shrink();
}

void FutexManager::ensureNoFutexOnPage(PhysicalAddress page) {
    scoped_lock sl(lock);
}
