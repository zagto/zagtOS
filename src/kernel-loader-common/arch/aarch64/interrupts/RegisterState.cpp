#include <common/utils.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/ArchRegions.hpp>
#include <iostream>


RegisterState::RegisterState() noexcept {
    memset(this, 0, sizeof(RegisterState));
}

RegisterState::RegisterState(UserVirtualAddress entry,
                             UserVirtualAddress stackPointer,
                             size_t entryArgument) noexcept
    : RegisterState() {

    sp = stackPointer.value();
    /* stack-pointer needs to be aligned on aarch64 */
    while (sp % 16 != 0) {
        sp--;
    }
    x[0] = entryArgument;

    if (entry.isKernel()) {
        pstate = FLAG_EL1H;
    } else {
        fromUser = true;
    }
    pc = entry.value();
    pstate |= FLAG_INTERRUPTS;
}

Logger &operator<<(Logger &logger, const RegisterState &regs) {
    logger << "[" << endl
           << "\tPC=" << regs.pc << ", SP=" << regs.sp << endl
           << "\tPSTATE=" << regs.pstate << endl;

    for (size_t index = 0; index < 10; index += 2) {
        logger << "\tX" << static_cast<char>('0'+index) << "=" << regs.x[index]
               << ", X" << static_cast<char>('1'+index) << "=" << regs.x[index+1] << endl;
    }
    for (size_t index = 10; index < 30; index += 2) {
        logger << "\tX" << static_cast<char>('0'+index/10) << static_cast<char>('0'+index%10)
               << "=" << regs.x[index]
               << ", X" << static_cast<char>('0'+index/10) << static_cast<char>('1'+index%10)
               << "=" << regs.x[index+1] << endl;
    }

    logger << "\texception type: " << regs.exceptionType << ", exception syndrome: "
           << regs.exceptionSyndrome << endl
           << "]";
    return logger;
}
