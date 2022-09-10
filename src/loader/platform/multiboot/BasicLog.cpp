#include <log/BasicLog.hpp>
#include <log/SerialBackend.hpp>
#include <log/FramebufferBackend.hpp>
#include <Framebuffer.hpp>
#include <iostream>

namespace basicLog {

static bool serialInitialized{false};
static bool framebufferInitialized{false};

static SerialBackend serialBackend;
static FramebufferBackend framebufferBackend;

void init() {
    auto &framebufferInfo = GetFramebuffer();
    framebufferBackend.init(framebufferInfo);
    framebufferInitialized = true;
}

void write(char character) {
    /* the loader may try to call write before the the framebuffer is initialized. In this case
     * we can already output to the serial port */
    if (!serialInitialized) {
        serialBackend.init();
        serialInitialized = true;
    }
    serialBackend.write(character);
    if (framebufferInitialized) {
        framebufferBackend.write(character);
    }
}

void exitBootServices() {
    serialBackend.init();
    auto &framebufferInfo = GetFramebuffer();
    framebufferBackend.init(framebufferInfo);
    cout << "Log is now written without EFI Boot Services" << endl;
}
}
