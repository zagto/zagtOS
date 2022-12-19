#pragma once
#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <common/panic.hpp>

class Processor;

/* Although in a separate file, the register state is part of the kernel handover interface and
 * chaging it means introducing a new inteface */
class RegisterState {
public:
    static const uint64_t FLAG_USER_IOPL{(3 << 12)};
    static const uint64_t FLAG_INTERRUPTS{(1 << 9)};

    /* if state was saved from syscall, less registers need to be restored */
    bool fromSyscall;

    /* keep things 16-byte aligned */
    uint64_t dummy;

    /* callee-saved */
    uint64_t r15, r14, r13, r12, rbp, rbx;

    uint64_t r11, r10, r9, r8;
    uint64_t rdi, rsi;
    uint64_t rdx, rcx, rax;
    uint64_t intNr, errorCode;
    uint64_t rip, cs, rflags, rsp, ss;

    RegisterState() noexcept;
    RegisterState(UserVirtualAddress entry,
                  UserVirtualAddress stackPointer,
                  size_t entryArgument) noexcept;
    RegisterState(KernelVirtualAddress entry,
                  KernelVirtualAddress stackPointer) noexcept;

    inline size_t stackPointer() const noexcept {
        return rsp;
    }
    inline void setSyscallResult(size_t value) noexcept {
        rax = value;
    }
    inline void setEntryArgument(uint32_t value) noexcept {
        rdi = value;
    }
    bool interruptsFlagSet() const noexcept {
        return rflags & FLAG_INTERRUPTS;
    }
};

Logger &operator<<(Logger &logger, const RegisterState &registerState);

class alignas(64) VectorRegisterState {
private:
    uint8_t data[1024];

public:
    void save();
    void restore();
};
