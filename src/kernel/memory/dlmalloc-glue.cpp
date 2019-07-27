#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <memory/dlmalloc-glue.hpp>


extern "C" void BasicDLMallocPanic(const char *location) {
    cout << "DLMalloc problem occured at: " << location << "\n";
    Panic();
}


extern "C" void *sbrk(ssize_t increment) {
    cout << "sbrk " << (size_t)increment << "\n";
    cout << "old heap end: " << Memory::instance()->heapEnd.value() << "\n";
    auto res = Memory::instance()->resizeHeapArea(increment).asPointer<void>();
    cout << "sbrk end\n";
    return  res;
}
