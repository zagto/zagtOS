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

namespace handleManager {

using Type = hos_v1::HandleType;
static const uint32_t NUM_TYPES = 7;

/* FUTEX_LOCK_PI wants to or a bit with a handle so make sure upmost bit is reserved */
static const uint32_t NUMBER_END = 0x3fffffff;

struct Element {
    Type type{Type::INVALID};
    union HandleData {
        uint32_t nextFreeNumber;
        shared_ptr<Port> port;
        weak_ptr<Port> remotePort;
        shared_ptr<Thread> thread;
        shared_ptr<MemoryArea> memoryArea;
        shared_ptr<BoundInterrupt> interrupt;
        HandleData() noexcept {}
    } data;

    Element() noexcept {}
    Element(uint32_t next) noexcept;
    Element(shared_ptr<Port> &port) noexcept;
    Element(weak_ptr<Port> &port) noexcept;
    Element(shared_ptr<Thread> &thread) noexcept;
    Element(shared_ptr<MemoryArea> &memoryArea) noexcept;
    Element(shared_ptr<BoundInterrupt> &interrupt) noexcept;
    Element(const Element &) = delete;
    Element(const Element &&other) noexcept;
    ~Element();
    void operator=(const Element &) = delete;
    void operator=(const Element &&other) noexcept;
    void destructData() noexcept;
};

class HandleManager {
private:
    Process &process;
    vector<Element> elements;
    uint32_t nextFreeNumber{NUMBER_END};
    mutex lock;

    uint32_t grabFreeNumber();
    bool handleValidFor(uint32_t number, Type type) noexcept;
    bool handleValid(uint32_t number) noexcept;
    uint32_t _addRemotePort(weak_ptr<Port> &port);
    uint32_t _addMemoryArea(shared_ptr<MemoryArea> &memoryArea);
    uint32_t _addInterrupt(shared_ptr<BoundInterrupt> &interrupt);
    void _removeHandle(uint32_t number, shared_ptr<Thread> &removedThread);

public:
    HandleManager(Process &process) noexcept :
        process{process} {}
    HandleManager(Process &process,
                  const hos_v1::Process &handOver,
                  const vector<shared_ptr<Thread>> &allThreads,
                  const vector<shared_ptr<Port>> &allPorts,
                  const vector<shared_ptr<MemoryArea>> &allMemoryAreas);
    HandleManager(HandleManager &) = delete;

    uint32_t addPort(shared_ptr<Port> &port);
    uint32_t addThread(shared_ptr<Thread> &thread);
    uint32_t addMemoryArea(shared_ptr<MemoryArea> &sharedMemory);
    uint32_t addInterrupt(shared_ptr<BoundInterrupt> &interrupt);
    shared_ptr<Port> lookupPort(uint32_t number);
    weak_ptr<Port> lookupRemotePort(uint32_t number);
    shared_ptr<Thread> lookupThread(uint32_t number);
    shared_ptr<MemoryArea> lookupMemoryArea(uint32_t number);
    shared_ptr<BoundInterrupt> lookupInterrupt(uint32_t number);
    void removeHandle(uint32_t number, shared_ptr<Thread> &removedThread);
    void transferHandles(vector<uint32_t> &handleValues, HandleManager &destination);
    uint32_t numFreeHandles() noexcept;
    void insertAllProcessPointersAfterKernelHandover(const shared_ptr<Process> &process) noexcept;
};

}

using handleManager::HandleManager;

