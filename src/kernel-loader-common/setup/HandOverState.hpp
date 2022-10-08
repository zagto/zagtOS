#pragma once

#include <common/inttypes.hpp>
#include <interrupts/RegisterState.hpp>
#include <processes/UserApi.hpp>

namespace hos_v1 {

enum class MappingSource : uint32_t {
    ANONYMOUS = 1, PHYSICAL = 2, DMA = 3,
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
    INVALID, FREE, PORT, REMOTE_PORT, THREAD, MEMORY_AREA, INTERRUPT, EVENT_QUEUE
};

/* TODO: other architectures */
enum DMAZone {
    BITS32 = 0, BITS64, COUNT
};
static const size_t DMAZoneMax[DMAZone::COUNT] = {(1ul << 32) - 1, static_cast<size_t>(-1)};
static const size_t FRAME_ID_NONE = static_cast<size_t>(-1);
static const size_t FUTEX_FRAME_ID_NONE = 0;

struct Frame {
    size_t address;
    size_t copyOnWriteCount;
    bool isForPhysicalAccess;
};

struct MemoryArea {
    size_t *frameIDs;
    uint64_t *futexIDs;
    MappingSource source;
    bool isShared;
    Permissions permissions;
    size_t length;
};

struct MappedArea {
    size_t memoryAreaID;
    size_t offset;
    size_t start;
    size_t length;
    Permissions permissions;
};

struct Thread {
    RegisterState registerState;
    size_t tlsPointer;
    ThreadPriority currentPriority;
    ThreadPriority ownPriority;
};

struct Futex {
    uint64_t id;
    size_t numWaitingThreads;
    size_t *waitingThreadIDs;
};

struct Handle {
    HandleType type;
    uint32_t handle;
    size_t objectID;
};

struct EventQueue {
    size_t numWaitingThreads;
    size_t *waitingThreadIDs;
    size_t numEvents;
    userApi::ZoEventInfo *events;
};

struct Port {
    size_t tag;
    size_t eventQueueID;
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
    uint32_t scaleFactor;
};

struct FrameStack {
    void *head;
    size_t addIndex;
};

struct System {
    size_t version;

    FramebufferInfo framebufferInfo;
    FrameStack freshFrameStack[DMAZone::COUNT];
    FrameStack usedFrameStack[DMAZone::COUNT];
    PhysicalAddress handOverPagingContext;
    FirmwareType firmwareType;
    PhysicalAddress firmwareRoot;
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
    size_t numMemoryAreas;
    MemoryArea *memoryAreas;
    size_t numFrames;
    Frame *frames;
    size_t numEventQueues;
    EventQueue *eventQueues;

    uint64_t timerFrequency;

    size_t numProcessors;
    size_t numFutexes;
    Futex *futexes;
    /* not 0, multiple of PAGE_SIZE */
    size_t nextFutexFrameID;

    void decodeProcesses();
};

}
