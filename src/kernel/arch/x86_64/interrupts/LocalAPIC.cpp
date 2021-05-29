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

LocalAPIC::LocalAPIC(PhysicalAddress base, Status &status) :
        timer(this) {

    if (!status) {
        return;
    }

    status = setupMap(base);
    if (!status) {
        return;
    }

    /* initialization sequence from:
     * http://www.osdever.net/tutorials/view/advanced-programming-interrupt-controller */
    writeRegister(Register::TASK_PRIORITY, 0x20);
    writeRegister(Register::LVT_TIMER, 0x10000);
    writeRegister(Register::LVT_PERFORMANCE_COUNTER, 0x10000);
    writeRegister(Register::LVT_REGULATOR_INTTERRUPTS, 0x08700);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x00400);
    writeRegister(Register::LVT_ERROR, 0x10000);

    /* enables the APIC and sets spurious interrupts vector.
     * make spurious interrupts use 0x20 the same vector they use on the legacy PIC, so we can
     * ignore them all the same way */
    writeRegister(Register::SPURIOUS_INTERRUPT_VECTOR, 0x120);

    writeRegister(Register::LVT_REGULATOR_INTTERRUPTS, 0x08700);
    writeRegister(Register::LVT_NON_MASKABLE_INTERRUPTS, 0x00400);
}


LocalAPIC::~LocalAPIC() {
    Panic();
}
