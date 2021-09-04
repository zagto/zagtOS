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

void Processor::sendCheckSchedulerIPI() {
    cout << "sending Check Scheduler IPI from " <<  CurrentProcessor->hardwareID << " to " << hardwareID << endl;

    CurrentProcessor->localAPIC.sendIPI(hardwareID, 0x40);
}

void Processor::sendInvalidateQueueProcessingIPI() {
    cout << "sending InvalidateQueueProcessing IPI from " <<  CurrentProcessor->hardwareID << " to " << hardwareID << endl;

    CurrentProcessor->localAPIC.sendIPI(hardwareID, 0x41);
}

