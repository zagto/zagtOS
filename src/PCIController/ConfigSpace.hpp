#pragma once

#include <cstddef>
#include <zagtos/Register.hpp>

struct ConfigSpace {
    REGISTERSI(0x1000/4,
               BITSI(vendorID,0,0,16) BITSI(deviceID,0,16,16)
               BITI(IOSpaceEnable,1,0) BITI(busMasterEnable,1,0) BITI(memorySpaceEnable,1,0) BITI(interruptDisable,1,0)
               BITSI(status,1,16,16)
               /*BITSI(revisionID,2,0,8) BITSI(progIF,2,8,8) BITSI(subclass,2,16,8) BITSI(classCode,2,24,8)*/
               BITSI(classCodeProgIFRevisionID,2,0,32)

               BITSI(headerType,3,16,8)
               )

    uint32_t BAR(size_t index) {
        assert(index < 6);
        return atIndex(index+4);
    }
    void BAR(size_t index, uint32_t value) {
        assert(index < 6);
        atIndex(index+4, value);
    }
};

static_assert(sizeof(ConfigSpace) == 0x1000);
