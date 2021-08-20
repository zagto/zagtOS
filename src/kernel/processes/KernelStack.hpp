#pragma once

#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>
#include <lib/Status.hpp>

class KernelStack {
private:
    uint8_t *data{nullptr};

public:
    KernelStack(Status &status);

    RegisterState *userRegisterState();
};
