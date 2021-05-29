#pragma once

#include <common/common.hpp>
#include <system/CommonSystem.hpp>

#define SYSTEM_X86_64 1

static constexpr size_t MAX_NUM_PROCESSORS = 256;

class System : public CommonSystem {
private:
    /* only used during initial handover state deserialization */
    Status handOverStatus{Status::OK()};

public:
    PhysicalAddress ACPIRoot;

    System();
};

extern System CurrentSystem;
extern hos_v1::System *_HandOverSystem;
