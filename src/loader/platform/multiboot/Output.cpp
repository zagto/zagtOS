#include <Serial.hpp>

static bool serialInitialized{false};

void Output(char character) {
    if (!serialInitialized) {
        InitSerial();
        serialInitialized = true;
    }
    WriteSerial(character);
}
