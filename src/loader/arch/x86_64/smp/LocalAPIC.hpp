#pragma once

#include <common/common.hpp>

class LocalAPIC {
private:
    enum class DeliveryMode : uint32_t {
        FIXED, LOWEST_PRIORITY, SMI, RESERVED1, NMI, INIT, STARTUP, RESERVED2
    };
    enum class TriggerMode : uint32_t {
        EDGE, LEVEL
    };
    enum class Level : uint32_t {
        DEASSERT, ASSERT
    };

    uint8_t *map;

    void setupMap(PhysicalAddress base);
    void wirteInterruptControlRegister(DeliveryMode deliveryMode,
                                       Level level,
                                       TriggerMode triggerMode,
                                       uint32_t destination,
                                       uint8_t vector);

protected:
    /* These values are offsets in the memory-mapped APIC */
    enum class Register : size_t {
        TASK_PRIORITY = 0x080,
        SPURIOUS_INTERRUPT_VECTOR = 0x0f0,
        INTERRUPT_COMMAND_LOW = 0x300,
        INTERRUPT_COMMAND_HIGH = 0x310,
        LVT_TIMER = 0x320,
        LVT_THERMAL = 0x330,
        LVT_PERFORMANCE_COUNTER = 0x340,
        LVT_REGULATOR_INTTERRUPTS = 0x350,
        LVT_NON_MASKABLE_INTERRUPTS = 0x360,
        LVT_ERROR = 0x370,
        INITIAL_COUNT = 0x380,
        CURRENT_COUNT = 0x390,
        DIVIDE_CONFIGURATION = 0x3e0,
        END = 0x3f0
    };

    void writeRegister(Register reg, uint32_t value);
    uint32_t readRegister(Register reg);

public:
    LocalAPIC(PhysicalAddress base);
    ~LocalAPIC();

    void sendInit(uint32_t apicID);
    void sendStartup(uint32_t apicID, PhysicalAddress entry);
};
