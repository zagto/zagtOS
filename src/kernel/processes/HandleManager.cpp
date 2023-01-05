#include <common/common.hpp>
#include <processes/HandleManager.hpp>
#include <processes/HandleManager.hpp>
#include <processes/Process.hpp>

namespace handleManager {

HandleManager::HandleManager(Process &process)  :
    process{process} {

    /* some inital handles so we don't have to always resize directly after Process creation */
    numAllocated = 2;
    data = DLMallocGlue.allocate(numAllocated * ELEMENT_SIZE, 0).asPointer<uint8_t>();

    nextFreeHandle = userApi::ZAGTOS_INVALID_HANDLE;
    for (size_t handle = 0; handle < numAllocated; handle++) {
        new (at(handle)) FreeElement(nextFreeHandle);
        nextFreeHandle = handle;
    }
}

HandleManager::HandleManager(Process &process,
                             const hos_v1::Process &handOver,
                             const vector<shared_ptr<Thread>> &allThreads,
                             const vector<shared_ptr<Port>> &allPorts,
                             const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                             const vector<shared_ptr<EventQueue>> &allEventQueues) :
        process{process} {

    uint32_t maxHandle = 0;
    for (size_t index = 0; index < handOver.numHandles; index++) {
        maxHandle = max(handOver.handles[index].handle, maxHandle);
    }

    numAllocated = maxHandle + 1;
    data = DLMallocGlue.allocate(numAllocated * ELEMENT_SIZE, 0).asPointer<uint8_t>();

    vector<bool> initialized(numAllocated, false);

    for (size_t index = 0; index < handOver.numHandles; index++) {
        hos_v1::Handle hosHandle = handOver.handles[index];
        assert(hosHandle.handle < userApi::ZAGTOS_MAX_HANDLES);

        uint32_t handle = hosHandle.handle;
        /* Elements become invalid type on contruction, make sure we don't try to use anything
         * twice */
        assert(!initialized[handle]);
        initialized[handle] = true;

        switch (hosHandle.type) {
        case Type::PORT:
            new (at(handle)) PointerElement(allPorts[hosHandle.objectID]);
            break;
        case Type::REMOTE_PORT: {
            weak_ptr<Port> weak = allPorts[hosHandle.objectID];
            new (at(handle)) PointerElement(weak);
            break;
        }
        case Type::THREAD:
        {
            shared_ptr<Thread> thread = allThreads[hosHandle.objectID];
            AbstractElement *res = new (at(handle)) PointerElement<shared_ptr<Thread>>(thread);
            thread->setHandle(hosHandle.handle);
            assert(dynamic_cast<AbstractPointerElement *>(res));
            assert(dynamic_cast<AbstractPointerElement *>(at(handle)));
            break;
        }
        case Type::MEMORY_AREA:
            new (at(handle)) PointerElement(allMemoryAreas[hosHandle.objectID]);
            break;
        case Type::EVENT_QUEUE:
            new (at(handle)) PointerElement(allEventQueues[hosHandle.objectID]);
            break;
        default:
            cout << "HandleManager: got unknown type form handover" << endl;
            Panic();
        }
    }

    /* Convert remaining elements to FREE type */
    nextFreeHandle = userApi::ZAGTOS_INVALID_HANDLE;
    for (size_t handle = 0; handle < numAllocated; handle++) {
        if (!initialized[handle]) {
            new (at(handle)) FreeElement(nextFreeHandle);
            nextFreeHandle = handle;
        }
    }
}

shared_ptr<Process> HandleManager::sharedProcess() const noexcept {
    return process.self.lock();
}

AbstractElement *HandleManager::at(uint32_t handle) const noexcept {
    assert(handle < numAllocated);
    return reinterpret_cast<AbstractElement *>(data + handle * ELEMENT_SIZE);
}

uint32_t HandleManager::grabFreeNumber() {
    assert(lock.isLocked());
    /* should go out of kernel memory before this happens */
    assert(numAllocated <= userApi::ZAGTOS_MAX_HANDLES);
    static_assert (KERNEL_HEAP_SIZE / ELEMENT_SIZE < userApi::ZAGTOS_MAX_HANDLES);
    /* TODO: maybe introduce a second limit for security, otherwise everyone can kill a process
     * by sending too many handles */

    if (nextFreeHandle == userApi::ZAGTOS_INVALID_HANDLE) {
        /* This is assuming that memcopying a smart pointer element is equivalent to using its move
         * conytructor, which is the case in our implementation. */
        data = DLMallocGlue.resize(data, numAllocated * 2 * ELEMENT_SIZE).asPointer<uint8_t>();
        numAllocated *= 2;
        for (size_t handle = numAllocated/2; handle < numAllocated - 1; handle++) {
            new (at(handle)) FreeElement(nextFreeHandle);
            nextFreeHandle = handle;
        }
        return numAllocated - 1;
    } else {
        uint32_t handle = nextFreeHandle;
        FreeElement *element = dynamic_cast<FreeElement *>(at(handle));
        nextFreeHandle = element->nextFreeHandle;
        element->~FreeElement();
        return handle;
    }
}

/* Returns true if the handle was removed successfully. If number is an in-use handle, this call
 * always succeeds. Otherwise it returns false.
 * If the removed handle was referring to a local Thread (a Thread of the Process associated with
 * this handleManager, not a REMOTE_THREAD type), removing it's handle leads to thread termination,
 * which requires further action by the caller. Such a thread is returned in the removedThread
 * argument */
void HandleManager::_removeHandle(uint32_t handle,
                                  shared_ptr<Thread> &removedThread,
                                  shared_ptr<Port> &removedPort) {
    assert(lock.isLocked());
    if (!handleValid(handle)) {
        throw BadUserSpace(sharedProcess());
    }

    AbstractElement *element = at(handle);
    auto threadElement = dynamic_cast<PointerElement<shared_ptr<Thread>> *>(element);
    if (threadElement) {
        removedThread = move(threadElement->pointer);
    }
    auto portElement = dynamic_cast<PointerElement<shared_ptr<Port>> *>(element);
    if (portElement) {
        removedPort = move(portElement->pointer);
    }
    element->~AbstractElement();
    new (element) FreeElement(nextFreeHandle);
    nextFreeHandle = handle;
}

void HandleManager::removeHandle(uint32_t number,
                                 shared_ptr<Thread> &removedThread,
                                 shared_ptr<Port> &removedPort) {
    scoped_lock sl(lock);
    _removeHandle(number, removedThread, removedPort);
}

bool HandleManager::handleValid(uint32_t handle) const noexcept {
    assert(lock.isLocked());
    return handle < numAllocated && dynamic_cast<AbstractPointerElement *>(at(handle));
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

            if (!handleValid(sourceHanlde)) {
                cout << "transferHandles: attempt to transfer non-existing handle." << endl;
                throw BadUserSpace(process.self.lock());
            }

            auto source = dynamic_cast<AbstractPointerElement *>(at(sourceHanlde));
            uint32_t destinationHandle = destination.grabFreeNumber();

            /* Ports become RemotePorts at the destination */
            /* TODO: also handle Threads specially here, or even better, get rid of the handle
             * type as indication of ownership */
            auto portSource = dynamic_cast<PointerElement<shared_ptr<Port>> *>(source);
            if (portSource) {
                new (destination.at(destinationHandle)) PointerElement<weak_ptr<Port>>(weak_ptr<Port>(portSource->pointer));
            } else {
                source->constructCopyAt(destination.at(destinationHandle));
            }
            handleValues[transferIndex] = destinationHandle;

            sl.checkWaiters();
        }
    } catch (...) {
        /* on error, destroy any newly created handles */
        for (size_t destroyIndex = 0; destroyIndex < transferIndex; destroyIndex++) {
            shared_ptr<Thread> dummy;
            shared_ptr<Port> dummy2;
            try {
                _removeHandle(handleValues[destroyIndex], dummy, dummy2);
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
    for (size_t handle = 0; handle < numAllocated; handle++) {
        auto threadElement = dynamic_cast<PointerElement<shared_ptr<Thread>> *>(at(handle));
        if (threadElement) {
            threadElement->pointer->process = process;
        }
        auto eventQueueElement = dynamic_cast<PointerElement<shared_ptr<EventQueue>> *>(at(handle));
        if (eventQueueElement) {
            eventQueueElement->pointer->process = process;
        }
    }
}


}
