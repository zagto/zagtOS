#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <memory/dlmalloc-glue.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <paging/PagingContext.hpp>


#define MFAIL reinterpret_cast<void *>(-1ull)

DLMallocStatusOptions DLMallocStatus = DLMallocStatusOptions::OK;


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
    if (result.status() == Status::OK()) {
        DLMallocStatus = DLMallocStatusOptions::OK;
        return *result;
    } else if (result.status() == Status::OutOfKernelHeap()) {
        DLMallocStatus = DLMallocStatusOptions::OutOfKernelHeap;
        return MFAIL;
    } else if (result.status() == Status::OutOfMemory()) {
        DLMallocStatus = DLMallocStatusOptions::OutOfPhysicalMemory;
        return MFAIL;
    } else {
        cout << "DLMallocGlue: KernelPageAllocator returned unexpected status" << endl;
        Panic();
    }
}
