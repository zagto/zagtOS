#include <interrupts/Record.hpp>
#include <interrupts/util.hpp>


template<typename T> Record<T>::Record(T *table) {
    // 1 always needs to be substracted from actual size for this field
    size = sizeof(T) - 1;
    address = table;
}


template<> void Record<InterruptDescriptorTable>::load() {
    loadInterruptDescriptorTable(this);
}


template<> void Record<GlobalDescriptorTable>::load() {
    loadGlobalDescriptorTable(this);
}
