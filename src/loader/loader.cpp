#include <exit.hpp>
#include <log/Logger.hpp>
#include <Framebuffer.hpp>
#include <Files.hpp>
#include <MemoryMap.hpp>
#include <memory/VirtualMemory.hpp>
#include <memory/PhysicalMemory.hpp>
#include <Paging.hpp>
#include <ProgramBinary.hpp>
#include <common/utils.hpp>
#include <Firmware.hpp>


/* converts pointers to physical memory for use in kernel where the identity mapping is offset at
 * an address in high kernel memory */
template<typename T> T *convertPointer(T *loaderPointer) {
    if (reinterpret_cast<size_t>(loaderPointer) == PhysicalAddress::NULL) {
        return nullptr;
    } else {
        return reinterpret_cast<T *>(reinterpret_cast<size_t>(loaderPointer) + IdentityMapping.start);
    }
}

void convertFrameStack(frameStack::Node *head) {
    if (reinterpret_cast<size_t>(head) == PhysicalAddress::NULL) {
        return;
    }
    frameStack::Node *node = head;
    while (reinterpret_cast<size_t>(node->next) != PhysicalAddress::NULL) {
        frameStack::Node *last = node;
        node = node->next;
        last->next = convertPointer(last->next);
    }
    node->next = nullptr;
}

void isort(hos_v1::MappedArea *mappedAreas, size_t len) {
    for (size_t i = 1; i < len; i++) {
        hos_v1::MappedArea tmp = mappedAreas[i];
        size_t j = i;
        while (j > 0 && mappedAreas[j-1].start > tmp.start) {
            mappedAreas[j] = mappedAreas[j-1];
            j--;
        }
        mappedAreas[j] = tmp;
    }
}

