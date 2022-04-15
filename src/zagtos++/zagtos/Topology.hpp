#pragma once

#include <cstddef>

namespace std {
class thread;
}

namespace zagtos {

class Processor {
private:
    size_t id = 0;

public:
    void pinCurrentThread();
};

}
