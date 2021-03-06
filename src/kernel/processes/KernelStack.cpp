#include <processes/KernelStack.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <system/Processor.hpp>

extern "C" __attribute__((noreturn)) void switchStack(size_t newStackPointer,
                                                      void entry(void *),
                                                      void *arg);

KernelStack::KernelStack(RegisterState _userRegisterState) {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock sl(KernelPageAllocator.lock);

    data = KernelPageAllocator.map(SIZE, true);
    *userRegisterState() = _userRegisterState;
}

KernelStack::~KernelStack() noexcept {
    if (data) {
        scoped_lock lg1(KernelInterruptsLock);
        scoped_lock sl(KernelPageAllocator.lock);
        KernelPageAllocator.unmap(data, SIZE, true);
    }
}

RegisterState *KernelStack::userRegisterState() noexcept {
    return reinterpret_cast<RegisterState *>(reinterpret_cast<size_t>(data) + USER_STATE_OFFSET);
}

[[noreturn]] void KernelStack::switchToKernelEntry(void kernelEntry(void *),void *argument) noexcept {
    /* On early inititalization, we need to switch to the initial KernelStack of each Processor.
     * in this case the following Assertion will fail. Note that activeThread being nullptr does
     * not nessesarily mean we are in early initializion. */
    Processor *currentProcessor = CurrentProcessor();
    if (currentProcessor->activeThread() != nullptr) {
        assert(currentProcessor->kernelStack.get() != this);
    }

    lock.lock();

    size_t initialStackPointer = reinterpret_cast<size_t>(data) + INITIAL_KERNEL_STACK_POINTER_OFFSET;
    switchStack(initialStackPointer, kernelEntry, argument);
}
