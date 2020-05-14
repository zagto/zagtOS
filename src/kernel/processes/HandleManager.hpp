#pragma once

#include <common/inttypes.hpp>
#include <vector>
#include <optional>
#include <mutex>
#include <memory>
#include <processes/Port.hpp>
#include <processes/Thread.hpp>

namespace handleManager {

enum class Type : uint32_t {
    INVALID, FREE, PORT, REMOTE_PORT, THREAD
};

static const uint32_t HANDLE_END = static_cast<uint32_t>(-1);

struct Handle {
    Type type{Type::INVALID};
    union HandleData {
        uint32_t nextFreeHandle;
        shared_ptr<Port> port;
        weak_ptr<Port> remotePort;
        shared_ptr<Thread> thread;

        HandleData() {}
    } data;

    Handle() {}
    Handle(uint32_t next);
    Handle(shared_ptr<Port> &port);
    Handle(weak_ptr<Port> &port);
    Handle(shared_ptr<Thread> &thread);
    Handle(const Handle &other);
    ~Handle();
    void operator=(const Handle &other);
    void destructData();
};

class HandleManager {
private:
    vector<Handle> handles;
    uint32_t nextFreeHandle{HANDLE_END};
    mutex lock;

    uint32_t grabFreeHandle();
    bool handleValidFor(uint32_t handle, Type type);
    uint32_t _addRemotePort(weak_ptr<Port> &port);

public:
    HandleManager() {}
    HandleManager(HandleManager &) = delete;

    uint32_t addPort(shared_ptr<Port> &port);
    uint32_t addThread(shared_ptr<Thread> &thread);
    optional<shared_ptr<Port>> lookupPort(uint32_t handle);
    optional<weak_ptr<Port>> lookupRemotePort(uint32_t handle);
    optional<shared_ptr<Thread>> lookupThread(uint32_t handle);
    bool removeHandle(uint32_t handle);
    bool transferHandles(vector<uint32_t> &handles,
                         HandleManager &destination);
    uint32_t numFreeHandles();
    void removeAllHandles();
};

}

using handleManager::HandleManager;

