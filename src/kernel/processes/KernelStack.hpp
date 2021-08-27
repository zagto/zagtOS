#pragma once

#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>
#include <lib/Status.hpp>
#include <system/System.hpp>

class KernelStack {
private:
    static constexpr size_t SIZE = 0x1000;
    static constexpr size_t REGISTER_STATE_ALIGNMENT = 16;
    static constexpr size_t USER_STATE_OFFSET = SIZE - sizeof(RegisterState)
            - (SIZE - sizeof(RegisterState)) % REGISTER_STATE_ALIGNMENT;
#ifdef SYSTEM_X86_64
    /* x86_64 expects a mis-aligned by 8 stack pointer */
    static constexpr size_t INITIAL_KERNEL_STACK_POINTER_OFFSET = USER_STATE_OFFSET - 8;
#else
#error "add your system architecture here"
#endif

    void *data{nullptr};

public:
    KernelStack(RegisterState userRegisterState, Status &status);
    ~KernelStack();

    RegisterState *userRegisterState();
    [[noreturn]] void switchToKernelEntry(void kernelEntry(void *), void *argument);
};
