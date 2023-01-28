#pragma once

#include <zagtos/EventListener.hpp>
#include <zagtos/SharedMemory.hpp>
#include "Device.hpp"

class MemoryArea : public zagtos::EventListener {
private:
    friend class Command;
    std::vector<size_t> deviceAddresses;
    const size_t length;
    Device &device;

public:
    zagtos::SharedMemory sharedMemory;

    MemoryArea(Device &device, size_t length);
    void handleEvent(const zagtos::Event &event) final;
};
