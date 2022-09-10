#pragma once

#include <vector>
#include <processes/UUID.hpp>

class Port;
class Process;
class MappedArea;

class Message {
private:
    /* Messages can't use shared_ptrs because a processes run message is transferred in the Process
     * constructor. Since long-living Messages are hold by the Port, they won't affect destintion
     * Process lifetime and should not affect Source process lifetime anyways. */
    Process *sourceProcess{nullptr};
    Process *destinationProcess{nullptr};
    UserVirtualAddress sourceAddress;
    UUID messageType;
    size_t numBytes{0};
    size_t numHandles{0};
    bool transferred{false};

    void prepareMemoryArea();
    void transferMessageInfo();
    void transferHandles();
    void transferData();
    UserVirtualAddress destinationAddress() const noexcept;
    size_t handlesSize() const noexcept;
    size_t simpleDataSize() const noexcept;

public:
    static constexpr size_t HANDLE_SIZE{4};

    UserVirtualAddress infoAddress;

    Message(Process *sourceProcess,
            Process *destinationProcess,
            UserVirtualAddress sourceAddress,
            UUID messageType,
            size_t numBytes,
            size_t numHandles) noexcept :
        sourceProcess{sourceProcess},
        destinationProcess{destinationProcess},
        sourceAddress{sourceAddress},
        messageType{messageType},
        numBytes{numBytes},
        numHandles{numHandles} {}
    Message(const hos_v1::Message &handOver) noexcept :
        infoAddress{handOver.infoAddress} {}

    void transfer();
    void setDestinationProcess(Process *process) noexcept;
};
