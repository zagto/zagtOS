#include <syscalls/CreateSharedMemory.hpp>

Result<size_t> CreateSharedMemory(const shared_ptr<Process> &process,
                                  size_t type,
                                  size_t offset,
                                  size_t length,
                                  size_t deviceAddressesPointer,
                                  size_t) {
    length = align(length, PAGE_SIZE, AlignDirection::UP);

    /* TODO: maybe don't hardcode permissions */
    Result<shared_ptr<MemoryArea>> memoryArea;
    switch (type) {
    case 0:
        /* Standard */
        if (offset != 0) {
            cout << "SYS_CREATE_SHARED_MEMORY: offset given for standard shared memory" << endl;
            Panic(); // TODO: exception
        }
        memoryArea = make_shared<MemoryArea>(Permissions::READ_WRITE, length);
        break;
    case 1:
        /* Physical */
        if (!process->canAccessPhysicalMemory()) {
            cout << "SYS_CREATE_SHARED_MEMORY: process which is not allowed to use physical "
                << "memory tried to use it" << endl;
            Panic(); // TODO: exception
        }
        if (offset % PAGE_SIZE != 0) {
            cout << "SYS_CREATE_SHARED_MEMORY: offset not aligned" << endl;
            Panic(); // TODO: exception
        }
        memoryArea = make_shared<MemoryArea>(offset, length);
        break;
    case 2: {
        /* DMA */
        scoped_lock sl(process->pagingLock);
        size_t numPages = length / PAGE_SIZE;

        int frameStack = hos_v1::DMAZone::COUNT - 1;
        while (offset > hos_v1::DMAZoneMax[frameStack]) {
            frameStack--;
            if (frameStack < 0) {
                cout << "SYS_CREATE_SHARED_MEMORY: requested device address ceiling impossible"
                     << "on this architecture." << endl;
                Panic(); // TODO: exception
            }
        }

        vector<size_t> deviceAddresses;
        memoryArea = make_shared<MemoryArea>(frameStack, length, deviceAddresses);
        if (memoryArea) {
            Status status = process->copyToUser(deviceAddressesPointer,
                                                reinterpret_cast<uint8_t *>(deviceAddresses.data()),
                                                numPages * sizeof(size_t),
                                                true);
            if (!status) {
                return status;
            }
        }
        break;
    }
    default:
        cout << "SYS_CREATE_SHARED_MEMORY: got invalid type parameter " << type << endl;
        return Status::BadUserSpace();
    }

    if (!memoryArea) {
        return memoryArea.status();
    }

    Result<uint32_t> handle = process->handleManager.addMemoryArea(*memoryArea);
    if (handle) {
        return *handle;
    } else {
        return handle.status();
    }
}
