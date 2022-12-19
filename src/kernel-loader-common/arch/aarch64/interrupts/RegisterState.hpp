#pragma once
#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <common/panic.hpp>

class Processor;
class Logger;

/* Although in a separate file, the register state is part of the kernel handover interface and
 * chaging it means introducing a new inteface */
class RegisterState {
public:
    static constexpr uint64_t FLAG_INTERRUPTS = 1 << 7;
    static constexpr uint64_t FLAG_EL1 = 0b101;

    /* if state was saved from syscall, less registers need to be restored */
    bool fromSyscall;
    bool fromUser;
    uint64_t pstate;
    uint64_t pc;
    uint64_t sp;
    uint64_t exceptionType;
    uint64_t exceptionSyndrome;
    uint64_t x[30];

    RegisterState() noexcept;
    RegisterState(UserVirtualAddress entry,
                  UserVirtualAddress stackPointer,
                  size_t entryArgument) noexcept;
    RegisterState(KernelVirtualAddress entry,
                  KernelVirtualAddress stackPointer) noexcept;

    inline size_t stackPointer() const noexcept {
        return sp;
    }
    inline void setSyscallResult(size_t value) noexcept {
        x[0] = value;
    }
    inline void setEntryArgument(uint32_t value) noexcept {
        x[0] = value;
    }
    bool interruptsFlagSet() const noexcept {
        return pstate & FLAG_INTERRUPTS;
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
