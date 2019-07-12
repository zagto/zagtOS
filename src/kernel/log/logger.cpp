#include <common/common.hpp>
#include <setup/BootInfo.hpp>
#include <log/serialbackend.hpp>
#include <log/framebufferbackend.hpp>

using namespace log;


Logger log::Log;

static SerialBackend serialBackend;
static FramebufferBackend framebufferBackend;


void Logger::init(const BootInfo *bootInfo) {
    serialBackend.init();
    framebufferBackend.init(&bootInfo->framebufferInfo);
}


Logger Logger::operator<<(char character) {
    serialBackend.write(character);
    framebufferBackend.write(character);
    return *this;
}


Logger Logger::operator<<(const char *string) {
    while (*string != '\0') {
        *this << *string;
        string++;
    }
    return *this;
}


Logger Logger::operator<<(usize value) {
    *this << "0x";
    for (int shift = sizeof(usize) * 2 - 1; shift >= 0; shift--) {
        usize part = (value >> (shift * 4)) & 0xf;
        if (part < 10) {
            *this << static_cast<char>(part + '0');
        } else {
            *this << static_cast<char>(part - 10 + 'a');
        }
    }
    return *this;
}


Logger Logger::operator<<(volatile void *pointer) {
    return *this << reinterpret_cast<usize>(pointer);
}


Logger Logger::operator<<(const void *pointer) {
    return *this << reinterpret_cast<usize>(pointer);
}


Logger Logger::operator<<(void *pointer) {
    return *this << reinterpret_cast<usize>(pointer);
}

Logger Logger::operator<<(const RegisterState &regs) {
    return *this << "[" << EndLine
                 << "\tRIP=" << regs.rip << ", RSP=" << regs.rsp << EndLine
                 << "\tCS=" << regs.cs << ", SS=" << regs.ss << EndLine
                 << "\tRDI=" << regs.rdi << ", RSI=" << regs.rsi << EndLine
                 << "\tRAX=" << regs.rax << ", RBX=" << regs.rbx << EndLine
                 << "\tRCX=" << regs.rcx << ", RDX=" << regs.rdx << EndLine
                 << "\tR12=" << regs.r12 << ", R13=" << regs.r13 << EndLine
                 << "\tR14=" << regs.r14 << ", R15=" << regs.r15 << EndLine
                 << "\tinterrupt type: " << regs.intNr << ", error code: " << regs.errorCode << EndLine
                 << "]";
}
