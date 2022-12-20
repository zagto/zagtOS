#include <log/BasicLog.hpp>
#include <log/FramebufferBackend.hpp>
#include <log/SerialBackend.hpp>
#include <Framebuffer.hpp>
#include <Serial.hpp>
#include <iostream>

namespace basicLog {

static bool backendsInitialized{false};

static FramebufferBackend framebufferBackend;
static SerialBackend serialBackend;

void init(hos_v1::SerialInfo &serial, hos_v1::FramebufferInfo &framebuffer) {
    serialBackend.init(serial);
    framebufferBackend.init(framebuffer);
    backendsInitialized = true;
}

void write(char character) {
    if (backendsInitialized) {
        serialBackend.write(character);
        framebufferBackend.write(character);
    }
}

}
