#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <system/Processor.hpp>


hos_v1::System *_HandOverSystem;

System::System() :
        CommonSystem(*_HandOverSystem),
        gdt(handOverStatus),
        ACPIRoot{_HandOverSystem->firmwareRoot} {
    if (!handOverStatus) {
        cout << "Exception during System initialization" << endl;
        Panic();
    }
    /* TODO: support for Intel PCIDs could be added here */
    tlbContextsPerProcessor = 1;
}

void System::setupCurrentProcessor() {
    gdt.load();
    idt.load();
    loadTaskStateSegment(CurrentProcessor->id);
    setupSyscalls();
}

void System::setupSyscalls() {
    cout << "registering syscall entry " << reinterpret_cast<uint64_t>(&syscallEntry) << endl;
    writeModelSpecificRegister(MSR::LSTAR, reinterpret_cast<uint64_t>(&syscallEntry));
    writeModelSpecificRegister(MSR::STAR, (static_cast<uint64_t>(0x08) << 32) | (static_cast<uint64_t>(0x10) << 48));
    writeModelSpecificRegister(MSR::SFMASK, RegisterState::FLAG_INTERRUPTS | RegisterState::FLAG_USER_IOPL);
}
