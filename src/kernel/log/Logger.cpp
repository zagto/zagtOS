#include <common/common.hpp>
#include <log/SerialBackend.hpp>
#include <log/FramebufferBackend.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>


#ifdef ZAGTOS_LOADER
Logger cout;
#endif

static SerialBackend serialBackend;
static FramebufferBackend framebufferBackend;

static SpinLock logLock;


Logger::Logger() {
    serialBackend.init();
    framebufferBackend.init(_HandOverSystem->framebufferInfo);
    setKernelColor();
}


void Logger::flush() {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(logLock);

    char *buffer = CurrentProcessor->logBuffer;
    for (size_t i = 0; i < CurrentProcessor->logBufferIndex; i++) {
        output(buffer[i]);
    }
    CurrentProcessor->logBufferIndex = 0;
}

void Logger::output(char character) {
    switch (character) {
    case 1:
        serialBackend.setKernelColor();
        framebufferBackend.setKernelColor();
        break;
    case 2:
        serialBackend.setProgramNameColor();
        framebufferBackend.setProgramNameColor();
        break;
    case 3:
        serialBackend.setProgramColor();
        framebufferBackend.setProgramColor();
        break;
    default:
        serialBackend.write(character);
        framebufferBackend.write(character);
    }
}

void Logger::setKernelColor() {
    basicWrite(1);
}

void Logger::setProgramNameColor() {
    basicWrite(2);
}

void Logger::setProgramColor() {
    basicWrite(3);
}


void Logger::basicWrite(char character) {
    /* On early boot there is no Processor object, write unbuffered in this case */
    if (CurrentProcessor == nullptr) {
        scoped_lock lg1(KernelInterruptsLock);
        scoped_lock lg(logLock);
        output(character);
    } else {
        char *buffer = CurrentProcessor->logBuffer;
        size_t &index = CurrentProcessor->logBufferIndex;

        buffer[index] = character;
        index++;

        if (character == '\n' || index == Processor::LOG_BUFFER_SIZE) {
            flush();
        }
    }
}

Logger &Logger::operator<<(char character) {
    if (character >= ' ' || character == '\n') {
        basicWrite(character);
    }
    return *this;
}

Logger &Logger::operator<<(const char *string) {
    while (*string != '\0') {
        *this << *string;
        string++;
    }
    return *this;
}

Logger &Logger::operator<<(uint64_t value) {
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

Logger &Logger::operator<<(uint32_t value) {
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

Logger &Logger::operator<<(uint16_t value) {
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

Logger &Logger::operator<<(volatile void *pointer) {
    return *this << reinterpret_cast<size_t>(pointer);
}

Logger &Logger::operator<<(const void *pointer) {
    return *this << reinterpret_cast<size_t>(pointer);
}

Logger &Logger::operator<<(void *pointer) {
    return *this << reinterpret_cast<size_t>(pointer);
}

Logger &Logger::operator<<(const RegisterState &regs) {
    return *this << "[" << endl
                 << "\tRIP=" << regs.rip << ", RSP=" << regs.rsp << endl
                 << "\tRBP=" << regs.rbp << endl
                 << "\tCS=" << regs.cs << ", SS=" << regs.ss << endl
                 << "\tRDI=" << regs.rdi << ", RSI=" << regs.rsi << endl
                 << "\tRAX=" << regs.rax << ", RBX=" << regs.rbx << endl
                 << "\tRCX=" << regs.rcx << ", RDX=" << regs.rdx << endl
                 << "\tR8=" << regs.r8 << ", R9=" << regs.r9 << endl
                 << "\tR10=" << regs.r10 << ", R11=" << regs.r11 << endl
                 << "\tR12=" << regs.r12 << ", R13=" << regs.r13 << endl
                 << "\tR14=" << regs.r14 << ", R15=" << regs.r15 << endl
                 << "\tinterrupt type: " << regs.intNr << ", error code: " << regs.errorCode << endl
                 << "]";
}

Logger &Logger::operator<<(hos_v1::Permissions permissions) {
    switch (permissions) {
    case Permissions::INVALID:
        return *this << "Invalid";
    case Permissions::READ:
        return *this << "Read-only";
    case Permissions::READ_EXECUTE:
        return *this << "Read/Execute";
    case Permissions::READ_WRITE:
        return *this << "Read/Write";
    case Permissions::READ_WRITE_EXECUTE:
        return *this << "Read/Write/Execute";
    default:
        *this << " -- Unknown Permission Value" << endl;
        Panic();
    }
}

void Logger::sendCoreDump(size_t nameLength,
                          const uint8_t *name,
                          size_t dataLength,
                          const uint8_t *data) {
    cout << "sending coredump data size " << dataLength << endl;
    flush();

    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(logLock);
    /* core dump marker */
    serialBackend.write(static_cast<char>(0xf2));
    for (size_t i = 0; i < 8; i++) {
        serialBackend.write((nameLength >> i * 8) & 0xff);
    }
    for (size_t i = 0; i < nameLength; i++) {
        serialBackend.write(name[i]);
    }
    for (size_t i = 0; i < 8; i++) {
        serialBackend.write((dataLength >> i * 8) & 0xff);
    }
    for (size_t i = 0; i < dataLength; i++) {
        serialBackend.write(data[i]);
    }
}

