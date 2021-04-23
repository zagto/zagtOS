#include <interrupts/RegisterState.hpp>
#include <interrupts/util.hpp>
#include <interrupts/TaskStateSegment.hpp>
#include <processes/Thread.hpp>


TaskStateSegment::TaskStateSegment(Status &status) {
    if (!status) {
        return;
    }

    Result NMIStack = make_raw_array<uint64_t>(IST_STACK_QWORDS);
    if (!NMIStack) {
        status = NMIStack.status();
        return;
    }
    IST1 = *NMIStack + IST_STACK_QWORDS;

    Result MCEStack = make_raw_array<uint64_t>(IST_STACK_QWORDS);
    if (!MCEStack) {
        status = MCEStack.status();
        return;
    }
    IST2 = *MCEStack + IST_STACK_QWORDS;
}


void TaskStateSegment::update(Thread *thread) {
    rsp = reinterpret_cast<size_t>(&thread->registerState) + sizeof(RegisterState);
    loadTaskStateSegment();
}

TaskStateSegment::~TaskStateSegment() {
    if (IST1 != nullptr) {
        delete[] (IST1 - IST_STACK_QWORDS);
    }
    if (IST2 != nullptr) {
        delete[] (IST2 - IST_STACK_QWORDS);
    }
}
