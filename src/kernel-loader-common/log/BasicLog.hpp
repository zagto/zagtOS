#pragma once

#include <common/inttypes.hpp>

namespace basicLog {

enum ControlCharacter {
    KERNEL_COLOR = 1, PROGRAM_NAME_COLOR, PROGRAM_COLOR
};

void init();
void write(char character);

#ifdef ZAGTOS_LOADER
/* EFI-sepcific */
void exitBootServices();
#else
void sendCoreDump(size_t nameLength,
                  const uint8_t *name,
                  size_t dataLength,
                  const uint8_t *data);
#endif

}
