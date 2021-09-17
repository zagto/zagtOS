#include <system/Processor.hpp>
#include <system/System.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <processes/Process.hpp>

Processor::Processor(size_t id, Status &status):
        CommonProcessor(id, status),
        tss(status),
        localAPIC(status) {

    if (!status) {
        return;
    }

    CurrentSystem.gdt.setupTSS(id, &tss);
}

void Processor::localInitialization() {
    Status status = localAPIC.initialize(readModelSpecificRegister(MSR::IA32_APIC_BASE) & 0xfffff000);
    assert(static_cast<bool>(status));
}

[[noreturn]] void Processor::returnToUserMode() {
    Thread *thread = CurrentProcessor->scheduler.activeThread();

    /* Idle thread does not have a process, but it can't enter user mode */
    assert(thread->process);

    thread->process->addressSpace.activate();

    CurrentSystem.gdt.resetTSS(CurrentProcessor->id);
    tss.update(thread);
    returnFromInterrupt(thread->kernelStack->userRegisterState(), thread->threadLocalStorage());
}

[[noreturn]] void Processor::returnInsideKernelMode(RegisterState *state) {
    returnFromInterrupt(state, {});
}

void Processor::sendIPI(IPI ipi) {
    cout << "sending IPI " << static_cast<uint32_t>(ipi) << " from " <<  CurrentProcessor->hardwareID << " to " << hardwareID << endl;

    __atomic_or_fetch(&ipiFlags, ipi, __ATOMIC_SEQ_CST);
    CurrentProcessor->localAPIC.sendIPI(hardwareID, 0x40);
}

void Processor::endOfInterrupt() {
    assert(KernelInterruptsLock.isLocked());
    assert(CurrentProcessor == this);

    localAPIC.endOfInterrupt();
}
