#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/ArchRegions.hpp>


RegisterState::RegisterState(VirtualAddress entry,
                             UserVirtualAddress stackPtr,
                             UserVirtualAddress tlsBase,
                             UserVirtualAddress masterTLSBase,
                             size_t tlsSize)
{
    memset(this, 0, sizeof(RegisterState));
    rsp = UserSpaceRegion.end();
    if (stackPtr.value()) {
        rsp = stackPtr.value();
        /* rdi will point to data that may be passed on stack, without the stack alignment */
        rdi = rsp;
    }
    while (rsp % 16 != 8) {
        rsp--;
    }

    rsi = tlsBase.value();
    rdx = masterTLSBase.value();
    rcx = tlsSize;

    if (entry.isKernel()) {
        cs = 0x08;
        ss = 0x10;
    } else {
        cs = 0x18 | 3;
        ss = 0x20 | 3;
        rflags = FLAG_USER_IOPL;
    }
    rip = entry.value();
    rflags |= FLAG_INTERRUPTS;
}
