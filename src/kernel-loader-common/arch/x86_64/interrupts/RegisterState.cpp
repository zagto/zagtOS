#include <common/utils.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/ArchRegions.hpp>


RegisterState::RegisterState() noexcept {
    memset(this, 0, sizeof(RegisterState));
}

RegisterState::RegisterState(UserVirtualAddress entry,
                             UserVirtualAddress stackPointer,
                             size_t entryArgument) noexcept
    : RegisterState() {

    rsp = stackPointer.value();
    /* stack-pointer needs to be misaligned on x86_64 */
    while (rsp % 16 != 8) {
        rsp--;
    }
    rdi = entryArgument;
    r8 = 1;

    if (entry.isKernel()) {
        cs = 0x08;
        ss = 0x10;
    } else {
        cs = 0x20 | 3;
        ss = 0x18 | 3;
        rflags = FLAG_USER_IOPL;
    }
    rip = entry.value();
    rflags |= FLAG_INTERRUPTS;
}

Logger &operator<<(Logger &logger, const RegisterState &registerState) {
    return *this << "[" << endl
                 << "\tRIP=" << regs.rip << ", RSP=" << regs.rsp << endl
                 << "\tRBP=" << regs.rbp << endl
                 << "\tCS=" << regs.cs << ", SS=" << regs.ss << endl
                 << "\tRDI=" << regs.rdi << ", RSI=" << regs.rsi << endl
                 << "\tRAX=" << regs.rax << ", RBX=" << regs.rbx << endl
                 << "\tRCX=" << regs.rcx << ", RDX=" << regs.rdx << endl
                 << "\tR8=" << regs.r8 << ", R9=" << regs.r9 << endl
                 << "\tR10=" << regs.r10 << ", R11=" << regs.r11 << endl
                 << "\tR12=" << regs.r12 << ", R13=" << regs.r13 << endl
                 << "\tR14=" << regs.r14 << ", R15=" << regs.r15 << endl
                 << "\tinterrupt type: " << regs.intNr << ", error code: " << regs.errorCode << endl
                 << "]";
}
