#include <common/common.hpp>
#include <processes/HandleManager.hpp>
#include <processes/Process.hpp>

namespace handleManager {

// TODO: naming different things differntly

Element::Element(uint32_t next) {
    type = Type::FREE;
    data.nextFreeNumber = next;
}

Element::Element(shared_ptr<Port> &port) {
    type = Type::PORT;
    new (&data.port) shared_ptr<Port>(port);
}

Element::Element(weak_ptr<Port> &port) {
    type = Type::REMOTE_PORT;
    new (&data.remotePort) weak_ptr<Port>(port);
}

Element::Element(shared_ptr<Thread> &thread) {
    type = Type::THREAD;
    new (&data.thread) shared_ptr<Thread>(thread);
}

Element::Element(shared_ptr<MemoryArea> &memoryArea) {
    type = Type::MEMORY_AREA;
    new (&data.memoryArea) shared_ptr<MemoryArea>(memoryArea);
}

Element::Element(const Element &other) {
    type = other.type;
    switch (type) {
    case Type::INVALID:
        break;
    case Type::FREE:
        data.nextFreeNumber = other.data.nextFreeNumber;
        break;
    case Type::PORT:
        new (&data.port) shared_ptr<Port>(other.data.port);
        break;
    case Type::REMOTE_PORT:
        new (&data.remotePort) weak_ptr<Port>(other.data.remotePort);
        break;
    case Type::THREAD:
        new (&data.thread) shared_ptr<Thread>(other.data.thread);
        break;
    case Type::MEMORY_AREA:
        new (&data.memoryArea) shared_ptr<MemoryArea>(other.data.memoryArea);
        break;
    }
}

Element::~Element() {
    destructData();
}

void Element::operator=(const Element &other) {
    destructData();
    new (this) Element(other);
}

void Element::destructData() {
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
    case Type::THREAD:
        data.thread.~shared_ptr();
        break;
    case Type::MEMORY_AREA:
        data.memoryArea.~shared_ptr();
        break;
    }
    type = Type::INVALID;
}

HandleManager::HandleManager(const hos_v1::Process &handOver,
              const vector<shared_ptr<Thread>> &allThreads,
              const vector<shared_ptr<Port>> &allPorts,
              const vector<shared_ptr<MemoryArea> > &allMemoryAreas) {
    uint32_t maxHandle = 0;
    for (size_t index = 0; index < handOver.numHandles; index++) {
        maxHandle = max(handOver.handles[index].handle, maxHandle);
    }

    elements.resize(maxHandle + 1);
    for (size_t index = 0; index < handOver.numHandles; index++) {
        hos_v1::Handle hosHandle = handOver.handles[index];
        assert(hosHandle.handle != 0);

        Element &element = elements[hosHandle.handle];
        /* Elements become invalid type on contruction, make sure we don't try to use anything
         * twice */
        assert(element.type == Type::INVALID);

        switch (hosHandle.type) {
        case Type::PORT:
            element = Element(allPorts[hosHandle.objectID]);
            break;
        case Type::REMOTE_PORT: {
            weak_ptr<Port> weak = allPorts[hosHandle.objectID];
            element = Element(weak);
            break;
        }
        case Type::THREAD:
        {
            shared_ptr<Thread> thread = allThreads[hosHandle.objectID];
            element = Element(thread);
            thread->setHandle(hosHandle.handle);
            break;
        }
        case Type::MEMORY_AREA:
            element = Element(allMemoryAreas[hosHandle.objectID]);
            break;
        default:
            /* Could never ever reach this */
            Panic();
        }
    }

    /* Convert remaining elements to FREE type */
    nextFreeNumber = NUMBER_END;
    for (size_t index = 1; index < elements.size(); index++) {
        if (elements[index].type == Type::INVALID) {
            elements[index] = Element(nextFreeNumber);
            nextFreeNumber = index;
        }
    }
}

uint32_t HandleManager::grabFreeNumber() {
    /* should go out of kernel memory way before this happens */
    assert(elements.size() < NUMBER_END);

    if (nextFreeNumber == NUMBER_END) {
        /* Avoid giving out handle 0. This is because handles of Zagtos may be used like Linux
         * Thread IDs, where the handle expected to be non-zero */
        elements.resize(elements.size() > 0 ? elements.size() + 1 : 2);
        return static_cast<uint32_t>(elements.size() - 1);
    } else {
        uint32_t handle = nextFreeNumber;
        assert(elements[handle].type == Type::FREE);
        nextFreeNumber = elements[handle].data.nextFreeNumber;
        return handle;
    }
}

bool HandleManager::handleValidFor(uint32_t number, Type type) {
    return number > 0 && number < elements.size() && elements[number].type == type;
}

/* generate a new handle for the given pointer */
uint32_t HandleManager::addPort(shared_ptr<Port> &port) {
    scoped_lock sl(lock);

    uint32_t handle = grabFreeNumber();
    elements[handle] = Element(port);
    return handle;
}

