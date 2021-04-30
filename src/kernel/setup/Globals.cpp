#include <system/System.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <memory/DLMallocGlue.hpp>

/* global contructors - initialization order matters. Do not reorder or move to a different
 * compilation unit */
Logger cout;
frameManagement::Management frameManagement::FrameManagement;
kernelPageAllocator::Allocator kernelPageAllocator::KernelPageAllocator;
dlMallocGlue::Glue dlMallocGlue::DLMallocGlue;
System CurrentSystem;
