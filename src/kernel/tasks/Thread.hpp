#ifndef COMMONTHREAD_HPP
#define COMMONTHREAD_HPP

#include <interrupts/RegisterState.hpp>

class Processor;
class Scheduler;
class Task;

static const size_t THREAD_STRUCT_AREA_SIZE = 0x400;

static const uint32_t  SYS_LOG = 1,
                  SYS_EXIT = 2,
                  SYS_SEND_MESSAGE = 3,
                  SYS_WAIT_MESSAGE = 4,
                  SYS_CREATE_PORT = 5,
                  SYS_RANDOM = 6,

                  SYS_MPROTECT = 10,
                  SYS_MMAP = 11,
                  SYS_MUNMAP = 12,
                  SYS_MSYNC = 13,
                  SYS_MREMAP = 14,

                  SYS_CREATE_THREAD = 20,
                  SYS_EXIT_THREAD = 21,
                  SYS_FUTEX = 22,
                  SYS_SET_ROBUST_LIST = 23,
                  SYS_GET_ROBUST_LIST = 24,
                  SYS_YIELD = 25,
                  SYS_SET_THREAD_AREA = 26,
                  SYS_GET_THREAD_AREA = 27,

                  SYS_CLOCK_GETTIME = 30,
                  SYS_CLOCK_NANOSLEEP = 31,

                  SYS_REGISTER_INTERRUPT = 40,
                  SYS_UNREGISTER_INTERRUPT = 41,
                  SYS_GET_ACPI_ROOT = 42,
                  SYS_GET_PHYSICAL_ADDRESS = 43,
                  SYS_IO_PORT_READ = 44,
                  SYS_IO_PORT_WRITE = 45;


class Thread {
protected:
    friend class Scheduler;
    Processor *currentProcessor{nullptr};
    bool usesFloatingPoint{false};

public:
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_BACKGROUND, INTERACTIVE_FOREGROUND
    };
    static const size_t NUM_PRIORITIES = 5;

public:
    RegisterState registerState;
    Task *task;
    UserVirtualAddress tlsBase;
    Priority ownPriority;
    Priority currentPriority;

    Thread(Task *task,
           VirtualAddress entry,
           Priority priority,
           UserVirtualAddress stackPointer,
           UserVirtualAddress tlsBase,
           UserVirtualAddress masterTLSBase,
           size_t tlsSize) :
        registerState(entry, stackPointer, tlsBase, masterTLSBase, tlsSize),
        task{task},
        tlsBase{tlsBase},
        ownPriority{priority},
        currentPriority{priority} {}
    ~Thread();

    UserVirtualAddress userThreadStruct() {
        return UserVirtualAddress(tlsBase.value() - THREAD_STRUCT_AREA_SIZE);
    }
    bool handleSyscall();
};

#endif
