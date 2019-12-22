#include <memory/Memory.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>


extern "C" int init_mparams(void);

Memory::Memory(BootInfo *bootInfo) :
        usedFrameStack{bootInfo->usedFrameStack},
        freshFrameStack{bootInfo->freshFrameStack} {
    init_mparams();
}

PhysicalAddress Memory::allocatePhysicalFrame() {
    lock_guard lg(frameManagementLock);
    if (freshFrameStack.isEmpty()) {
        //Log << "Warning: fresh physical frame stack empty on alloc, trying recycle" << EndLine;
        recyclePhysicalFrame();
    }
    return freshFrameStack.pop();
}

void Memory::freePhysicalFrame(PhysicalAddress address) {
    lock_guard lg(frameManagementLock);
    usedFrameStack.push(address);
}

void Memory::recyclePhysicalFrame() {
    if (usedFrameStack.isEmpty()) {
        cout << "Out of memory." << endl;
        Panic();
    }
    PhysicalAddress frame = usedFrameStack.pop();
    memset(frame.identityMapped().asPointer<uint8_t>(), 0, PAGE_SIZE);
    freshFrameStack.push(frame);
}

extern "C" {
    void* dlmalloc(size_t);
    void* dlmemalign(size_t, size_t);
    void* dlrealloc(void*, size_t);
    void  dlfree(void*);
}

KernelVirtualAddress Memory::allocateVirtualArea(size_t length, size_t align) {
    lock_guard lg(heapLock);
    KernelVirtualAddress result;
    if (align > 0) {
        result = KernelVirtualAddress(dlmemalign(align, length));
    } else {
        result = KernelVirtualAddress(dlmalloc(length));
    }
    return result;
}

KernelVirtualAddress Memory::resizeVirtualArea(KernelVirtualAddress address, size_t length) {
    lock_guard lg(heapLock);
    return KernelVirtualAddress(dlrealloc(address.asPointer<void>(), length));
}

void Memory::freeVirtualArea(KernelVirtualAddress address) {
    lock_guard lg(heapLock);
    dlfree(address.asPointer<void>());
}


Memory *Memory::instance() {
    return &CurrentSystem.memory;
}

KernelVirtualAddress Memory::resizeHeapArea(ssize_t change) {
    assert(change % PAGE_SIZE == 0);
    assert(heapLock.isLocked());

    if (change == 0) {
        return heapEnd;
    } else if (change > 0) {
        for (size_t index = 0; index < change / PAGE_SIZE; index++) {
            lock_guard lg(kernelPagingLock);
            PagingContext::map(heapEnd + index * PAGE_SIZE,
                               allocatePhysicalFrame(),
                               Permissions::WRITE,
                               false);
        }
    } else {
        for (size_t index = 1; index <= static_cast<size_t>(-change) / PAGE_SIZE; index++) {
            lock_guard lg(kernelPagingLock);
            PhysicalAddress frame = PagingContext::resolve(heapEnd - index * PAGE_SIZE);
            PagingContext::unmap(heapEnd - index * PAGE_SIZE);
            freePhysicalFrame(frame);
        }
    }
    KernelVirtualAddress oldEnd = heapEnd;
    heapEnd = heapEnd + change;
    return oldEnd;
}
