#ifndef REGISTERSTATE_HPP
#define REGISTERSTATE_HPP

#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <common/panic.hpp>

class alignas(16) RegisterState
{
public:
    static const u64 FLAG_USER_IOPL{(3 << 12)};
    static const u64 FLAG_INTERRUPTS{(1 << 9)};

    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi;
    u64 rdx, rcx, rbx, rax;
    u64 intNr, errorCode;
    u64 rip, cs, rflags, rsp, ss;

    RegisterState(VirtualAddress entry,
                  UserVirtualAddress stackPointer,
                  UserVirtualAddress tlsBase,
                  UserVirtualAddress masterTLSBase,
                  usize tlsSize);

    inline u32 syscallNr() {
        return rdi;
    }
    inline u64 syscallParameter(usize index) {
        switch (index) {
        case 0:
            return rsi;
        case 1:
            return rdx;
        case 2:
            return rcx;
        case 3:
            return r8;
        case 4:
            return r9;
        default:
            Panic();
        }
    }
    inline usize stackPointer() {
        return rsp;
    }
    inline void setSyscallResult(usize value) {
        rax = value;
    }
};

#endif // REGISTERSTATE_HPP
