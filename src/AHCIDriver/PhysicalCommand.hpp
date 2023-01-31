#pragma once

#include <zagtos/SharedMemory.hpp>
#include "Device.hpp"
#include "MemoryArea.hpp"
#include "LogicalCommand.hpp"

class Device;
class CommandHeader;
class CommandTable;
enum class ATACommand;

class PhysicalCommand {
private:
    PhysicalCommand(Device &device);
    static ATACommand ataCommandFor(LogicalCommand::Action action);
    static bool actionIsWrite(LogicalCommand::Action action);

public:
    std::unique_ptr<LogicalCommand> logicalCommand;
    Device &device;
    const size_t slotID;
    CommandHeader &header;
    CommandTable &table;
    MemoryArea *memoryArea;

    PhysicalCommand(std::unique_ptr<LogicalCommand> logicalCommand, size_t slotID);
    ~PhysicalCommand();
    PhysicalCommand(PhysicalCommand &other) = delete;
};

