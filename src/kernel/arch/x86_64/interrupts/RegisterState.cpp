#include <common/common.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory/ArchRegions.hpp>


RegisterState::RegisterState() {
    memset(this, 0, sizeof(RegisterState));
    self = this;
}

RegisterState::RegisterState(const RegisterState &other) {
    memcpy(this, &other, sizeof(RegisterState));
    self = this;
}
RegisterState &RegisterState::operator=(const RegisterState &other) {
    memcpy(this, &other, sizeof(RegisterState));
    self = this;
    return *this;
}

RegisterState::RegisterState(UserVirtualAddress entry,
                             UserVirtualAddress stackPointer,
                             UserVirtualAddress runMessageAddress,
                             UserVirtualAddress tlsBase,
                             UserVirtualAddress masterTLSBase,
                             size_t tlsSize)
    : RegisterState() {

    rsp = stackPointer.value();
    /* stack-pointer needs to be misaligned on x86_64 */
    while (rsp % 16 != 8) {
        rsp--;
    }
    rdi = runMessageAddress.value();
    cout << "creating RegisterState with tlsBase " << tlsBase.value() << endl;
    rsi = tlsBase.value();
    rdx = masterTLSBase.value();
    rcx = tlsSize;
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
