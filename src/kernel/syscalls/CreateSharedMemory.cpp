#include <syscalls/CreateSharedMemory.hpp>

size_t CreateSharedMemory(const shared_ptr<Process> &process,
                                  size_t type,
                                  size_t offset,
                                  size_t length,
                                  size_t deviceAddressesPointer,
                                  size_t) {
    length = align(length, PAGE_SIZE, AlignDirection::UP);

    /* TODO: maybe don't hardcode permissions */
    shared_ptr<MemoryArea> memoryArea;
    switch (type) {
    case 0:
        /* Standard */
        if (offset != 0) {
            cout << "SYS_CREATE_SHARED_MEMORY: offset given for standard shared memory" << endl;
            Panic(); // TODO: exception
        }
        memoryArea = make_shared<MemoryArea>(true, Permissions::READ_WRITE, length);
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
        memoryArea = make_shared<MemoryArea>(PhysicalAddress(offset), length);
        break;
    case 2: {
        /* DMA */
        size_t numPages = length / PAGE_SIZE;

        int zoneID = hos_v1::DMAZone::COUNT - 1;
        while (offset > hos_v1::DMAZoneMax[zoneID]) {
            zoneID--;
            if (zoneID < 0) {
                cout << "SYS_CREATE_SHARED_MEMORY: requested device address ceiling impossible"
                     << "on this architecture." << endl;
                throw BadUserSpace(process);
            }
        }

        vector<size_t> deviceAddresses;
        memoryArea = make_shared<MemoryArea>(zoneID, length, deviceAddresses);
        if (memoryArea) {
            process->addressSpace.copyTo(deviceAddressesPointer,
                                         reinterpret_cast<uint8_t *>(deviceAddresses.data()),
                                         numPages * sizeof(size_t),
                                         true);
        }
        break;
    }
    default:
        cout << "SYS_CREATE_SHARED_MEMORY: got invalid type parameter " << type << endl;
        throw BadUserSpace(process);
    }

    return process->handleManager.addMemoryArea(memoryArea);
}
