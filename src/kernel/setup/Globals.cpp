#include <system/System.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory/CPUMask.hpp>
#include <memory/TLBContext.hpp>
#include <processes/InterruptManager.hpp>
#include <log/BasicLog.hpp>

Processor *Processors;
TLBContext *TLBContexts;

/* dummy class to ensure BasicLog initialization happening at the right time */
class KernelBasicLog {
public:
    KernelBasicLog() {
        basicLog::init(_HandOverSystem->serialInfo, _HandOverSystem->framebufferInfo);
    }
};

/* global contructors - initialization order matters. Do not reorder or move to a different
 * compilation unit */
SerialBackend basicLog::serialBackend;
FramebufferBackend basicLog::framebufferBackend;
KernelBasicLog kernelBasicLog;
Logger cout;
frameManagement::Management frameManagement::FrameManagement;
kernelPageAllocator::Allocator kernelPageAllocator::KernelPageAllocator;
dlMallocGlue::Glue dlMallocGlue::DLMallocGlue;
System CurrentSystem;
interruptManager::Manager interruptManager::InterruptManager;
