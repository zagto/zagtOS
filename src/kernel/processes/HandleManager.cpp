#include <common/common.hpp>
#include <processes/HandleManager.hpp>
#include <processes/Process.hpp>

namespace handleManager {

// TODO: naming different things differntly

Element::Element(uint32_t next) noexcept {
    type = Type::FREE;
    data.nextFreeNumber = next;
}

Element::Element(shared_ptr<Port> &port) noexcept {
    type = Type::PORT;
    new (&data.port) shared_ptr<Port>(port);
}

Element::Element(weak_ptr<Port> &port) noexcept {
    type = Type::REMOTE_PORT;
    new (&data.remotePort) weak_ptr<Port>(port);
}

Element::Element(shared_ptr<Thread> &thread) noexcept {
    type = Type::THREAD;
    new (&data.thread) shared_ptr<Thread>(thread);
}

Element::Element(shared_ptr<MemoryArea> &memoryArea) noexcept {
    type = Type::MEMORY_AREA;
    new (&data.memoryArea) shared_ptr<MemoryArea>(memoryArea);
}

Element::Element(const Element &&other) noexcept {
    type = other.type;
    switch (type) {
    case Type::INVALID:
        break;
    case Type::FREE:
        data.nextFreeNumber = other.data.nextFreeNumber;
        break;
    case Type::PORT:
        new (&data.port) shared_ptr<Port>(move(other.data.port));
        break;
    case Type::REMOTE_PORT:
        new (&data.remotePort) weak_ptr<Port>(move(other.data.remotePort));
        break;
    case Type::THREAD:
        new (&data.thread) shared_ptr<Thread>(move(other.data.thread));
        break;
    case Type::MEMORY_AREA:
        new (&data.memoryArea) shared_ptr<MemoryArea>(move(other.data.memoryArea));
        break;
    }
}

Element::~Element() {
    destructData();
}

void Element::operator=(const Element &&other) noexcept {
    destructData();
    new (this) Element(move(other));
}

void Element::destructData() noexcept {
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

HandleManager::HandleManager(Process &process,
                             const hos_v1::Process &handOver,
                             const vector<shared_ptr<Thread>> &allThreads,
                             const vector<shared_ptr<Port>> &allPorts,
                             const vector<shared_ptr<MemoryArea> > &allMemoryAreas) :
        process{process} {

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
    /* should go out of kernel memory before this happens */
    assert(elements.size() < NUMBER_END);
    static_assert (KERNEL_HEAP_SIZE / sizeof(Element) < NUMBER_END);
    /* TODO: maybe introduce a second limit for security, otherwise everyone can kill a process
     * by sending too many handles */

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

bool HandleManager::handleValidFor(uint32_t number, Type type) noexcept {
    return number > 0 && number < elements.size() && elements[number].type == type;
}

/* Checks if this number is currently a valid handle to any object */
bool HandleManager::handleValid(uint32_t number) noexcept {
    return number > 0
            && number < elements.size()
            && elements[number].type != Type::FREE
            && elements[number].type != Type::INVALID;
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
shared_ptr<Port> HandleManager::lookupPort(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::PORT)) {
        cout << "lookupPort: invalid port handle " << number << endl;
        throw BadUserSpace(process.self.lock());
    }
    return elements[number].data.port;
}

weak_ptr<Port> HandleManager::lookupRemotePort(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::REMOTE_PORT)) {
        cout << "lookupRemotePort: invalid port handle " << number << endl;
        throw BadUserSpace(process.self.lock());
    }
    return elements[number].data.remotePort;
}

shared_ptr<Thread> HandleManager::lookupThread(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::THREAD)) {
        cout << "lookupThread: invalid port handle " << number << endl;
        throw BadUserSpace(process.self.lock());
    }
    return elements[number].data.thread;
}

