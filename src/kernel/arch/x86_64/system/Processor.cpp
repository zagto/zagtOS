#include <system/Processor.hpp>
#include <system/System.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <processes/Process.hpp>

Processor::Processor(size_t id, Status &status):
        CommonProcessor(id, status),
        tss(status),
        localAPIC(readModelSpecificRegister(MSR::IA32_APIC_BASE), status) {

    if (!status) {
        return;
    }

    CurrentSystem.gdt.setupTSS(id, &tss);
}

__attribute__((noreturn))
void Processor::returnToUserMode() {
    Thread *thread = CurrentProcessor->scheduler.activeThread();
    // Idle thread does not have a process
    if (thread->process) {
        thread->process->addressSpace.activate();
    }
    CurrentSystem.gdt.resetTSS(CurrentProcessor->id);
    tss.update(thread);
    returnFromInterrupt(&thread->registerState, thread->threadLocalStorage());
}
