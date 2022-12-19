#include <Device.hpp>

using namespace zagtos;

#ifdef __aarch64__

Interrupt Device::allocateMSIInterrupt() {
    throw std::runtime_error("allocateMSIInterrupt not implented");
}

#endif
