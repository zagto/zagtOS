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
    INVALID, FREE, PORT, REMOTE_PORT
};

static const uint32_t HANDLE_END = static_cast<uint32_t>(-1);

struct Handle {
    Type type;
    union HandleData {
        uint32_t nextFreeHandle;
        shared_ptr<Port> port;
        weak_ptr<Port> remotePort;

        HandleData() {}
    } data;

    void clear() {
        switch (type) {
        case Type::FREE:
        case Type::INVALID:
            break;
        case Type::PORT:
            data.port.~shared_ptr();
            break;
        case Type::REMOTE_PORT:
            data.remotePort.~weak_ptr();
            break;
        }
        type = Type::INVALID;
    }
    Handle():
        type{Type::INVALID} {}
    Handle(uint32_t next) {
        type = Type::FREE;
        data.nextFreeHandle = next;
    }
    Handle(shared_ptr<Port> &port) {
        type = Type::PORT;
        new (&data.port) shared_ptr<Port>(port);
    }
    Handle(const Handle &other) {
        type = other.type;
        switch (type) {
        case Type::INVALID:
            break;
        case Type::FREE:
            data.nextFreeHandle = other.data.nextFreeHandle;
            break;
        case Type::PORT:
            data.port = other.data.port;
            break;
        case Type::REMOTE_PORT:
            data.remotePort = other.data.remotePort;
            break;
        }
    }
    ~Handle() {
        clear();
    }
    void operator=(const Handle &other) {
        clear();
        new (this) Handle(other);
    }
};

class HandleManager {
private:
    vector<Handle> handles;
    uint32_t nextFreeHandle{HANDLE_END};
    mutex lock;

    optional<uint32_t> grabFreeHandle() {
        if (handles.size() == HANDLE_END && nextFreeHandle == HANDLE_END) {
            return {};
        }
        if (nextFreeHandle == HANDLE_END) {
            handles.resize(handles.size() + 1);
            return static_cast<uint32_t>(handles.size() - 1);
        } else {
            uint32_t handle = nextFreeHandle;
            assert(handles[handle].type == Type::FREE);
            nextFreeHandle = handles[handle].data.nextFreeHandle;
            return handle;
        }
    }

    bool handleValidFor(uint32_t handle, Type type) {
        return handle < handles.size() && handles[handle].type == type;
    }

public:
    HandleManager() {}
    HandleManager(HandleManager &) = delete;

    /* try to resolve a handle to the corresponding pointer
     * returns no value if given handle is bogus
     * returns true and sets result to the pointer, which may be null if the object the handle was
     * referring to no longer exists */
    optional<shared_ptr<Port>> lookupPort(uint32_t handle) {
        lock_guard lg(lock);
        if (!handleValidFor(handle, Type::PORT)) {
            return {};
        }
        return handles[handle].data.port;
    }

    bool removePort(uint32_t handle) {
        lock_guard lg(lock);
        if (!handleValidFor(handle, Type::PORT)) {
            return false;
        }
        handles[handle] = nextFreeHandle;
        nextFreeHandle = handle;
        return true;
    }

    /* generate a new handle for the given pointer
     * returns no value if handle namespace is full */
    optional<uint32_t> addPort(shared_ptr<Port> &port) {
        lock_guard lg(lock);
        optional<uint32_t> handle = grabFreeHandle();
        if (handle) {
            handles[*handle] = Handle(port);
        }
        return handle;
    }

};

}

using handleManager::HandleManager;

