#pragma once

#include <common/inttypes.hpp>
#include <optional>

class Logger;

namespace cpuid {
    enum class Vendor {
        INTEL, AMD
    };
    enum class Hypervisor {
        NONE, VMWARE, KVM, VIRTUALBOX, QEMU_TCG, UNKNOWN
    };

    struct CrystalInfo {
        uint32_t crystalTSCRatio1;
        uint32_t crystalTSCRatio2;
        uint32_t crystal;
    };

    struct Model {
        Vendor vendor;
        Hypervisor hypervisor;
        uint32_t familyID;
        uint32_t modelID;
        uint32_t steppingID;

        bool invariantTSC;

        optional<CrystalInfo> frequencyInfo;
        uint32_t hypervisorTSCKHz;

        bool isIntelNehalem() const;
        bool isIntelSandyBridge() const;
        bool isIntelIvyBridge() const;
        bool isIntelHaswell() const;
        bool isIntelBroadwell() const;
        bool isIntelSkylakeClient() const;
        bool isNewerIntelClient() const;
        bool isIntelNewerClient() const;
        bool isIntelSilvermont() const;
        bool isIntelAirmont() const;
        bool isIntelGoldmont() const;
    };

    Model getModel();
    Logger& operator<<(Logger &logger, Vendor vendor);
}
