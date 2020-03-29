#include <common/common.hpp>
#include <processes/HandleManager.hpp>

namespace handleManager {

Handle::Handle(uint32_t next) {
    type = Type::FREE;
    data.nextFreeHandle = next;
}

Handle::Handle(shared_ptr<Port> &port) {
    type = Type::PORT;
    new (&data.port) shared_ptr<Port>(port);
}

Handle::Handle(weak_ptr<Port> &port) {
    type = Type::REMOTE_PORT;
    new (&data.remotePort) weak_ptr<Port>(port);
}

Handle::Handle(const Handle &other) {
    type = other.type;
    switch (type) {
    case Type::INVALID:
        break;
    case Type::FREE:
        data.nextFreeHandle = other.data.nextFreeHandle;
        break;
    case Type::PORT:
        new (&data.port) shared_ptr<Port>(other.data.port);
        break;
    case Type::REMOTE_PORT:
        new (&data.remotePort) weak_ptr<Port>(other.data.remotePort);
        break;
    }
}

Handle::~Handle() {
    destructData();
}

void Handle::operator=(const Handle &other) {
    destructData();
    new (this) Handle(other);
}

void Handle::destructData() {
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


uint32_t HandleManager::grabFreeHandle() {
    /* should go out of kernel memory way before this happens */
    assert(handles.size() < HANDLE_END);

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

bool HandleManager::handleValidFor(uint32_t handle, Type type) {
    return handle < handles.size() && handles[handle].type == type;
}

/* generate a new handle for the given pointer */
uint32_t HandleManager::addPort(shared_ptr<Port> &port) {
    scoped_lock sl(lock);

    uint32_t handle = grabFreeHandle();
    handles[handle] = Handle(port);
    return handle;
}

/* unlike the above, this is for internal use and expects the lock to be already hold. Nobody else
 * should create their own remote ports as they wouldn't be remote. */
uint32_t HandleManager::_addRemotePort(weak_ptr<Port> &port) {
    assert(lock.isLocked());

    uint32_t handle = grabFreeHandle();
    handles[handle] = Handle(port);
    return handle;
}

/* try to resolve a handle to the corresponding pointer
 * returns no value if given handle is bogus
 * returns true and sets result to the pointer, which may be null if the object the handle was
 * referring to no longer exists */
optional<shared_ptr<Port>> HandleManager::lookupPort(uint32_t handle) {
    scoped_lock sl(lock);
    if (!handleValidFor(handle, Type::PORT)) {
        return {};
    }
    return handles[handle].data.port;
}

optional<weak_ptr<Port>> HandleManager::lookupRemotePort(uint32_t handle) {
    scoped_lock sl(lock);
    if (!handleValidFor(handle, Type::PORT)) {
        return {};
    }
    return handles[handle].data.remotePort;
}

bool HandleManager::removeHandle(uint32_t handle) {
    scoped_lock sl(lock);
    if (handles[handle].type == Type::FREE) {
        return false;
    }
    assert(handles[handle].type != Type::INVALID);

    handles[handle] = nextFreeHandle;
    nextFreeHandle = handle;
    return true;
}

bool HandleManager::transferHandles(vector<uint32_t> &handleValues,
                                    HandleManager &destination) {
    scoped_lock sl(lock, destination.lock);

    for (uint32_t handle: handleValues) {
        optional<uint32_t> result;

        if (handle >= handles.size() || handles[handle].type == Type::FREE) {
            cout << "transferHandles: attempt to transfer non-existing handle." << endl;
            return false;
        }

        /* Check each possible handle type as they all need to be treated differently */
        switch(handles[handle].type) {
        case Type::PORT: {
            weak_ptr<Port> port(handles[handle].data.port);
            result = destination._addRemotePort(port);
            break;
        }
        case Type::REMOTE_PORT: {
            weak_ptr<Port> port = handles[handle].data.remotePort;
            result = destination._addRemotePort(port);
            break;
        }
        default:
            cout << "Should never go here" << endl;
            Panic();
        }
        handle = *result;
    }
    return true;
}


}
