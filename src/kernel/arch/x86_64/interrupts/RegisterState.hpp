#pragma once
#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <common/panic.hpp>

class alignas(16) RegisterState
{
public:
    static const uint64_t FLAG_USER_IOPL{(3 << 12)};
    static const uint64_t FLAG_INTERRUPTS{(1 << 9)};

    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi;
    uint64_t rdx, rcx, rbx, rax;
    uint64_t intNr, errorCode;
    uint64_t rip, cs, rflags, rsp, ss;

    RegisterState(VirtualAddress entry,
                  UserVirtualAddress stackPointer,
                  UserVirtualAddress tlsBase,
                  UserVirtualAddress masterTLSBase,
                  size_t tlsSize);

    inline uint32_t syscallNumber() const {
        return static_cast<uint32_t>(rdi);
    }
    inline uint64_t syscallParameter(size_t index) const {
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
    inline size_t stackPointer() const {
        return rsp;
    }
    inline void setSyscallResult(size_t value) {
        rax = value;
    }
    inline void setThreadHandle(uint32_t value) {
        r8 = value;
    }
};
