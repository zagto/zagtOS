#include <log/BasicLog.hpp>
#include <log/SerialBackend.hpp>
#include <log/FramebufferBackend.hpp>
#include <Framebuffer.hpp>
#include <EFI.hpp>
#include <iostream>

namespace basicLog {

/* After ExitBootServices, the Log will be printed directly to Serial Port, without using UEFI */
static bool logUseBootServices{true};

static SerialBackend serialBackend;
static FramebufferBackend framebufferBackend;

void init(hos_v1::SerialInfo &, hos_v1::FramebufferInfo &) {
    /* do nothing - internal log backends are initialized later once exitBootServices is called */
}

void write(char character) {
    efi::CHAR16 string[] = {static_cast<efi::CHAR16>(character), '\0'};

    if (logUseBootServices) {
        efi::Print(string);
    } else {
        serialBackend.write(character);
        framebufferBackend.write(character);
    }
}

void exitBootServices(const hos_v1::SerialInfo &serial,
                      const hos_v1::FramebufferInfo &framebuffer) {
    serialBackend.init(serial);
    framebufferBackend.init(framebuffer);
    logUseBootServices = false;
    cout << "Log is now written without EFI Boot Services" << endl;
}
}
