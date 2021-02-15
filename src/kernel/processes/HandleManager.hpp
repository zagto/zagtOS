#pragma once

#include <common/inttypes.hpp>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>
#include <processes/MemoryArea.hpp>

namespace handleManager {

using Type = hos_v1::HandleType;
static const uint32_t NUM_TYPES = 6;

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
        HandleData() {}
    } data;

    Element() {}
    Element(uint32_t next);
    Element(shared_ptr<Port> &port);
    Element(weak_ptr<Port> &port);
    Element(shared_ptr<Thread> &thread);
    Element(shared_ptr<MemoryArea> &memoryArea);
    Element(const Element &other);
    ~Element();
    void operator=(const Element &other);
    void destructData();
};

class HandleManager {
private:
    vector<Element> elements;
    uint32_t nextFreeNumber{NUMBER_END};
    mutex lock;

    uint32_t grabFreeNumber();
    bool handleValidFor(uint32_t number, Type type);
    uint32_t _addRemotePort(weak_ptr<Port> &port);
    uint32_t _addMemoryArea(shared_ptr<MemoryArea> &memoryArea);
    Status _removeHandle(uint32_t number, shared_ptr<Thread> &removedThread);

public:
    HandleManager() {}
    HandleManager(const hos_v1::Process &handOver,
                  const vector<shared_ptr<Thread>> &allThreads,
                  const vector<shared_ptr<Port>> &allPorts,
                  const vector<shared_ptr<MemoryArea>> &allMemoryAreas);
    HandleManager(HandleManager &) = delete;

    Result<uint32_t> addPort(shared_ptr<Port> &port);
    Result<uint32_t> addThread(shared_ptr<Thread> &thread);
    Result<uint32_t> addMemoryArea(shared_ptr<MemoryArea> &sharedMemory);
    optional<shared_ptr<Port>> lookupPort(uint32_t number);
    optional<weak_ptr<Port>> lookupRemotePort(uint32_t number);
    optional<shared_ptr<Thread>> lookupThread(uint32_t number);
    optional<shared_ptr<MemoryArea>> lookupMemoryArea(uint32_t number);
    shared_ptr<Thread> extractThread();
    Status removeHandle(uint32_t number, shared_ptr<Thread> &removedThread);
    Status transferHandles(vector<uint32_t> &elements, HandleManager &destination);
    uint32_t numFreeHandles();
    void insertAllProcessPointersAfterKernelHandover(const shared_ptr<Process> &process);
};

}

using handleManager::HandleManager;

