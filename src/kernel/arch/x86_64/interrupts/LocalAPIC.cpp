#include <interrupts/LocalAPIC.hpp>
#include <memory/KernelPageAllocator.hpp>

void LocalAPIC::writeRegister(Register reg, uint32_t value) {
    *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg)) = value;
}

uint32_t LocalAPIC::readRegister(Register reg) {
    return *reinterpret_cast<volatile uint32_t *>(map + static_cast<size_t>(reg));
}

Status LocalAPIC::setupMap(PhysicalAddress base) {
    assert(base.value() / PAGE_SIZE
           == (base.value() + static_cast<size_t>(Register::END) - 1) / PAGE_SIZE);
    PhysicalAddress physicalFrame = align(base.value(), PAGE_SIZE, AlignDirection::DOWN);
    scoped_lock sl(KernelPageAllocator.lock);
    Result<void *> virtualPage = KernelPageAllocator.map(PAGE_SIZE, false, &physicalFrame);
    if (!virtualPage) {
        return virtualPage.status();
    }

    map = static_cast<uint8_t *>(*virtualPage) + base.value() % PAGE_SIZE;
    return Status::OK();
}

void LocalAPIC::wirteInterruptControlRegister(DeliveryMode deliveryMode,
                                              Level level,
                                              TriggerMode triggerMode,
                                              uint32_t destination,
                                              uint8_t vector) {
    writeRegister(Register::INTERRUPT_COMMAND_HIGH, destination << (56 - 32));
    writeRegister(Register::INTERRUPT_COMMAND_LOW,
            vector
            | (static_cast<uint32_t>(deliveryMode) << 8)
            | (static_cast<uint32_t>(level) << 14)
            | (static_cast<uint32_t>(triggerMode) << 15));
}

void LocalAPIC::sendIPI(uint32_t apicID, uint8_t vector) {
    wirteInterruptControlRegister(DeliveryMode::FIXED,
                                  Level::ASSERT,
                                  TriggerMode::EDGE,
                                  apicID,
                                  vector);
    wirteInterruptControlRegister(DeliveryMode::FIXED,
                                  Level::DEASSERT,
                                  TriggerMode::EDGE,
                                  apicID,
                                  vector);
}

LocalAPIC::LocalAPIC(PhysicalAddress base, Status &status) :
        timer(this) {

    if (!status) {
        return;
    }

    status = setupMap(base);
    if (!status) {
        return;
    }
}


LocalAPIC::~LocalAPIC() {
    Panic();
}
