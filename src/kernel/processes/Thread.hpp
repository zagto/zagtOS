#ifndef THREAD_HPP
#define THREAD_HPP

#include <interrupts/RegisterState.hpp>
#include <memory>
#include <vector>

class Processor;
class Scheduler;
class Process;
class Port;

static const size_t THREAD_STRUCT_AREA_SIZE = 0x400;


class Thread {
protected:
    friend class Scheduler;
    Processor *currentProcessor{nullptr};
    bool usesFloatingPoint{false};

public:
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_FOREGROUND
    };
    static const size_t NUM_PRIORITIES = 4;

public:
    RegisterState registerState;
    Process *process;
    UserVirtualAddress tlsBase;
    Priority ownPriority;
    Priority currentPriority;

    Thread(Process *process,
           VirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase,
           UserVirtualAddress masterTLSBase,
           size_t tlsSize) :
        registerState(entry, stackPointer, tlsBase, masterTLSBase, tlsSize),
        process{process},
        tlsBase{tlsBase},
        ownPriority{priority},
        currentPriority{priority} {}
    ~Thread();

    UserVirtualAddress userThreadStruct() {
        return UserVirtualAddress(tlsBase.value() - THREAD_STRUCT_AREA_SIZE);
    }
    bool handleSyscall();
    bool lookupOwnPort(uint32_t handle, shared_ptr<Port> &port) const;
};

#endif
