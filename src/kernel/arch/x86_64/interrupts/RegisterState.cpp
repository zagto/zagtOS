#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/ArchRegions.hpp>


RegisterState::RegisterState() {
    memset(this, 0, sizeof(RegisterState));
}

RegisterState::RegisterState(UserVirtualAddress entry,
                             UserVirtualAddress stackPointer,
                             size_t entryArgument)
    : RegisterState() {

    rsp = stackPointer.value();
    /* stack-pointer needs to be misaligned on x86_64 */
    while (rsp % 16 != 8) {
        rsp--;
    }
    rdi = entryArgument;
    r8 = 1;

    if (entry.isKernel()) {
        cs = 0x08;
        ss = 0x10;
    } else {
        cs = 0x20 | 3;
        ss = 0x18 | 3;
        rflags = FLAG_USER_IOPL;
    }
    rip = entry.value();
    rflags |= FLAG_INTERRUPTS;
}
