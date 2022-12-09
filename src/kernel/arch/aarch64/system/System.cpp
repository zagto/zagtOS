#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#include <processes/Process.hpp>

hos_v1::System *_HandOverSystem;

System::System() :
        CommonSystem(*_HandOverSystem){
    /* TODO: support for Intel PCIDs could be added here */
    tlbContextsPerProcessor = 1;

}

void System::setupCurrentProcessor() noexcept {
}

void System::lateInitialization() {
}

void System::bindInterrupt(BoundInterrupt &/*boundInterrupt*/) {
}

void System::unbindInterrupt(BoundInterrupt &/*boundInterrupt*/) {
}

void System::interruptFullyProcessed(BoundInterrupt &/*boundInterrupt*/) {
}
