#pragma once

#include <zagtos/HandleObject.hpp>

namespace zagtos {

class SharedMemory : public HandleObject {
private:
    void *_map(int protection);

public:
    static std::tuple<SharedMemory, std::vector<size_t>> DMA(size_t deviceMax, size_t length);
    static SharedMemory Physical(size_t physicalAddress, size_t length);
    static SharedMemory Standard(size_t length);

    SharedMemory() {}
    SharedMemory(SharedMemory &) = delete;
    SharedMemory(SharedMemory &&other) : HandleObject(std::move(other)) {}

    void operator=(SharedMemory && other);

    template<typename T> T *map(int protection) {
        return reinterpret_cast<T *>(_map(protection));
    }
};
void UnmapWhole(void *pointer);

}
