#include <CPUID.hpp>
#include <cpuid.h>
#include <common/common.hpp>

namespace cpuid {

static const char *VENDOR_STRING_INTEL = "GenuineIntel";
static const char *VENDOR_STRING_AMD = "AuthenticAMD";
static const char *HYPERVISOR_STRING_VMWARE = "VMwareVMware";
static const char *HYPERVISOR_STRING_KVM = "KVMKVMKVM\0\0";
static const char *HYPERVISOR_STRING_VIRTUALBOX = "VBoxVBoxVBox";
static const size_t VENDOR_STRING_LENGTH = 12;

Model getModel() {
    size_t maxFunction;
    uint32_t unused;
    uint32_t vendorString[4];
    uint32_t hypervisorString[4];
    uint32_t combinedModel;
    __cpuid(0, maxFunction, vendorString[0], vendorString[2], vendorString[1]);
    __cpuid(1, combinedModel, unused, unused, unused);
    __cpuid(0x40000000, unused, hypervisorString[0], hypervisorString[1], hypervisorString[2]);
    vendorString[3] = 0;
    hypervisorString[3] = 0;

    Model result;
    if (memcmp(vendorString, VENDOR_STRING_INTEL, VENDOR_STRING_LENGTH) == 0) {
        result.vendor = Vendor::INTEL;
    } else if (memcmp(vendorString, VENDOR_STRING_AMD, VENDOR_STRING_LENGTH) == 0) {
        result.vendor = Vendor::AMD;
    } else {
        cout << "Unsupported CPU Vendor String: " << reinterpret_cast<char *>(vendorString) << endl;
        Panic();
    }

    if (memcmp(hypervisorString, HYPERVISOR_STRING_KVM, VENDOR_STRING_LENGTH) == 0) {
        result.hypervisor = Hypervisor::KVM;
    } else if (memcmp(hypervisorString, HYPERVISOR_STRING_VMWARE, VENDOR_STRING_LENGTH) == 0) {
        result.hypervisor = Hypervisor::VMWARE;
    } else if (memcmp(hypervisorString, HYPERVISOR_STRING_VIRTUALBOX, VENDOR_STRING_LENGTH) == 0) {
        result.hypervisor = Hypervisor::VIRTUALBOX;
    } else {
        result.hypervisor = Hypervisor::NONE;
    }

    result.steppingID = combinedModel & 0xf;
    result.modelID = (combinedModel >> 4) & 0xf;
    result.familyID = (combinedModel >> 8) & 0xf;
    if (result.familyID == 0xf || result.familyID == 0x6) {
        result.modelID |= ((combinedModel >> 16) & 0xf) << 4;
    }
    if (result.familyID == 0xf) {
        result.familyID += (combinedModel >> 20) & 0xff;
    }

    cout << "CPUID max function = " << maxFunction << endl;

    uint32_t advancedPowerManagement;
    __cpuid(0x80000007, unused, unused, unused, advancedPowerManagement);
    result.invariantTSC = advancedPowerManagement & (1u << 8);

    if (result.vendor == Vendor::INTEL
            && result.hypervisor == Hypervisor::NONE
            && maxFunction >= 0x15) {
        CrystalInfo info;
        __cpuid(0x15, info.crystalTSCRatio2, info.crystalTSCRatio1, info.crystal, unused);

        /* ratio is enumerated - the frequencyInfo is usable and will be returned */
        if (info.crystalTSCRatio1 != 0 && info.crystalTSCRatio2 != 0) {
            /* crystal frequency is not enumerated -> intel manual says to use table */
            if (info.crystal == 0) {
                if (result.isIntelSkylakeClient() || result.isNewerIntelClient()) {
                    info.crystal = 24'000'000;
                } else if (result.isIntelGoldmont()) {
                    info.crystal = 19'200'000;
                } else {
                    cout << "CPUID does define crystal ratio but not crystal frequency and CPU is "
                         << " not in the table from Intel Manual" << endl;
                    Panic();
                }
            }
            result.frequencyInfo = info;
        }
    }

    result.hypervisorTSCKHz = 0;
    if (result.hypervisor != Hypervisor::NONE) {
        __cpuid(0x40000010, result.hypervisorTSCKHz, unused, unused, unused);
    }

    return result;
}

Logger& operator<<(Logger &logger, Vendor vendor) {
    switch (vendor) {
    case Vendor::INTEL:
        return logger << "Intel";
    case Vendor::AMD:
        return logger << "AMD";
    default:
        cout << "Tried to output unsupported CPU vendor." << endl;
        Panic();
    }
}

/* The following methods are based on:
 * https://en.wikichip.org/wiki/intel/cpuid */
bool Model::isIntelNehalem() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        /* Nehalem */
        case 26:
        case 30:
        case 31:
        case 46:
        /* Westmere */
        case 37:
        case 44:
        case 47:
            return true;
        }
    }
    return false;
}

bool Model::isIntelSandyBridge() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 42:
        case 45:
            return true;
        }
    }
    return false;
}

bool Model::isIntelIvyBridge() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 58:
        case 62:
            return true;
        }
    }
    return false;
}

bool Model::isIntelHaswell() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 60:
        case 63:
        case 69:
        case 70:
            return true;
        }
    }
    return false;
}

bool Model::isIntelBroadwell() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 61:
        case 71:
        case 79:
        case 86:
            return true;
        }
    }
    return false;
}


bool Model::isIntelSkylakeClient() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 78:
        case 94:
            return true;
        }
    }
    return false;
}

/* CPUs not explicitly named in Intel Manual, but we will handle them like Kaby Lake if no
 * core Frequency information is present in CPUID. */
bool Model::isNewerIntelClient() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        /* Kaby/Whiskey/Amber/Comet/Coffee Lake */
        case 142:
        case 158:
        /* Cannon Lake */
        case 102:
        /* Comet Lake */
        case 165:
        /* Ice Lake */
        case 125:
        case 126:
        /* Some IceLake CPUs already have full data in CPUID, so they won't need to be handled
         * by this:
         *  https://github.com/InstLatx64/InstLatx64/blob/master/GenuineIntel/GenuineIntel00706E5_IceLakeY_CPUID.txt */
            return true;
        }
    }
    return false;
}

bool Model::isIntelSilvermont() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 55:
        case 74:
        case 77:
        case 90:
        case 93:
            return true;
        }
    }
    return false;
}

bool Model::isIntelAirmont() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 76:
            return true;
        }
    }
    return false;
}

bool Model::isIntelGoldmont() const {
    if (vendor == Vendor::INTEL && familyID == 6) {
        switch (modelID) {
        case 92:
        case 95:
            return true;
        }
    }
    return false;
}
}
