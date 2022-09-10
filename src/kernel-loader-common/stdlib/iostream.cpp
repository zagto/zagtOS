#include <iostream>
#include <setup/HandOverState.hpp>
#include <log/BasicLog.hpp>

/* kernel defines cout in Globals.hpp to ensure initialization order */
#ifdef ZAGTOS_LOADER
Logger cout;
#endif

void Logger::basicWrite(char character) {
    basicLog::write(character);
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
    using hos_v1::Permissions;
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
