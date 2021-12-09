
mutex FutexManager::lock > mutex ProcessAddressSpace::lock
mutex ProcessAddressSpace::lock > mutex MemoryArea::lock
mutex ProcessAddressSpace::lock > SpinLock ProcessAddressSpace::tlbIDsLock
MemoryArea::lock > Spinlock Processor::tlbContextsLock
MemoryArea::lock > Spinlock InvalidateList::lock
Processor::tlbContextsLock?? > SpinLock InvalidateList::lock
SpinLock InvalidateList::lock > SpinLock Logger::lock
SpinLock InvalidateList::lock > SpinLock FrameManagement.lock
SpinLock FrameManagement.lock> SpinLock KernelPageAllocator.lock

SpinLock Scheduler::lock > Thread::stateLock

after owner:
Thread::stateLock (only do one at a time - a scheduler may lock one second one)
