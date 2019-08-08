#include <system/System.hpp>
#include <time/Time.hpp>

void Time::initialize() {
    detectTimerFrequency();
}


void Time::detectTimerFrequency() {
    /* on x86_64, waking secondary processors requires timing, so it should always happen after
     * this */
    assert(CurrentSystem.processors.size() == 1);

    APICTimer &apicTimer = CurrentProcessor->interrupts.localAPIC.timer;
    apicTimer.startCounting(0xffffffff);

    /* let the legacy timer wrap after 0xffff ticks */
    legacyTimer.setReloadValue(0xffff);
    legacyTimer.setOperatingMode();

    /* first wait for the value to drop below half */
    while (legacyTimer.readValue() >= 0x8000) {}
    /* now if it's bigger again the timer wrapped around */
    while (legacyTimer.readValue() < 0x8000) {}

    uint64_t dif = 0xffffffff - apicTimer.readValue();
    const uint64_t legacyFrequency = 1193182;

    timerFrequency = (legacyFrequency * dif) / 0xffffull;


    /* TODO: this does not seem very accurate, check on real hradware
    cout << "APIC timer frequency: " << timerFrequency << "Hz" << endl;


    uint64_t counter = 0;
    while (true) {
        cout << counter << endl;

        apicTimer.startCounting(timerFrequency);
        while (apicTimer.readValue() > 0) {}

        counter++;
    }
    */
}
