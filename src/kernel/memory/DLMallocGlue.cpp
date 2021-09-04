#include <common/common.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <interrupts/KernelInterruptsLock.hpp>


extern "C" {
    int init_mparams(void);
    void* dlmalloc(size_t);
    void* dlmemalign(size_t, size_t);
    void* dlrealloc(void*, size_t);
    void  dlfree(void*);
}


#define MFAIL reinterpret_cast<void *>(-1ull)

static Status DLMallocStatus = Status::OK();


namespace dlMallocGlue {

Glue::Glue() {
    init_mparams();
}

Result<KernelVirtualAddress> Glue::allocate(size_t length, size_t align) {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(KernelPageAllocator.lock);
    DLMallocStatus = Status::OK();
    void *rawAddress;

    if (align > 0) {
        rawAddress = dlmemalign(align, length);
    } else {
        rawAddress = dlmalloc(length);
    }

    if (rawAddress == nullptr) {
        assert(DLMallocStatus != Status::OK());
        return DLMallocStatus;
    }
    return KernelVirtualAddress(rawAddress);
}

Result<KernelVirtualAddress> Glue::resize(KernelVirtualAddress address, size_t length) {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(KernelPageAllocator.lock);
    DLMallocStatus = Status::OK();

    void *rawAddress = dlrealloc(address.asPointer<void>(), length);

    if (rawAddress == nullptr) {
        assert(DLMallocStatus != Status::OK());
        return DLMallocStatus;
    }
    return KernelVirtualAddress(rawAddress);
}

void Glue::free(KernelVirtualAddress address) {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(KernelPageAllocator.lock);
    dlfree(address.asPointer<void>());
}

}

extern "C" void BasicDLMallocPanic(const char *location) {
    cout << "DLMalloc problem occured at: " << location << "\n";
    Panic();
}

extern "C" int KernelMUnmap(void *address, size_t length) {
    KernelPageAllocator.unmap(address, length, true);
    return 0;
}

extern "C" void *KernelMMap(size_t length) {
    Result result = KernelPageAllocator.map(length, true);
    if (result) {
        DLMallocStatus = Status::OK();
        return *result;
    } else {
        DLMallocStatus = result.status();
        return MFAIL;
    }
}
