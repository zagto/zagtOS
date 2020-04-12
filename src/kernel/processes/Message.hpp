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
    Process *sourceProcess;
    Process *destinationProcess;
    const UserVirtualAddress sourceAddress;
    MappedArea *messageArea{nullptr};
    const UUID messageType;
    const size_t numBytes;
    const size_t numHandles;
    bool transferred{false};

    bool prepareMemoryArea();
    void transferMessageInfo();
    bool transferHandles();
    bool transferData();
    UserVirtualAddress destinationAddress() const;
    size_t handlesSize() const;
    size_t simpleDataSize() const;

public:
    static constexpr size_t HANDLE_SIZE{4};

    Message(Process *sourceProcess,
            Process *destinationProcess,
            UserVirtualAddress sourceAddress,
            UUID messageType,
            size_t numBytes,
            size_t numHandles) :
        sourceProcess{sourceProcess},
        destinationProcess{destinationProcess},
        sourceAddress{sourceAddress},
        messageType{messageType},
        numBytes{numBytes},
        numHandles{numHandles} {}

    bool transfer();
    void setDestinationProcess(Process *process);
    UserVirtualAddress infoAddress() const;
};

struct UserMessageInfo {
    UUID type;
    size_t address;
    size_t length;
    size_t numHandles;
    /* externally for user = kernel */
    bool allocatedExternally;
};
