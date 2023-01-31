#pragma once

#include <zagtos/EventListener.hpp>
#include <zagtos/SharedMemory.hpp>

class Device;
struct LogicalCommand;

class MemoryArea : public zagtos::EventListener {
private:
    friend class PhysicalCommand;
    std::vector<size_t> deviceAddresses;
    const size_t length;

public:
    Device &device;
    zagtos::SharedMemory sharedMemory;

    MemoryArea(Device &device, size_t length);
    void handleEvent(const zagtos::Event &event) final;
    void commandComplete(LogicalCommand &command);
};
