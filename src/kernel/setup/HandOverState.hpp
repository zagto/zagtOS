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

enum FramebufferFormat : uint32_t {
    RGB = 1, BGR = 2
};

enum FirmwareType : uint32_t {
    ACPI = 1, DTB = 2
};

enum class HandleType : uint32_t {
    INVALID, FREE, PORT, REMOTE_PORT, THREAD, SHARED_MEMORY
};

struct MappedArea {
    size_t physicalStart;
    size_t start;
    size_t length;
    MappingSource source;
    Permissions permissions;
};

struct Thread {
    RegisterState registerState;
    UserVirtualAddress TLSBase;
    ThreadPriority currentPriority;
    ThreadPriority ownPriority;
};

struct Futex {
    size_t address;
    size_t numWaitingThreads;
    size_t *waitingThreadIDs;
};

struct Handle {
    HandleType type;
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
    PhysicalAddress pagingContext;
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
    FirmwareType firmwareType;
    PhysicalAddress firmwareRoot;
    PhysicalAddress secondaryProcessorEntry;
    /* TODO: a way to pass time offset to APIC timer */

    size_t numProcesses;
    Process *processes;
    /* This list should not include the idle Threads. These are re-created when the new scheduler
     * objects are created. Their code is part of kernel code so their state would be invalid after
     * kernel handover! */
    size_t numThreads;
    Thread *threads;
    size_t numPorts;
    Port *ports;
    size_t numSharedMemories;
    SharedMemory *sharedMemories;

    size_t numProcessors;
    size_t numFutexes;
    Futex *futexes;

    void decodeProcesses();
};

}
