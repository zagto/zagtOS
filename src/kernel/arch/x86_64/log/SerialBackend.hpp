#pragma once

class SerialBackend {
private:
    bool signalReceived();
    bool isTransmitEmpty();

public:
    void init();
    void write(char character);
    char read();
    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();
};
