#define _GNU_SOURCE 1
#include <zagtos/SharedMemory.hpp>
#include <zagtos/Align.hpp>
#include <zagtos/syscall.h>
#include <sys/mman.h>
#include <climits>

namespace zagtos {

enum SharedType {
    STANDARD = 0,
    PHYSICAL = 1,
    DMA = 2,
};

std::tuple<SharedMemory, std::vector<size_t>> SharedMemory::DMA(size_t deviceMax, size_t length) {
    size_t numPages = zagtos::align(length, PAGE_SIZE, zagtos::AlignDirection::UP) / PAGE_SIZE;
    std::vector<size_t> deviceAddresses(numPages);
    SharedMemory shm;
    shm._handle = static_cast<uint32_t>(
                zagtos_syscall4(SYS_CREATE_SHARED_MEMORY,
                                SharedType::DMA,
                                static_cast<size_t>(deviceMax),
                                length,
                                reinterpret_cast<size_t>(deviceAddresses.data())));
    return {std::move(shm), std::move(deviceAddresses)};
}

SharedMemory SharedMemory::Physical(size_t physicalAddress, size_t length) {
    SharedMemory shm;
    shm._handle = static_cast<uint32_t>(
                zagtos_syscall4(SYS_CREATE_SHARED_MEMORY,
                                SharedType::PHYSICAL,
                                physicalAddress,
                                length,
                                0));
    return shm;
}

SharedMemory SharedMemory::Standard(size_t length) {
    SharedMemory shm;
    shm._handle = static_cast<uint32_t>(
                zagtos_syscall4(SYS_CREATE_SHARED_MEMORY,
                                SharedType::STANDARD,
                                0,
                                length,
                                0));
    return shm;
}

void SharedMemory::operator=(SharedMemory &&other) {
    if (_handle != INVALID_HANDLE) {
        zagtos_syscall1(SYS_DELETE_HANDLE, _handle);
    }
    _handle = other._handle;
    other._handle = INVALID_HANDLE;
}

void *SharedMemory::_map(int protection) {
    assert(_handle != INVALID_HANDLE);
    return mmap(nullptr, 0, protection, MAP_SHARED|MAP_WHOLE, _handle, 0);
}

void UnmapWhole(void *pointer) {
    size_t ret = zagtos_syscall3(SYS_MUNMAP, 1, reinterpret_cast<size_t>(pointer), 0);
    assert(ret == 0);
}

}
