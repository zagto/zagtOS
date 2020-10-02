#include <Output.hpp>
#include <Serial.hpp>
#include <EFI.hpp>
#include <log/Logger.hpp>

#define DEBUG_LOADER 1


/* After ExitBootServices, the Log will be printed directly to Serial Port, without using UEFI */
static bool logUseSerial{false};


#ifdef DEBUG_LOADER
void Output(char character) {
    efi::CHAR16 string[] = {static_cast<efi::CHAR16>(character), '\0'};

    if (logUseSerial) {
        WriteSerial(character);
    } else {
        efi::Print(string);
    }
}
#else
void Output(char) {
    // do nothing
}
#endif

#ifdef DEBUG_LOADER
void OutputExitBootServices() {
    InitSerial();
    logUseSerial = true;
    cout << "Log is now written without EFI Boot Services" << endl;
}
#else
void LogExitBootServices(void) {
    // do nothing
}
#endif
