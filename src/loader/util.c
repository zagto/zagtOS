#include <efi.h>
#include <efilib.h>
#include <util.h>
#include <serial.h>


#define DEBUG_LOADER 1


// After ExitBootServices, the Log will be printed directly to Serial Port, without using UEFI
static BOOLEAN logUseSerial = FALSE;


__attribute__((noreturn)) void Halt(void) {
    while(1) {
        asm("hlt");
    }
}


#ifdef DEBUG_LOADER
static void LogCharacter(char c) {
    CHAR16 string[] = {c, '\0'};

    if (logUseSerial) {
        WriteSerial(c);
    } else {
        Print(string);
    }
}
#else
static void LogCharacter(char c) {
    // do nothing
}
#endif


void Log(const char *message) {
    while (*message) {
        LogCharacter(*message);
        message++;
    }
}


void Log16(const CHAR16 *message) {
    while (*message) {
        LogCharacter((char)*message);
        message++;
    }
}


#define SHIFT_MAX (sizeof(UINTN) * 2 - 1)
void LogUINTN(UINTN value) {
    UINTN shift;
    UINTN part;
    Log("0x");
    for (shift = 0; shift <= SHIFT_MAX; shift++) {
        part = (value >> ((SHIFT_MAX - shift) * 4)) & 0xf;
        if (part < 0xa) {
            LogCharacter('0' + (char)part);
        } else {
            LogCharacter('a' + (char)part - 0xa);
        }
    }
}


#define STATUS_CASE(symbol) \
    case symbol: \
        Log(#symbol); \
        break;


void LogStatus(const char *message, EFI_STATUS status) {
    Log(message);
    Log(": ");

    switch (status) {
        STATUS_CASE(EFI_SUCCESS)
        STATUS_CASE(EFI_LOAD_ERROR)
        STATUS_CASE(EFI_INVALID_PARAMETER)
        STATUS_CASE(EFI_UNSUPPORTED)
        STATUS_CASE(EFI_BAD_BUFFER_SIZE)
        STATUS_CASE(EFI_BUFFER_TOO_SMALL)
        STATUS_CASE(EFI_NOT_READY)
        STATUS_CASE(EFI_DEVICE_ERROR)
        STATUS_CASE(EFI_WRITE_PROTECTED)
        STATUS_CASE(EFI_OUT_OF_RESOURCES)
        STATUS_CASE(EFI_VOLUME_CORRUPTED)
        STATUS_CASE(EFI_VOLUME_FULL)
        STATUS_CASE(EFI_NO_MEDIA)
        STATUS_CASE(EFI_MEDIA_CHANGED)
        STATUS_CASE(EFI_NOT_FOUND)
        STATUS_CASE(EFI_ACCESS_DENIED)
        STATUS_CASE(EFI_NO_RESPONSE)
        STATUS_CASE(EFI_NO_MAPPING)
        STATUS_CASE(EFI_TIMEOUT)
        STATUS_CASE(EFI_NOT_STARTED)
        STATUS_CASE(EFI_ALREADY_STARTED)
        STATUS_CASE(EFI_ABORTED)
        STATUS_CASE(EFI_ICMP_ERROR)
        STATUS_CASE(EFI_TFTP_ERROR)
        STATUS_CASE(EFI_PROTOCOL_ERROR)
        STATUS_CASE(EFI_INCOMPATIBLE_VERSION)
        STATUS_CASE(EFI_SECURITY_VIOLATION)
        STATUS_CASE(EFI_CRC_ERROR)
        STATUS_CASE(EFI_END_OF_MEDIA)
        STATUS_CASE(EFI_END_OF_FILE)
        STATUS_CASE(EFI_WARN_UNKOWN_GLYPH)
        STATUS_CASE(EFI_WARN_DELETE_FAILURE)
        STATUS_CASE(EFI_WARN_WRITE_FAILURE)
        STATUS_CASE(EFI_WARN_BUFFER_TOO_SMALL)
    default:
         Log("Unknown Status");
    }
    Log("\n");
}


#ifdef DEBUG_LOADER
void LogExitBootServices(void) {
    InitSerial();
    logUseSerial = TRUE;
    Log("Log is now written without EFI Boot Services\n");
}
#else
void LogExitBootServices(void) {
    // do nothing
}
#endif
