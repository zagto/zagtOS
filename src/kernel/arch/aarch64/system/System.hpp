#pragma once

#include <common/common.hpp>
#include <system/CommonSystem.hpp>

#define SYSTEM_AARCH64 1

static constexpr size_t MAX_NUM_PROCESSORS = 256;

class BoundInterrupt;

class System : public CommonSystem {
private:
    friend class Processor;
    friend __attribute__((noreturn)) void _handleInterrupt(RegisterState *registerState);

public:
    /* Called directly at the beginning, before Processors management is set up */
    System();
    /* Called once on each Processor once there it is on a good stack */
    void setupCurrentProcessor() noexcept;
    /* Called afterwards */
    void lateInitialization();

    void bindInterrupt(BoundInterrupt &boundInterrupt);
    void unbindInterrupt(BoundInterrupt &boundInterrupt);
    void interruptFullyProcessed(BoundInterrupt &boundInterrupt);
};

extern System CurrentSystem;
extern hos_v1::System *_HandOverSystem;
