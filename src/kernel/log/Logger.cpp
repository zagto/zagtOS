#include <common/common.hpp>
#include <setup/BootInfo.hpp>
#include <log/SerialBackend.hpp>
#include <log/FramebufferBackend.hpp>
#include <system/System.hpp>


Logger cout;

static SerialBackend serialBackend;
static FramebufferBackend framebufferBackend;

static Lock logLock;


void Logger::init(const BootInfo *bootInfo) {
    serialBackend.init();
    framebufferBackend.init(&bootInfo->framebufferInfo);
}


void Logger::flush() {
    LockHolder lh(logLock);

    char *buffer = CurrentProcessor->logBuffer;
    for (size_t i = 0; i < CurrentProcessor->logBufferIndex; i++) {
        serialBackend.write(buffer[i]);
        framebufferBackend.write(buffer[i]);
    }
    CurrentProcessor->logBufferIndex = 0;
}


Logger Logger::operator<<(char character) {
    /* On early boot there is no Processor object, write unbuffered in this case */
    if (CurrentProcessor == nullptr) {
        serialBackend.write(character);
        framebufferBackend.write(character);
    } else {
        char *buffer = CurrentProcessor->logBuffer;
        size_t &index = CurrentProcessor->logBufferIndex;

        buffer[index] = character;
        index++;

        if (character == '\n' || CurrentProcessor->logBufferIndex == Processor::LOG_BUFFER_SIZE) {
            flush();
        }
    }
    return *this;
}


Logger Logger::operator<<(const char *string) {
    while (*string != '\0') {
        *this << *string;
        string++;
    }
    return *this;
}


Logger Logger::operator<<(uint64_t value) {
    *this << "0x";
    for (int shift = sizeof(uint64_t) * 2 - 1; shift >= 0; shift--) {
        size_t part = (value >> (shift * 4)) & 0xf;
        if (part < 10) {
            *this << static_cast<char>(part + '0');
        } else {
            *this << static_cast<char>(part - 10 + 'a');
        }
    }
    return *this;
}


Logger Logger::operator<<(uint32_t value) {
    *this << "0x";
    for (int shift = sizeof(uint32_t) * 2 - 1; shift >= 0; shift--) {
        size_t part = (value >> (shift * 4)) & 0xf;
        if (part < 10) {
            *this << static_cast<char>(part + '0');
        } else {
            *this << static_cast<char>(part - 10 + 'a');
        }
    }
    return *this;
}


Logger Logger::operator<<(volatile void *pointer) {
    return *this << reinterpret_cast<size_t>(pointer);
}


Logger Logger::operator<<(const void *pointer) {
    return *this << reinterpret_cast<size_t>(pointer);
}


Logger Logger::operator<<(void *pointer) {
    return *this << reinterpret_cast<size_t>(pointer);
}

Logger Logger::operator<<(const RegisterState &regs) {
    return *this << "[" << endl
                 << "\tRIP=" << regs.rip << ", RSP=" << regs.rsp << endl
                 << "\tCS=" << regs.cs << ", SS=" << regs.ss << endl
                 << "\tRDI=" << regs.rdi << ", RSI=" << regs.rsi << endl
                 << "\tRAX=" << regs.rax << ", RBX=" << regs.rbx << endl
                 << "\tRCX=" << regs.rcx << ", RDX=" << regs.rdx << endl
                 << "\tR12=" << regs.r12 << ", R13=" << regs.r13 << endl
                 << "\tR14=" << regs.r14 << ", R15=" << regs.r15 << endl
                 << "\tinterrupt type: " << regs.intNr << ", error code: " << regs.errorCode << endl
                 << "]";
}
