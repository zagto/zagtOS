#include <system/Processor.hpp>
#include <system/System.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <processes/Process.hpp>

Processor::Processor():
        CommonProcessor(),
        tss(),
        localAPIC() {
    CurrentSystem.gdt.setupTSS(id, &tss);
}

void Processor::localInitialization() noexcept {
    localAPIC.initialize(readModelSpecificRegister(MSR::IA32_APIC_BASE) & 0xfffff000);
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

    CurrentSystem.gdt.resetTSS(id);
    tss.update(thread);
    writeModelSpecificRegister(MSR::FSBASE, thread->tlsPointer);
    returnFromInterrupt(thread->kernelStack->userRegisterState());
}

[[noreturn]] void Processor::returnInsideKernelMode(RegisterState *state) noexcept {
    returnFromInterrupt(state);
}

void Processor::sendIPI(IPI ipi) noexcept {
    cout << "sending IPI " << static_cast<uint32_t>(ipi) << " from " <<  CurrentProcessor()->hardwareID << " to " << hardwareID << endl;

    __atomic_or_fetch(&ipiFlags, ipi, __ATOMIC_SEQ_CST);
    CurrentProcessor()->localAPIC.sendIPI(hardwareID);
}

void Processor::endOfInterrupt() noexcept {
    assert(KernelInterruptsLock.isLocked());
    assert(CurrentProcessor() == this);

    localAPIC.endOfInterrupt();
}

void InitCurrentProcessorPointer(Processor *processor) {
    writeModelSpecificRegister(MSR::KERNEL_GSBASE, reinterpret_cast<uint64_t>(processor));
    asm volatile("swapgs");
}

bool ProcessorsInitialized = false;
