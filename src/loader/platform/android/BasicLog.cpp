#include <log/BasicLog.hpp>
#include <log/FramebufferBackend.hpp>
#include <Framebuffer.hpp>
#include <iostream>

namespace basicLog {

static bool framebufferInitialized{false};

static FramebufferBackend framebufferBackend;

void init() {
    auto &framebufferInfo = GetFramebuffer();
    framebufferBackend.init(framebufferInfo);
    framebufferInitialized = true;
}

void write(char character) {
    if (framebufferInitialized) {
        framebufferBackend.write(character);
    }
}

}
