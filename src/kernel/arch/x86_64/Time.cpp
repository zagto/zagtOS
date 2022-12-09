#include <time/Time.hpp>
#include <common/ModelSpecificRegister.hpp>

void setTimer(uint64_t value) noexcept {
    writeModelSpecificRegister(MSR::TSC_DEADLINE, value);
}
