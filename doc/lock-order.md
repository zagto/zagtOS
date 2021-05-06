
mutex FutexManager::lock > mutex ProcessAddressSpace::lock
mutex ProcessAddressSpace::lock > mutex MemoryArea::lock
MemoryArea::lock > Processor::tlbContextsLock
MemoryArea::lock > InvalidateList::lock Spinlock
SpinLock InvalidateList::lock > SpinLock Logger::lock
SpinLock InvalidateList::lock > SpinLock FrameManagement.lock
SpinLock FrameManagement.lock> SpinLock KernelPageAllocator.lock

SpinLock Scheduler::lock > Thread::stateLock

after owner:
Thread::stateLock (only do one at a time - a scheduler may lock one second one)