shared_ptr<MemoryArea> HandleManager::lookupMemoryArea(uint32_t number) {
    scoped_lock sl(lock);
    if (!handleValidFor(number, Type::MEMORY_AREA)) {
        cout << "lookupThread: invalid MemoryArea handle " << number << endl;
        throw BadUserSpace(process.self.lock());
    }
    return elements[number].data.memoryArea;
}


/* Returns true if the handle was removed successfully. If number is an in-use handle, this call
 * always succeeds. Otherwise it returns false.
 * If the removed handle was referring to a local Thread (a Thread of the Process associated with
 * this handleManager, not a REMOTE_THREAD type), removing it's handle leads to thread termination,
 * which requires further action by the caller. Such a thread is returned in the removedThread
 * argument */
void HandleManager::_removeHandle(uint32_t number, shared_ptr<Thread> &removedThread) {
    if (number == 0 || elements[number].type == Type::FREE) {
        throw BadUserSpace(process.self.lock());
    }
    assert(elements[number].type != Type::INVALID);

    if (elements[number].type == Type::THREAD) {
        cout << "removed thread" << endl;
        removedThread = move(elements[number].data.thread);
    } else {
        removedThread = {};
    }

    elements[number] = Element(nextFreeNumber);
    nextFreeNumber = number;
}

void HandleManager::removeHandle(uint32_t number, shared_ptr<Thread> &removedThread) {
    scoped_lock sl(lock);
    _removeHandle(number, removedThread);
}

/* creates handles in destination for all objects referenced by the initial values handleValues.
 * On success, handleValues will contain the new destination handle numbers. In case of failure,
 * these that were not created will have the number handleManager::NUMBER_END. */
void HandleManager::transferHandles(vector<uint32_t> &handleValues,
                                    HandleManager &destination) {
    scoped_lock sl(lock, destination.lock);

    size_t transferIndex;
    try {
        for (transferIndex = 0; transferIndex < handleValues.size(); transferIndex++) {
            uint32_t sourceHanlde = handleValues[transferIndex];
            uint32_t result;

            if (!handleValid(sourceHanlde)) {
                cout << "transferHandles: attempt to transfer non-existing handle." << endl;
                throw BadUserSpace(process.self.lock());
            }

            /* Check each possible handle type as they all need to be treated differently */
            switch(elements[sourceHanlde].type) {
            case Type::PORT: {
                weak_ptr<Port> port(elements[sourceHanlde].data.port);
                result = destination._addRemotePort(port);
                break;
            }
            case Type::REMOTE_PORT: {
                weak_ptr<Port> port = elements[sourceHanlde].data.remotePort;
                result = destination._addRemotePort(port);
                break;
            }
            case Type::MEMORY_AREA: {
                shared_ptr<MemoryArea> memoryArea(elements[sourceHanlde].data.memoryArea);
                result = destination._addMemoryArea(memoryArea);
                break;
            }
            default:
                cout << "TODO: non-implemented handle type in transferHandles" << endl;
                Panic();
            }
            handleValues[transferIndex] = result;

            sl.checkWaiters();
        }
    } catch (...) {
        /* on error, destroy any newly created handles */
        for (size_t destroyIndex = 0; destroyIndex < transferIndex; destroyIndex++) {
            shared_ptr<Thread> dummy;
            try {
                _removeHandle(handleValues[destroyIndex], dummy);
            } catch(...) {
                cout << "transferHandles: destination process has messed with handles that were "
                     << "not even passed to it yet and deserves to crash." << endl;
                /* removing handles should only fail if invalid handle numbers are passed */
            }
            sl.checkWaiters();
        }
        throw;
    }
}

void HandleManager::insertAllProcessPointersAfterKernelHandover(const shared_ptr<Process> &process) noexcept {
    for (Element &element: elements) {
        if (element.type == Type::THREAD) {
            element.data.thread->process = process;
        } else if (element.type == Type::PORT) {
            element.data.port->process = process;
        }
    }
}


}
