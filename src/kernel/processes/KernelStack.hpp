#pragma once

#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>
#include <lib/Status.hpp>

class KernelStack {
private:
    static constexpr size_t SIZE = 0x1000;

    uint8_t *data{nullptr};

public:
    KernelStack(RegisterState userState, RegisterState kernelState, Status &status);

    RegisterState *userRegisterState();
};
