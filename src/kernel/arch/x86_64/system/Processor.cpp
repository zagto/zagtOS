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

__attribute__((noreturn))
void Processor::returnToUserMode() {
    Thread *thread = CurrentProcessor->scheduler.activeThread();

    cout << "A" << endl;
    /* Idle thread does not have a process, but it can't enter user mode */
    assert(thread->process);

    thread->process->addressSpace.activate();
    cout << "B" << endl;

    CurrentSystem.gdt.resetTSS(CurrentProcessor->id);
    cout << "C" << endl;
    tss.update(thread);
    cout << "D" << endl;
    returnFromInterrupt(thread->kernelStack->userRegisterState(), thread->threadLocalStorage());
}

void Processor::sendCheckSchedulerIPI() {
    cout << "sending Check Scheduler IPI from " <<  CurrentProcessor->hardwareID << " to " << hardwareID << endl;

    CurrentProcessor->localAPIC.sendIPI(hardwareID, 0x40);
}
