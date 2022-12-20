#pragma once

#include <common/inttypes.hpp>
#include <setup/HandOverState.hpp>

namespace basicLog {

enum ControlCharacter {
    KERNEL_COLOR = 1, PROGRAM_NAME_COLOR, PROGRAM_COLOR
};

void init(hos_v1::SerialInfo &serial, hos_v1::FramebufferInfo &framebuffer);
void write(char character);

#ifdef ZAGTOS_LOADER
/* EFI-sepcific */
void exitBootServices(hos_v1::SerialInfo &serial, hos_v1::FramebufferInfo &framebuffer);
#else
void sendCoreDump(size_t nameLength,
                  const uint8_t *name,
                  size_t dataLength,
                  const uint8_t *data);
#endif

}
