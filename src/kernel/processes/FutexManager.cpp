#include <memory>
#include <queue>
#include <processes/FutexManager.hpp>
#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <processes/Scheduler.hpp>


struct Futex {
    bool active;
    size_t address;
    /* 32-bit hashes will give us lots of collisions when there are close to 4G entries. But that
     * many active mutexes mean that many threads which means there are worse problems than that */
    uint32_t hash;
    queue<Thread *> threads;

    Futex():
        active{false},
        address{-1ul},
        hash{-1u} {}

    void destroy() {
        while (!threads.empty()) {
            Thread *thread = threads.top();
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
    if (!(numElements >= numAllocated() / 2)) {
        return;
    }
    vector<Futex> newData(numAllocated() * 2);
    /* at this point there can be no more exceptions, start changing the data */
    numBits++;
    moveData(newData);
}

void FutexManager::shrink() {
    if (!(numElements < (numAllocated() / 4) && numBits > MIN_BITS)) {
        return;
    }
    vector<Futex> newData(numAllocated() / 2);
    /* at this point there can be no more exceptions, start changing the data */
    numBits--;
    moveData(newData);
}

size_t FutexManager::indexForNew(size_t address) {
    size_t index = reduceHash(getHash(address));
    while (data[index].active) {
        index = (index + 1) % numAllocated();
    }
    return index;
}

void FutexManager::removeElement(size_t index) {
    assert(data[index].active);
    index = (index + 1) % numAllocated();

    while (data[index].active) {
        Futex tmp;
        tmp = move(data[index]);
        data[index] = {};
        data[indexForNew(tmp.address)] = move(tmp);

        index = (index + 1) % numAllocated();
    }
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

void FutexManager::wait(PhysicalAddress address, Thread *&thread) {
    assert(lock.isLocked());

    scoped_lock sl(thread->stateLock);
    assert(thread->state() == Thread::State::RUNNING);

    uint32_t hash = getHash(address.value());
    size_t index = reduceHash(hash);

    while (data[index].active) {
        if (data[index].address == address.value()) {
            /* found - add thread to queue */
            data[index].threads.push_back(thread);
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
    element.address = address.value();
    element.hash = hash;
    numElements++;
}

size_t FutexManager::wake(PhysicalAddress address, size_t numWake) {
    assert(lock.isLocked());

    size_t index = reduceHash(getHash(address.value()));
    while (data[index].active) {
        Futex &element = data[index];
        if (element.address == address.value()) {
            /* found - wake thread(s) */
            if (numWake >= element.threads.size()) {
                numWake = element.threads.size();
            }

            for (size_t i = 0; i < numWake; i++) {
                Thread *thread = element.threads.top();

                scoped_lock sl(thread->stateLock);
                assert(thread->state() == Thread::State::FUTEX_PUBLIC
                       || thread->state() == Thread::State::FUTEX_PRIVATE);

                Scheduler::schedule(thread);

                element.threads.pop();
            }

            if (element.threads.empty()) {
                removeElement(index);

                // TODO: try and catch OOM - this should not fail, shrinking is not important
                /* shrink may go OOM - do it first so in this case nothing is changed */
                shrink();
                numElements--;
            }
            return numWake;
        }

        index = (index + 1) % numAllocated();
    }
    return 0;
}

void FutexManager::ensureNoFutexOnPage(PhysicalAddress page) {
    scoped_lock sl(lock);

    bool found;
    do {
        found = false;

        size_t index = reduceHash(getHash(page.value()));
        while (data[index].active) {
            Futex &element = data[index];

            if ((element.address >> PAGE_SHIFT) == (page.value() >> PAGE_SHIFT)) {
                found = true;
                element.destroy();
                removeElement(index);
                break;
            }

            index = (index + 1) % numAllocated();
        }
    } while (found);
}
