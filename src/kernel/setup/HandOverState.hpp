#pragma once

#include <common/inttypes.hpp>
#include <interrupts/RegisterState.hpp>

namespace hos_v1 {

enum class MappingSource : uint32_t {
    MEMORY = 1, PHYSICAL_MEMORY = 2, GUARD = 3
};

enum class Permissions : uint32_t {
    READ_WRITE = 1, READ_EXECUTE = 2, READ_WRITE_EXECUTE = 3, READ = 4, INVALID = 5
};

enum ThreadPriority : uint32_t {
    IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_FOREGROUND,
};

struct MappedArea {
    size_t type;
    size_t physicalStart;
    size_t start;
    size_t length;
    MappingSource source;
    Permissions permissions;
};

struct Thread {
    RegisterState registerState;
    PhysicalAddress paginingContext;
    size_t TLSBase;
    ThreadPriority currentPriority;
    ThreadPriority ownPriority;
};

struct Futex {
    size_t address;
    size_t numWaitingThreads;
    size_t *waitingThreadIDs;
};

struct Handle {
    size_t type;
    uint32_t handle;
    size_t objectID;
};

struct Message {
    size_t infoAddress;
};

struct Port {
    bool threadWaits;
    size_t waitingThreadID;
    size_t numMessages;
    Message *messages;
};

struct Process {
    size_t numMappedAreas;
    MappedArea *mappedAreas;
    size_t numLocalFutexes;
    Futex *localFutexes;
    size_t numHandles;
    Handle *handles;
    size_t numLogNameChars;
    char *logName;
};

struct FramebufferInfo {
    uint8_t *frontBuffer;
    uint8_t *backBuffer;
    uint32_t width;
    uint32_t height;
    uint32_t bytesPerPixel;
    uint32_t bytesPerLine;
    uint32_t format;
};

struct FrameStack {
    void *head;
    size_t addIndex;
};

struct SharedMemory {
    size_t source;
    size_t physicalStart;
    size_t length;
};


struct System {
    size_t version;

    FramebufferInfo framebufferInfo;
    FrameStack freshFrameStack;
    FrameStack usedFrameStack;
    PhysicalAddress handOverPagingContext;
    PhysicalAddress ACPIRoot;
    PhysicalAddress secondaryProcessorEntry;
    /* TODO: a way to pass time offset to APIC timer */

    size_t numProcesses;
    Process *processes;
    size_t numThreads;
    Thread *threads;
    size_t numPorts;
    Port *ports;
    size_t numSharedMemories;
    SharedMemory *sharedMemories;

    size_t numProcessors;
    bool isMainProcessor;
    size_t numFutexes;
    Futex *futexes;

    void decodeProcesses();
};

}
