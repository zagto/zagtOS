#pragma once

#include <cstddef>
#include <zagtos/Register.hpp>

struct Capability {
    REGISTER(header, BITS(id,0,8) BITS(nextIndex,10,6))
};

struct ExtendedCapability {
    REGISTER(header, BITS(id,0,16) BITS(version,16,4) BITS(nextOffset,20,12))
};

struct MSICapability {
    REGISTER(header, BITS(id,0,8) BITS(nextIndex,10,6) BIT(enable,16) BITS(queueMask,17,3) BITS(queueSize,20,3) BIT(is64bit,23))
};

struct MSICapability64 : MSICapability {
    REGISTER(addressLow)
    REGISTER(addressHigh)
    REGISTER(data)
    REGISTER(mask)
    REGISTER(pending)
};

struct MSICapability32 : MSICapability {
    REGISTER(address)
    REGISTER(data)
    REGISTER(reserved)
    REGISTER(mask)
    REGISTER(pending)
};

struct MSIXCapability {
    REGISTER(header, BITS(id,0,8) BITS(nextIndex,10,6) BITS(tableSize,16,11) BIT(maskAll,30) BIT(enable,31))
    REGISTER(table, BITS(BARIndex,0,3) BITS(offsetInBAR,3,27))
    REGISTER(PendingBitArray, BITS(BARIndex,0,3) BITS(offsetInBAR,3,27))
};

struct MSIXTableEntry {
    REGISTER(addressLow)
    REGISTER(addressHigh)
    REGISTER(data)
    REGISTER(vectorControl, BIT(mask,0))
};

struct ConfigSpace {
    /* number of 32-bit registers ingoring logical layout */
    static constexpr size_t NUM_REGISERS = 0x1000/4;

    REGISTERSI(NUM_REGISERS,
               BITSI(vendorID,0,0,16) BITSI(deviceID,0,16,16)
               /* command */
               BITI(IOSpaceEnable,1,0) BITI(memorySpaceEnable,1,10) BITI(busMasterEnable,1,2) BITI(interruptDisable,1,0) BITI(hasCapabilitiesList,1,20)
               BITSI(status,1,16,16)
               /*BITSI(revisionID,2,0,8) BITSI(progIF,2,8,8) BITSI(subclass,2,16,8) BITSI(classCode,2,24,8)*/
               BITSI(classCodeProgIFRevisionID,2,0,32)

               BITSI(headerType,3,16,8)

               BITSI(capabilitiesPointer,13,2,6)
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
