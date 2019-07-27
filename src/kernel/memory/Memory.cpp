#include <memory/Memory.hpp>
#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>


extern "C" int init_mparams(void);

Memory::Memory(BootInfo *bootInfo) :
        usedFrameStack{bootInfo->usedFrameStack},
        freshFrameStack{bootInfo->freshFrameStack} {
    init_mparams();
}

PhysicalAddress Memory::allocatePhysicalFrame() {
    if (freshFrameStack.isEmpty()) {
        //Log << "Warning: fresh physical frame stack empty on alloc, trying recycle" << EndLine;
        recyclePhysicalFrame();
    }
    return freshFrameStack.pop();
}

void Memory::freePhysicalFrame(PhysicalAddress address) {
    usedFrameStack.push(address);
}

void Memory::recyclePhysicalFrame() {
    if (usedFrameStack.isEmpty()) {
        Log << "Out of memory." << EndLine;
        Panic();
    }
    PhysicalAddress frame = usedFrameStack.pop();
    memset(frame.identityMapped().asPointer<u8>(), 0, PAGE_SIZE);
    freshFrameStack.push(frame);
}

extern "C" {
    void* dlmalloc(usize);
    void* dlmemalign(usize, usize);
    void* dlrealloc(void*, usize);
    void  dlfree(void*);
}

KernelVirtualAddress Memory::allocateVirtualArea(usize length, usize align) {
    LockHolder lh(heapLock);
    KernelVirtualAddress result;
    if (align > 0) {
        result = KernelVirtualAddress(dlmemalign(align, length));
    } else {
        result = KernelVirtualAddress(dlmalloc(length));
    }
    return result;
}

KernelVirtualAddress Memory::resizeVirtualArea(KernelVirtualAddress address, usize length) {
    LockHolder lh(heapLock);
    return KernelVirtualAddress(dlrealloc(address.asPointer<void>(), length));
}

void Memory::freeVirtualArea(KernelVirtualAddress address) {
    LockHolder lh(heapLock);
    dlfree(address.asPointer<void>());
}


Memory *Memory::instance() {
    return &CurrentSystem.memory;
}

KernelVirtualAddress Memory::resizeHeapArea(isize change) {
    Assert(change >= 0);
    Assert(change % PAGE_SIZE == 0);

    for (usize index = 0; index < change / PAGE_SIZE; index++) {
        MasterPageTable::map(heapEnd + index * PAGE_SIZE,
                             allocatePhysicalFrame(),
                             Permissions::WRITE);
    }
    KernelVirtualAddress oldEnd = heapEnd;
    heapEnd = heapEnd + change;
    return oldEnd;
}