uint32_t HandleManager::addThread(shared_ptr<Thread> &thread) {
    scoped_lock sl(lock);

    uint32_t handle = grabFreeNumber();
    elements[handle] = Element(thread);
    thread->setHandle(handle);
    return handle;
}

uint32_t HandleManager::addMemoryArea(shared_ptr<MemoryArea> &memoryArea) {
    scoped_lock sl(lock);
    return _addMemoryArea(memoryArea);
}


/* unlike the above, this is for internal use and expects the lock to be already hold. Nobody else
 * should create their own remote ports as they wouldn't be remote. */
uint32_t HandleManager::_addRemotePort(weak_ptr<Port> &remotePort) {
    assert(lock.isLocked());

    uint32_t handle = grabFreeNumber();
    elements[handle] = Element(remotePort);
    return handle;
}

uint32_t HandleManager::_addMemoryArea(shared_ptr<MemoryArea> &memoryArea) {
    assert(lock.isLocked());

    uint32_t handle = grabFreeNumber();
    elements[handle] = Element(memoryArea);
    return handle;
}

/* try to resolve a handle to the corresponding pointer
 * returns no value if given handle is bogus
 * returns true and sets result to the pointer, which may be null if the object the handle was
 * referring to no longer exists */
optional<shared_ptr<Port>> HandleManager::lookupPort(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::PORT)) {
        return {};
    }
    return elements[number].data.port;
}

optional<weak_ptr<Port>> HandleManager::lookupRemotePort(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::REMOTE_PORT)) {
        return {};
    }
    return elements[number].data.remotePort;
}

optional<shared_ptr<Thread>> HandleManager::lookupThread(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::THREAD)) {
        return {};
    }
    return elements[number].data.thread;
}

optional<shared_ptr<MemoryArea>> HandleManager::lookupMemoryArea(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::MEMORY_AREA)) {
        return {};
    }
    return elements[number].data.memoryArea;
}


shared_ptr<Thread> HandleManager::extractThread() {
    scoped_lock sl(lock);
    for (uint32_t number = 0; number < elements.size(); number++) {
        if (elements[number].type == Type::THREAD) {
            shared_ptr<Thread> result;
            _removeHandle(number, result);
            return result;
        }
    }
    return {};
}

/* Returns true if the handle was removed successfully. If number is an in-use handle, this call
 * always succeeds. Otherwise it returns false.
 * If the removed handle was referring to a local Thread (a Thread of the Process associated with
 * this handleManager, not a REMOTE_THREAD type), removing it's handle leads to thread termination,
 * which requires further action by the caller. Such a thread is returned in the removedThread
 * argument */
bool HandleManager::_removeHandle(uint32_t number, shared_ptr<Thread> &removedThread) {
    if (number == 0 || elements[number].type == Type::FREE) {
        return false;
    }
    assert(elements[number].type != Type::INVALID);

    if (elements[number].type == Type::THREAD) {
        cout << "removed thread" << endl;
        removedThread = move(elements[number].data.thread);
    } else {
        removedThread = {};
    }

    elements[number] = nextFreeNumber;
    nextFreeNumber = number;
    return true;
}

bool HandleManager::removeHandle(uint32_t number, shared_ptr<Thread> &removedThread) {
    scoped_lock sl(lock);
    return _removeHandle(number, removedThread);
}

bool HandleManager::transferHandles(vector<uint32_t> &handleValues,
                                    HandleManager &destination) {
    scoped_lock sl(lock, destination.lock);

    for (uint32_t &handle: handleValues) {
        if (handle == 0 || handle >= elements.size() || elements[handle].type == Type::FREE) {
            cout << "transferHandles: attempt to transfer non-existing handle." << endl;
            return false;
        }

        /* Check each possible handle type as they all need to be treated differently */
        switch(elements[handle].type) {
        case Type::PORT: {
            weak_ptr<Port> port(elements[handle].data.port);
            handle = destination._addRemotePort(port);
            break;
        }
        case Type::REMOTE_PORT: {
            weak_ptr<Port> port = elements[handle].data.remotePort;
            handle = destination._addRemotePort(port);
            break;
        }
        case Type::MEMORY_AREA: {
            shared_ptr<MemoryArea> memoryArea(elements[handle].data.memoryArea);
            handle = destination._addMemoryArea(memoryArea);
            break;
        }
        default:
            cout << "non-implemented handle type in transferHandles" << endl;
            Panic();
        }
    }
    return true;
}

/*void HandleManager::removeAllHandles() {
    scoped_lock sl(lock);
    handles.resize(0);
    nextFreeHandle = 0;
}*/

void HandleManager::insertAllProcessPointersAfterKernelHandover(const shared_ptr<Process> &process) {
    for (Element &element: elements) {
        if (element.type == Type::THREAD) {
            element.data.thread->process = process;
        } else if (element.type == Type::PORT) {
            element.data.port->process = process;
        }
    }
}


}