void LoaderMain() {
    cout << "Initializing..." << endl;
    hos_v1::FramebufferInfo &framebufferInfo = InitFramebuffer();

    cout << "Detecting Images..." << endl;
    ProgramBinary kernel = LoadKernelImage();
    ProgramBinary process = LoadProcessImage();

    /* sections, run message */
    size_t numMappings = process.numSections() + 1;
    if (process.hasTLS()) {
        numMappings++;
    }
    size_t numFrames = process.loadedUserFrames();

    size_t handOverSize = sizeof(hos_v1::System)
            + sizeof(hos_v1::Process)
            + sizeof(hos_v1::Thread)
            + sizeof(hos_v1::Handle)
            + 17 /* SystemEnvironment string */
            + numMappings * sizeof(hos_v1::MappedArea)
            + numMappings * sizeof(hos_v1::MemoryArea)
            + numFrames * sizeof(hos_v1::Frame)
            /* handover frame ids, futex frame ids */
            + 2 * numFrames * sizeof(size_t);
    uint8_t *pointer = memoryMap::allocateHandOver(handOverSize / PAGE_SIZE + 1);
    auto handOverSystem = reinterpret_cast<hos_v1::System *>(pointer);
    pointer += sizeof (hos_v1::System);
    auto handOverProcess = reinterpret_cast<hos_v1::Process *>(pointer);
    pointer += sizeof (hos_v1::Process);
    auto handOverThread = reinterpret_cast<hos_v1::Thread *>(pointer);
    pointer += sizeof (hos_v1::Thread);
    auto handOverHandle = reinterpret_cast<hos_v1::Handle *>(pointer);
    pointer += sizeof (hos_v1::Handle);
    auto handOverMappedAreas = reinterpret_cast<hos_v1::MappedArea *>(pointer);
    pointer += numMappings * sizeof(hos_v1::MappedArea);
    auto handOverMemoryAreas = reinterpret_cast<hos_v1::MemoryArea *>(pointer);
    pointer += numMappings * sizeof(hos_v1::MemoryArea);
    auto handOverFrames = reinterpret_cast<hos_v1::Frame *>(pointer);
    pointer += numFrames * sizeof(hos_v1::Frame);
    auto handOverFramesIDs = reinterpret_cast<size_t *>(pointer);
    pointer += numFrames * sizeof(size_t);
    auto futexFramesIDs = reinterpret_cast<size_t *>(pointer);
    pointer += numFrames * sizeof(size_t);
    auto handOverString = reinterpret_cast<char *>(pointer);

    cout << "Getting Memory Map..." << endl;
    memoryMap::freezeAndExitFirmware();

    cout << "Initialzing Memory Management..." << endl;
    PhysicalAddress maxPhysicalAddress = InitPhysicalFrameManagement();
    InitPaging();
    MapLoaderMemory();
    MapFramebufferMemory(framebufferInfo);
    CreateIdentityMap(maxPhysicalAddress);

    size_t frameIndex = 0;
    cout << "Setting up address space for kernel..." << endl;
    kernel.load(PagingContext::GLOBAL, nullptr, frameIndex);
    /* loadring to GLOBAL context should map directly and not fill in frames */
    assert(frameIndex == 0);

    cout << "Setting up address space for initial process..." << endl;
    process.load(PagingContext::PROCESS, handOverFrames, frameIndex);
    assert(frameIndex == numFrames);

    cout << "Preparing handover structures..." << endl;

    RegisterState regState(process.entryAddress(),
                           UserSpaceRegion.end() - 1, /* ensure valid UserVirtualAddress */
                           process.runMessageAddress(),
                           process.TLSBase(),
                           process.masterTLSBase(),
                           process.TLSRegion().length - process.TLSBase().value() % PAGE_SIZE);

    *handOverSystem = hos_v1::System{
        .version = 1,
        .framebufferInfo = framebufferInfo,
        .freshFrameStack = {}, /* correct data will be filled in later */
        .usedFrameStack = {},  /* correct data will be filled in later */
        .handOverPagingContext = reinterpret_cast<size_t>(HandOverMasterPageTable),
        .firmwareType = GetFirmwareType(),
        .firmwareRoot = GetFirmwareRoot(),
        .secondaryProcessorEntry = SecondaryProcessorEntry,
        .numProcesses = 1,
        .processes = convertPointer(handOverProcess),
        .numThreads = 1,
        .threads = convertPointer(handOverThread),
        .numPorts = 0,
        .ports = nullptr,
        .numMemoryAreas = numMappings,
        .memoryAreas = handOverMemoryAreas,
        .numProcessors = 1,
        .numFutexes = 0,
        .futexes = nullptr,
        .nextFutexFrameID = PAGE_SIZE,
    };
    for (size_t stackIndex = 0; stackIndex < hos_v1::DMAZone::COUNT; stackIndex++) {
        handOverSystem->freshFrameStack[stackIndex] = {
            convertPointer(CleanFrameStack[stackIndex].head),
            CleanFrameStack[stackIndex].addIndex
        };
        handOverSystem->usedFrameStack[stackIndex] = {
            convertPointer(DirtyFrameStack[stackIndex].head),
            DirtyFrameStack[stackIndex].addIndex
        };
    }
    *handOverProcess = {
        .numMappedAreas = numMappings,
        .mappedAreas = convertPointer(handOverMappedAreas),
        .numLocalFutexes = 0,
        .localFutexes = nullptr,
        .numHandles = 1,
        .handles = convertPointer(handOverHandle),
        .numLogNameChars = 17,
        .logName = convertPointer(handOverString)
    };
    *handOverThread = hos_v1::Thread{
        .registerState = regState,
        .TLSBase = process.TLSBase(),
        .currentPriority = hos_v1::ThreadPriority::BACKGROUND,
        .ownPriority = hos_v1::ThreadPriority::BACKGROUND,
    };
    *handOverHandle = hos_v1::Handle{
        .type = hos_v1::HandleType::THREAD,
        .handle = 1,
        .objectID = 0
    };
    frameIndex = 0;
    for (size_t index = 0; index < process.numSections(); index++) {
        size_t offset = process.sectionOffset(index);
        handOverMappedAreas[index] = hos_v1::MappedArea{
            .memoryAreaID = index,
            .offset = 0,
            .start = process.sectionAddress(offset),
            .length = process.sectionSizeInMemory(offset),
            .permissions = process.sectionPermissions(offset)
        };
        handOverMemoryAreas[index] = hos_v1::MemoryArea{
            .frameIDs = handOverFramesIDs + frameIndex,
            .futexIDs = futexFramesIDs + frameIndex,
            .source = hos_v1::MappingSource::ANONYMOUS,
            .isShared = false,
            .permissions = Permissions::READ_WRITE_EXECUTE,
            .length = process.sectionSizeInMemory(offset),
        };
        frameIndex += process.sectionSizeInMemory(offset) / PAGE_SIZE;
    }
    /* run message */
    handOverMappedAreas[process.numSections()] = hos_v1::MappedArea{
        .memoryAreaID = process.numSections(),
        .offset = 0,
        .start = process.runMessageAddress().value(),
        .length = PAGE_SIZE,
        .permissions = hos_v1::Permissions::READ
    };
    handOverMemoryAreas[process.numSections()] = hos_v1::MemoryArea{
        .frameIDs = handOverFramesIDs + frameIndex,
        .futexIDs = futexFramesIDs + frameIndex,
        .source = hos_v1::MappingSource::ANONYMOUS,
        .isShared = false,
        .permissions = hos_v1::Permissions::READ_WRITE_EXECUTE,
        .length = PAGE_SIZE
    };
    frameIndex++;
    /* TLS */
    if (process.hasTLS()) {
        handOverMappedAreas[process.numSections() + 1] = hos_v1::MappedArea{
            .memoryAreaID = process.numSections() + 1,
            .offset = 0,
            .start = process.TLSRegion().start,
            .length = process.TLSRegion().length,
            .permissions = hos_v1::Permissions::READ_WRITE
        };
        handOverMemoryAreas[process.numSections() + 1] = hos_v1::MemoryArea{
            .frameIDs = handOverFramesIDs + frameIndex,
            .futexIDs = futexFramesIDs + frameIndex,
            .source = hos_v1::MappingSource::ANONYMOUS,
            .isShared = false,
            .permissions = hos_v1::Permissions::READ_WRITE_EXECUTE,
            .length = process.TLSRegion().length
        };
        frameIndex += process.TLSRegion().length / PAGE_SIZE;
    }
    assert(frameIndex == numFrames);
    isort(handOverMappedAreas, numMappings);
    memcpy(handOverString, "SystemEnvironment", 17);

    cout << "Converting frame stacks..." << endl;
    for (FrameStack &stack: CleanFrameStack) {
        convertFrameStack(stack.head);
    }
    for (FrameStack &stack: DirtyFrameStack) {
        convertFrameStack(stack.head);
    }

    cout << "Exiting to Kernel..." << endl;

    ExitToKernel(kernel.entryAddress(), HandOverMasterPageTable, handOverSystem);
}

void* __dso_handle = nullptr;
