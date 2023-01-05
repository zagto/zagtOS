#pragma once

#include <common/inttypes.hpp>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include <processes/EventQueue.hpp>
#include <processes/Thread.hpp>
#include <processes/MemoryArea.hpp>
#include <processes/UserApi.hpp>
#include <processes/InterruptManager.hpp>

/* simple object that a Process can hold via a handle so it can access I/O ports */
struct IOPortRange {
    uint16_t start;
    uint16_t max;
};

struct Port {
    const shared_ptr<EventQueue> eventQueue;
    const size_t eventTag;
    bool active = true;
    mutex activeLock;

    Port(shared_ptr<EventQueue> eventQueue, size_t eventTag) noexcept :
        eventQueue{move(eventQueue)},
        eventTag{eventTag} {}
};

namespace handleManager {

using Type = hos_v1::HandleType;


struct AbstractElement {
    virtual ~AbstractElement() = default;
};

struct AbstractPointerElement : AbstractElement {
    virtual ~AbstractPointerElement() = default;

    virtual void constructCopyAt(AbstractElement *destination) const noexcept = 0;
};

template <typename T>
struct PointerElement : public AbstractPointerElement {
    T pointer;

    PointerElement(T pointer) :
        pointer{pointer} {}
    virtual ~PointerElement() = default;

    virtual void constructCopyAt(AbstractElement *destination) const noexcept override {
        new (destination) PointerElement<T>(*this);
    }
};

struct FreeElement : public AbstractElement {
    uint32_t nextFreeHandle;

    FreeElement(uint32_t nextFreeHandle) :
        nextFreeHandle{nextFreeHandle} {}
    virtual ~FreeElement() = default;
};

template struct PointerElement<shared_ptr<Port>>;
template struct PointerElement<weak_ptr<Port>>;
template struct PointerElement<shared_ptr<Thread>>;
template struct PointerElement<shared_ptr<MemoryArea>>;
template struct PointerElement<shared_ptr<BoundInterrupt>>;
template struct PointerElement<IOPortRange>;
template struct PointerElement<shared_ptr<EventQueue>>;
static constexpr size_t ELEMENT_SIZE = sizeof(PointerElement<shared_ptr<Port>>);
static constexpr size_t ELEMENT_ALIGN = alignof(PointerElement<shared_ptr<Port>>);
static_assert(sizeof(PointerElement<weak_ptr<Port>>) == ELEMENT_SIZE);
static_assert(alignof(PointerElement<weak_ptr<Port>>) == ELEMENT_ALIGN);
static_assert(sizeof(FreeElement) <= ELEMENT_SIZE);
static_assert(alignof(FreeElement) <= ELEMENT_ALIGN);

class HandleManager {
private:
    Process &process;
    uint8_t *data;
    size_t numAllocated;

    uint32_t nextFreeHandle;
    mutex lock;

    AbstractElement *at(uint32_t handle) const noexcept;
    shared_ptr<Process> sharedProcess() const noexcept;
    uint32_t grabFreeNumber();
    void _removeHandle(uint32_t handle,
                       shared_ptr<Thread> &removedThread,
                       shared_ptr<Port> &removedPort);
    bool handleValid(uint32_t handle) const noexcept;
    template<typename T> uint32_t _add(T pointer) noexcept {
        assert(lock.isLocked());
        uint32_t handle = grabFreeNumber();
        new (at(handle)) PointerElement<T>(pointer);
        return handle;
    }

public:
    HandleManager(Process &process);
    HandleManager(Process &process,
                  const hos_v1::Process &handOver,
                  const vector<shared_ptr<Thread>> &allThreads,
                  const vector<shared_ptr<Port>> &allPorts,
                  const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                  const vector<shared_ptr<EventQueue>> &allEventQueues);
    HandleManager(HandleManager &) = delete;

    template<typename T> uint32_t add(T pointer) noexcept {
        scoped_lock sl(lock);
        return _add(pointer);
    }
    template<typename T> T lookup(uint32_t handle) {
        scoped_lock sl(lock);
        if (handle >= numAllocated) {
            cout << "handle " << handle << " is out of range" << endl;
            throw BadUserSpace(sharedProcess());
        }
        auto result = dynamic_cast<PointerElement<T> *>(at(handle));
        if (!result) {
            cout << "handle " << handle << " does not point to an object of right type" << endl;
            throw BadUserSpace(sharedProcess());
        }
        return result->pointer;
    }
    void removeHandle(uint32_t number,
                      shared_ptr<Thread> &removedThread,
                      shared_ptr<Port> &removedPort);
    void transferHandles(vector<uint32_t> &handleValues, HandleManager &destination);
    uint32_t numFreeHandles() noexcept;
    void insertAllProcessPointersAfterKernelHandover(const shared_ptr<Process> &process) noexcept;
};


}

using handleManager::HandleManager;

