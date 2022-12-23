#pragma once

#include <common/inttypes.hpp>
#include <setup/HandOverState.hpp>
#include <log/SerialBackend.hpp>
#include <log/FramebufferBackend.hpp>

namespace basicLog {

enum ControlCharacter {
    KERNEL_COLOR = 1, PROGRAM_NAME_COLOR, PROGRAM_COLOR
};

void init(hos_v1::SerialInfo &serial, hos_v1::FramebufferInfo &framebuffer);
void write(char character);

#ifdef ZAGTOS_LOADER
/* EFI-sepcific */
void exitBootServices(const hos_v1::SerialInfo &serial,
                      const hos_v1::FramebufferInfo &framebuffer);
#else
void sendCoreDump(size_t nameLength,
                  const uint8_t *name,
                  size_t dataLength,
                  const uint8_t *data);

extern SerialBackend serialBackend;
extern FramebufferBackend framebufferBackend;

#endif

}
