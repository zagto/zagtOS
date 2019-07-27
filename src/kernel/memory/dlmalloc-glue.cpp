#include <common/common.hpp>
#include <memory/Memory.hpp>
#include <memory/dlmalloc-glue.hpp>


extern "C" void BasicDLMallocPanic(const char *location) {
    Log << "DLMalloc problem occured at: " << location << "\n";
    Panic();
}


extern "C" void *sbrk(isize increment) {
    Log << "sbrk " << (usize)increment << "\n";
    Log << "old heap end: " << Memory::instance()->heapEnd.value() << "\n";
    auto res = Memory::instance()->resizeHeapArea(increment).asPointer<void>();
    Log << "sbrk end\n";
    return  res;
}
