#pragma once

#include "PortListener.hpp"
#include "zagtos/SharedMemory.hpp"

class MemoryArea : public PortListener {
private:
    std::vector<size_t> deviceAddresses;
    const size_t length;

public:
    zagtos::SharedMemory sharedMemory;

    MemoryArea(size_t length);
    void handleMessage(const zagtos::Event &event) final;
};
