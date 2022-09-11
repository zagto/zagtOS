# Mutexes
## 1. FutexManager::lock
- before: mutex ProcessAddressSpace::lock

## 2. ProcessAddressSpace::lock
- before: mutex MemoryArea::lock
- before: SpinLock ProcessAddressSpace::tlbIDsLock

## 3. HandleManager::lock
- before: SpinLock PlatformInterrupt::lock

## 4. InterruptManager::Lock
- before: mutex BoundInterrupt::lock

## 5. BoundInterrupt::lock
- before: mutex EventQueue::lock
- before: SpinLock KernelPageAllocator::lock

## 6. EventQueue::lock
- before: mutex ProcessAddressSpace::lock
- before: SpinLock Scheduler::lock
- before: SpinLock KernelPageAllocator::lock
- before: SpinLock Thread::stateLock

## 7. ProcessAddressSpace::lock
- before: mutex MemoryArea::lock

## 8. MemoryArea::lock
- before: Spinlock Processor::tlbContextsLock
- before: Spinlock InvalidateList::lock
- before: SpinLock ProcessAddressSpace::tlbIDsLock

# KernelInterruptsLock
- after: all mutexes
- before: all SpinLocks

# SpinLocks
## 0. ProcessAddressSpace::tlbIDsLock
- before: SpinLock Scheduler::lock

## 1. Processor::tlbContextsLock
- before: SpinLock InvalidateList::lock

## 2. InvalidateQueue::lock
- before: SpinLock KernelPageAllocator::lock

## 3. KernelPageAllocator::lock
- before: SpinLock FrameManagement::lock

## 4. SpinLock FrameManagement::lock

## 5. SpinLock Scheduler::lock
- before: SpinLock Thread::stateLock
- before: SpinLock Time::offsetLock

## 6. SpinLock Thread::stateLock
only do one at a time - a scheduler may lock one second one

## 7. SpinLock Time::offsetLock

## 8. logLock (basicLog.cpp)
- after: everything else

