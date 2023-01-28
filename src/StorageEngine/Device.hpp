#pragma once

#include <zagtos/EventListener.hpp>
#include <zagtos/protocols/BlockDevice.hpp>
#include "GptStructures.hpp"

class Device : public zagtos::EventListener {
private:
    enum class State {
        ALLOCATE_HEADER,
        LOAD_HEADER,
        ALLOCATE_PARTITIONS,
        LOAD_PARTITIONS,
        NO_GPT,
        READY
    };

    zagtos::RemotePort devicePort;
    const zagtos::blockDevice::DeviceInfo info;
    State state;
    zagtos::SharedMemory gptHeaderMemory;
    zagtos::RemotePort gptHeaderPort;
    zagtos::SharedMemory gptPartitionsMemory;
    zagtos::RemotePort gptPartitionsPort;
    const GptHeader *gptHeader = nullptr;
    size_t numPartitionSectors;
    std::vector<const PartitionEntry *> partitionEntries;

    void sendAllocateHeader();
    void sendLoadHeader();
    void sendAllocatePartitions();
    void sendLoadPartitions();

public:
    const uint64_t id;

    Device(zagtos::RemotePort devicePort, uint64_t deviceID, zagtos::blockDevice::DeviceInfo info);
    void handleEvent(const zagtos::Event &event) final;
};
