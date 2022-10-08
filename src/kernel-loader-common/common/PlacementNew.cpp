#include <common/PlacementNew.hpp>

void *operator new(size_t, void *address) {
    return address;
}
void *operator new(size_t, KernelVirtualAddress address) {
    return address.asPointer<void>();
}

