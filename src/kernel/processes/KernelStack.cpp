#include <processes/KernelStack.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <system/Processor.hpp>

extern "C" __attribute__((noreturn)) void switchStack(size_t newStackPointer,
                                                      void entry(void *),
                                                      void *arg);

KernelStack::KernelStack(RegisterState _userRegisterState, Status &status) {
    if (!status) {
        return;
    }

    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock sl(KernelPageAllocator.lock);

    Result<void *> dataAddress = KernelPageAllocator.map(SIZE, true);
    if (!dataAddress) {
        status = dataAddress.status();
        return;
    }

    data = *dataAddress;
    *userRegisterState() = _userRegisterState;
}

KernelStack::~KernelStack() {
    if (data) {
        scoped_lock lg1(KernelInterruptsLock);
        scoped_lock sl(KernelPageAllocator.lock);
        KernelPageAllocator.unmap(data, SIZE, true);
    }
}

RegisterState *KernelStack::userRegisterState() {
    return reinterpret_cast<RegisterState *>(reinterpret_cast<size_t>(data) + USER_STATE_OFFSET);
}

[[noreturn]] void KernelStack::switchToKernelEntry(void kernelEntry(void *),void *argument) {
    /* On early inititalization, we need to switch to the initial KernelStack of each Processor.
     * in this case the following Assertion will fail. Note that activeThread being nullptr does
     * not nessesarily mean we are in early initializion. */
    if (CurrentProcessor->scheduler.activeThread() != nullptr) {
        assert(CurrentProcessor->kernelStack.get() != this);
    }

    lock.lock();

    size_t initialStackPointer = reinterpret_cast<size_t>(data) + INITIAL_KERNEL_STACK_POINTER_OFFSET;
    switchStack(initialStackPointer, kernelEntry, argument);
}
