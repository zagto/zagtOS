#pragma once

class SerialBackend {
private:
    bool isTransmitEmpty();
    void setKernelColor();
    void setProgramNameColor();
    void setProgramColor();

public:
    void init();
    void writeCharacter(char character);
    void write(char character);
};
