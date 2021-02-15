#include <memory/Memory.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>


extern "C" int init_mparams(void);

Memory::Memory(const hos_v1::System &handOver) {
    for (size_t stackIndex = 0; stackIndex < NUM_STACKS; stackIndex++) {
        usedFrameStack[stackIndex] = handOver.usedFrameStack[stackIndex];
        freshFrameStack[stackIndex] = handOver.freshFrameStack[stackIndex];
    }
    init_mparams();
}

Result<PhysicalAddress> Memory::allocatePhysicalFrame(size_t maxStack) {
    scoped_lock lg(frameManagementLock);
    assert(maxStack < hos_v1::DMAZone::COUNT);

    for (int stackIndex = maxStack; stackIndex >= 0; stackIndex--) {
        if (!freshFrameStack[stackIndex].isEmpty() || !usedFrameStack[stackIndex].isEmpty()) {
            if (freshFrameStack[stackIndex].isEmpty()) {
                recyclePhysicalFrame(stackIndex);
            }
            return freshFrameStack[stackIndex].pop();
        }
    }
    cout << "out of memory" << endl;
    Panic();
}

void Memory::freePhysicalFrame(PhysicalAddress address) {
    scoped_lock lg(frameManagementLock);
    for (size_t stackIndex = 0; stackIndex < NUM_STACKS; stackIndex++) {
        if (address.value() <= hos_v1::DMAZoneMax[stackIndex]) {
            usedFrameStack[stackIndex].push(address);
            return;
        }
    }
    cout << "should not reach here" << endl;
    Panic();
}

void Memory::recyclePhysicalFrame(size_t stackIndex) {
    PhysicalAddress frame = usedFrameStack[stackIndex].pop();
    memset(frame.identityMapped().asPointer<uint8_t>(), 0, PAGE_SIZE);
    freshFrameStack[stackIndex].push(frame);
}

extern "C" {
    void* dlmalloc(size_t);
    void* dlmemalign(size_t, size_t);
    void* dlrealloc(void*, size_t);
    void  dlfree(void*);
}

Result<KernelVirtualAddress> Memory::allocateVirtualArea(size_t length, size_t align) {
    scoped_lock lg(heapLock);
    KernelVirtualAddress result;
    if (align > 0) {
        result = KernelVirtualAddress(dlmemalign(align, length));
    } else {
        result = KernelVirtualAddress(dlmalloc(length));
    }
    return result;
}

Result<KernelVirtualAddress> Memory::resizeVirtualArea(KernelVirtualAddress address, size_t length) {
    scoped_lock lg(heapLock);
    return KernelVirtualAddress(dlrealloc(address.asPointer<void>(), length));
}

void Memory::freeVirtualArea(KernelVirtualAddress address) {
    scoped_lock lg(heapLock);
    dlfree(address.asPointer<void>());
}


Memory *Memory::instance() {
    return &CurrentSystem.memory;
}
