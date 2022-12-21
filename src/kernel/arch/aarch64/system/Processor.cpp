#include <system/Processor.hpp>
#include <system/System.hpp>
#include <processes/Process.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>

Processor::Processor():
        CommonProcessor() {
}

extern const char ExceptionVectorTable;

void Processor::localInitialization() noexcept {
    SetExceptionVectorTable(&ExceptionVectorTable);
}

[[noreturn]] void Processor::returnToUserMode() noexcept {
    assert(this == CurrentProcessor());
    Thread *thread = activeThread();

    /* Idle thread does not have a process, but it can't enter user mode */
    assert(thread->process);

    /* Optimization: Frame::pageOut does not add Entries for non-current Threads to the
     * PageOutContext. To ensure they still get processed before user-space code could access the
     * frames, we need to do localProcessing here. */
    invalidateQueue.localProcessing();

    thread->process->addressSpace.activate();

    SetUserTLSRegister(thread->tlsPointer);
    //cout << "returnToUserMode: " << *thread->kernelStack->userRegisterState() << endl;
    returnFromInterrupt(thread->kernelStack->userRegisterState());
}

[[noreturn]] void Processor::returnInsideKernelMode(RegisterState *state) noexcept {
    returnFromInterrupt(state);
}

void Processor::sendIPI(IPI ipi) noexcept {
    cout << "sending IPI " << static_cast<uint32_t>(ipi) << " from " <<  CurrentProcessor()->hardwareID << " to " << hardwareID << endl;

    __atomic_or_fetch(&ipiFlags, ipi, __ATOMIC_SEQ_CST);
    //CurrentProcessor()->localAPIC.sendIPI(hardwareID);
    Panic();
}

void Processor::endOfInterrupt() noexcept {
    assert(KernelInterruptsLock.isLocked());
    assert(CurrentProcessor() == this);

    cout << "endOfInterrupt" << endl;
    Panic();
}

bool ProcessorsInitialized = false;
