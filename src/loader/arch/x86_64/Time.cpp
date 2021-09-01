#include <Time.hpp>
#include <CPUID.hpp>
#include <common/ModelSpecificRegister.hpp>

uint64_t timerFrequency = 0;

void detectTimerFrequency() {
    auto model = cpuid::getModel();

    if (!model.invariantTSC) {
        cout << "CPU without an invariant TSC. This is not supported." << endl;
        Panic();
    }

    const char *methodName = nullptr;

    if (model.hypervisor == cpuid::Hypervisor::NONE) {
        if (model.vendor == cpuid::Vendor::INTEL) {
            /* Based on: Indel SDM Vol. 3B: System Programming Guide, Part 2
             * 18.18.3 Determining the Processor Base Frequency */
            if (model.frequencyInfo) {
                cpuid::CrystalInfo freq = *model.frequencyInfo;
                timerFrequency = (static_cast<uint64_t>(freq.crystal) * freq.crystalTSCRatio1)
                        / freq.crystalTSCRatio2;

                methodName = "CPUID 15h";

            } else if (model.isIntelNehalem()
                       || model.isIntelSandyBridge()
                       || model.isIntelIvyBridge()
                       || model.isIntelHaswell()
                       || model.isIntelBroadwell()) {

                uint64_t busFrequency = (readModelSpecificRegister(MSR::PLATFORM_INFO) >> 8) & 0xff;
                if (model.isIntelNehalem()) {
                    timerFrequency = busFrequency * 133'330'000;
                    methodName = "PLATFORM_INFO MSR (Nehalem)";
                } else {
                    timerFrequency = busFrequency * 100'000'000;
                    methodName = "PLATFORM_INFO MSR (Sandy Bridge or greater)";
                }
            } else if (model.isIntelSilvermont() || model.isIntelAirmont()) {
                uint64_t busRatio = (readModelSpecificRegister(MSR::PLATFORM_ID) >> 8) & 0x3f;
                uint64_t busFreqency;
                if (model.isIntelSilvermont()) {
                    busFreqency = readModelSpecificRegister(MSR::FSB_FREQ) & 0x7;
                    methodName = "PLARFORM_ID/FSB_FREQ MSRs (Silvermont)";
                } else {
                    busFreqency = readModelSpecificRegister(MSR::FSB_FREQ) & 0xf;
                    methodName = "PLARFORM_ID/FSB_FREQ MSRs (Airmont)";
                }
                timerFrequency = busFreqency * busRatio;
            } else {
                cout << "Unsupported CPU Model: Family " << model.familyID << " model "
                     << model.modelID << endl;
                Panic();
            }
        } else {
            cout << "TODO: implement TSC frequency calculation for " << model.vendor << " CPUs." << endl;
            Panic();
        }
    } else {
        /* running in a hypervisor */
        if (model.hypervisorTSCKHz == 0) {
            cout << "Hypervisor detected, but no Hypervisor TSC frequency in CPUID. This is not "
                 << "supported." << endl;
            Panic();
        } else {
            timerFrequency = static_cast<uint64_t>(model.hypervisorTSCKHz) * 1000;
            methodName = "Hypervisor";
        }
    }



    cout << "Detected TSC timer frequency using << " << methodName << ": " << (timerFrequency/(1000000)) << "MHz" << endl;
    assert(timerFrequency != 0);
}

void delayMilliseconds(uint64_t ms) {
    assert(timerFrequency != 0);

    uint64_t startValue = readTimerValue();
    uint64_t endValue = startValue + (timerFrequency * ms) / 1000;
    uint64_t now = startValue;
    while (now < endValue) {
        now = readTimerValue();
    }
}
