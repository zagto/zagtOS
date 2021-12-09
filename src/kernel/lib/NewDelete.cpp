#include <common/addresses.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory>

void *operator new(size_t, KernelVirtualAddress address) {
    return address.asPointer<void>();
}
/*void operator delete(void *object) {
    DLMallocGlue.free(KernelVirtualAddress(object));
}

void operator delete(void *object, size_t) {
    operator delete(object);
}

void operator delete(void *object, size_t, std::align_val_t) {
    operator delete(object);
}*/
