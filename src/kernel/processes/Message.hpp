#pragma once

#include <vector>
#include <processes/Event.hpp>
#include <processes/UUID.hpp>

class Port;
class Process;
class MappedArea;

class Message {
private:
    /* Messages can't use shared_ptrs because a processes run message is transferred in the Process
     * constructor. Since long-living Messages are hold by the Port, they won't affect destintion
     * Process lifetime and should not affect Source process lifetime anyways. */
    Event event;
    Process *sourceProcess{nullptr};
    Process *destinationProcess{nullptr};
    UserVirtualAddress sourceAddress;
    UUID messageType;
    size_t numBytes{0};
    size_t numHandles{0};
    bool transferred{false};
    bool isRunMessage;
    /* run message only */
    uint32_t threadHandle = -1u;
    uint32_t eventQueueHandle = -1u;

    void prepareMemoryArea();
    void transferStartupInfo();
    void transferHandles();
    void transferData();
    size_t infoStructSize() const noexcept;
    UserVirtualAddress destinationAddress() const noexcept;
    size_t handlesSize() const noexcept;
    size_t simpleDataSize() const noexcept;

public:
    static constexpr size_t HANDLE_SIZE{4};

    UserVirtualAddress infoAddress;

    Message(Process *sourceProcess,
            Process *destinationProcess,
            size_t destinationTag,
            UserVirtualAddress sourceAddress,
            UUID messageType,
            size_t numBytes,
            size_t numHandles,
            bool isRunMessage) noexcept :
        event(userApi::ZAGTOS_EVENT_MESSAGE, destinationTag),
        sourceProcess{sourceProcess},
        destinationProcess{destinationProcess},
        sourceAddress{sourceAddress},
        messageType{messageType},
        numBytes{numBytes},
        numHandles{numHandles},
        isRunMessage{isRunMessage} {}

    unique_ptr<Event> transfer();
    void setDestinationProcess(Process *process) noexcept;
    void setStartupInfo(uint32_t threadHandle, uint32_t messageQueueHandle) noexcept;
};
