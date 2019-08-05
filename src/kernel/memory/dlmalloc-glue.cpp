#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <memory/dlmalloc-glue.hpp>


extern "C" void BasicDLMallocPanic(const char *location) {
    cout << "DLMalloc problem occured at: " << location << "\n";
    Panic();
}


extern "C" void *sbrk(ssize_t increment) {
    auto res = Memory::instance()->resizeHeapArea(increment).asPointer<void>();
    return  res;
}
