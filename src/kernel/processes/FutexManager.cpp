#include <memory>
#include <queue>
#include <processes/FutexManager.hpp>
#include <processes/Thread.hpp>
#include <processes/Process.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>


struct Futex {
    bool active;
    /* 32-bit hashes will give us lots of collisions when there are close to 4G entries. But that
     * many active mutexes mean that many threads which means there are worse problems than that */
    uint32_t hash;
    uint64_t id;
    queue<Thread *> threads;

    Futex():
        active{false},
        hash{-1u},
        id{0} {}

    void destroy() {
        while (!threads.empty()) {
            Thread *thread = threads.top();
            threads.pop();
            thread->process->crash("Waiting on futex in memory region that no longer exists",
                                   thread);
        }
        *this = {};
    }
};


constexpr size_t FutexManager::numAllocated() {
    return 1u << numBits;
}

constexpr uint32_t FutexManager::getHash(uint64_t id) {
    /* Lower bits of id is the offset into the frame. Hash all IDs in same frame to same hash so we
     * can find them to enusre there is no futex left on a specific frame */
    id >>= PAGE_SHIFT;
    id ^= id >> 32u;
    /* Knuths multiplicative hash */
    return static_cast<uint32_t>(id) * 2654435761u;
}

constexpr uint32_t FutexManager::reduceHash(uint32_t hash) {
    return hash & ((1u << numBits) - 1);
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

Status FutexManager::grow() {
    if (!(numElements >= numAllocated() / 2)) {
        return Status::OK();
    }
    Status status;
    vector<Futex> newData(numAllocated() * 2, status);
    if (!status) {
        return status;
    }

    numBits++;
    moveData(newData);
    return Status::OK();
}

void FutexManager::shrink() {
    if (!(numElements < (numAllocated() / 4) && numBits > MIN_BITS)) {
        return;
    }
    Status status;
    vector<Futex> newData(numAllocated() / 2, status);
    /* in case of problems, just skip the shrinking - we need to be able to remove
     * futexes under all circumstances */
    if (status) {
        numBits--;
        moveData(newData);
    }
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
        data[indexForNew(tmp.id)] = move(tmp);

        index = (index + 1) % numAllocated();
    }
}

FutexManager::FutexManager(Status &status):
    data(1ul << MIN_BITS, status),
    numBits{MIN_BITS},
    numElements{0} {}


FutexManager::FutexManager(hos_v1::Futex *futexes,
                           size_t numFutexes,
                           const vector<shared_ptr<Thread>> &allThreads,
                           Status &status) {
    if (!status) {
        return;
    }

    numBits = MIN_BITS;
    while (numFutexes >= numAllocated() / 2) {
        numBits++;
    }
    status = data.resize(numAllocated());
    if (!status) {
        return;
    }

    for (size_t sourceIndex = 0; sourceIndex < numFutexes; sourceIndex++) {
        const hos_v1::Futex &handOver = futexes[sourceIndex];

        uint32_t hash = getHash(handOver.id);
        size_t index = reduceHash(hash);

        while (data[index].active) {
            assert(data[index].id != handOver.id);
        }

        Futex &element = data[index];
        assert(element.threads.empty());
        assert(handOver.numWaitingThreads > 0);
        for (size_t index = 0; index < handOver.numWaitingThreads; index++) {
            size_t threadIndex = handOver.waitingThreadIDs[index];

            status = element.threads.push_back(allThreads[threadIndex].get());
            if (!status) {
                return;
            }
        }

        element.active = true;
        element.id = handOver.id;
        element.hash = hash;
        numElements++;
    }

    /* set Thread states once all the futexes are created successfully */
    for (size_t sourceIndex = 0; sourceIndex < numFutexes; sourceIndex++) {
        const hos_v1::Futex &handOver = futexes[sourceIndex];
        for (size_t index = 0; index < handOver.numWaitingThreads; index++) {
            size_t threadIndex = handOver.waitingThreadIDs[index];
            allThreads[threadIndex]->setState(Thread::State::Futex(this, handOver.id));
        }
    }
}


FutexManager::~FutexManager() {
    assert(!lock.isLocked());

    for (Futex &element: data) {
        if (element.active) {
            element.destroy();
        }
    }
}

Status FutexManager::wait(uint64_t id, Thread *&thread) {
    assert(lock.isLocked());

    /* Thread can only do the syscall if it's active. */
    assert(thread->state().kind() == Thread::ACTIVE);

    uint32_t hash = getHash(id);
    size_t index = reduceHash(hash);

    while (data[index].active) {
        if (data[index].id == id) {
            /* found - add thread to queue */
            Status status = data[index].threads.push_back(thread);
            if (!status) {
                return status;
            }

            thread->currentProcessor()->scheduler.remove(thread);
            thread->setState(Thread::State::Futex(this, id));
            return Status::OK();
        }
        index = (index + 1) % numAllocated();
    }

    /* nobody waits on this address yet - create new data element */
    Status status = grow();
    if (!status) {
        return status;
    }

    Futex &element = data[index];
    assert(element.threads.empty());

    status = element.threads.push_back(thread);
    if (!status) {
        return status;
    }

    thread->currentProcessor()->scheduler.remove(thread);
    thread->setState(Thread::State::Futex(this, id));

    element.active = true;
    element.id = id;
    element.hash = hash;
    numElements++;
    return Status::OK();
}

size_t FutexManager::wake(uint64_t id, size_t numWake) {
    assert(lock.isLocked());

    size_t index = reduceHash(getHash(id));
    while (data[index].active) {
        Futex &element = data[index];
        if (element.id == id) {
            /* found - wake thread(s) */
            if (numWake >= element.threads.size()) {
                numWake = element.threads.size();
            }

            for (size_t i = 0; i < numWake; i++) {
                Thread *thread = element.threads.top();

                assert(thread->state().kind() == Thread::FUTEX);
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
