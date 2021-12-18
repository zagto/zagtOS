#pragma once

#include <common/inttypes.hpp>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>
#include <processes/MemoryArea.hpp>
#include <processes/InterruptManager.hpp>

/* simple object that a Process can hold via a handle so it can access I/O ports */
struct IOPortRange {
    uint16_t start;
    uint16_t max;
};

namespace handleManager {

using Type = hos_v1::HandleType;

/* FUTEX_LOCK_PI wants to or a bit with a handle so make sure upmost bit is reserved */
static const uint32_t HANDLE_FIRST = 1;
static const uint32_t HANDLE_END = 0x3fffffff;

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
    void _removeHandle(uint32_t handle, shared_ptr<Thread> &removedThread);
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
                  const vector<shared_ptr<MemoryArea>> &allMemoryAreas);
    HandleManager(HandleManager &) = delete;

    template<typename T> uint32_t add(T pointer) noexcept {
        scoped_lock sl(lock);
        return _add(pointer);
    }
    template<typename T> T lookup(uint32_t handle) {
        scoped_lock sl(lock);
        if (handle < HANDLE_FIRST || handle >= numAllocated) {
            throw BadUserSpace(sharedProcess());
        }
        auto result = dynamic_cast<PointerElement<T> *>(at(handle));
        if (!result) {
            throw BadUserSpace(sharedProcess());
        }
        return result->pointer;
    }
    void removeHandle(uint32_t number, shared_ptr<Thread> &removedThread);
    void transferHandles(vector<uint32_t> &handleValues, HandleManager &destination);
    uint32_t numFreeHandles() noexcept;
    void insertAllProcessPointersAfterKernelHandover(const shared_ptr<Process> &process) noexcept;
};


}

using handleManager::HandleManager;

