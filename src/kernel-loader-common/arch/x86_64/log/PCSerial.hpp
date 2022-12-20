#pragma once

class PCSerial {
private:
    bool isTransmitEmpty();

public:
    void init();
    void writeCharacter(char character);
};
