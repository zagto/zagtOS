#include <zagtos/Topology.hpp>
#include <zagtos/syscall.h>
#include <thread>

namespace zagtos {

void Processor::pinCurrentThread() {
    zagtos_syscall1(SYS_PIN_THREAD, id);
}

}
