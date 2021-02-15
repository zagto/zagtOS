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
    MappedArea *messageArea{nullptr};
    UUID messageType;
    size_t numBytes{0};
    size_t numHandles{0};
    bool transferred{false};

    Status prepareMemoryArea();
    Status transferMessageInfo();
    Status transferHandles();
    Status transferData();
    UserVirtualAddress destinationAddress() const;
    size_t handlesSize() const;
    size_t simpleDataSize() const;

public:
    static constexpr size_t HANDLE_SIZE{4};

    UserVirtualAddress infoAddress;

    Message(Process *sourceProcess,
            Process *destinationProcess,
            UserVirtualAddress sourceAddress,
            UUID messageType,
            size_t numBytes,
            size_t numHandles,
            Status &status) :
        sourceProcess{sourceProcess},
        destinationProcess{destinationProcess},
        sourceAddress{sourceAddress},
        messageType{messageType},
        numBytes{numBytes},
        numHandles{numHandles} {

        status = Status::OK();
    }
    Message(const hos_v1::Message &handOver, Status status) :
        infoAddress{handOver.infoAddress} {

        status = Status::OK();
    }

    Status transfer();
    void setDestinationProcess(Process *process);
};

/* when changing this struct also change it in loader/ProgramBinary.hpp */
struct UserMessageInfo {
    UUID type;
    size_t address;
    size_t length;
    size_t numHandles;
    /* externally for user = kernel */
    bool allocatedExternally;
};
