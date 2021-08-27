#include <interrupts/RegisterState.hpp>
#include <interrupts/util.hpp>
#include <interrupts/TaskStateSegment.hpp>
#include <processes/Thread.hpp>
#include <system/Processor.hpp>
#include <memory/DLMallocGlue.hpp>

TaskStateSegment::TaskStateSegment(Status &status) {
    if (!status) {
        return;
    }

    Result NMIStack = DLMallocGlue.allocate(IST_STACK_QWORDS * 8, 16);
    if (!NMIStack) {
        status = NMIStack.status();
        return;
    }
    IST1 = NMIStack->asPointer<uint64_t>() + IST_STACK_QWORDS;

    Result MCEStack = DLMallocGlue.allocate(IST_STACK_QWORDS * 8, 16);
    if (!MCEStack) {
        status = MCEStack.status();
        return;
    }
    IST2 = MCEStack->asPointer<uint64_t>() + IST_STACK_QWORDS;
}


void TaskStateSegment::update(Thread *thread) {
    rsp = reinterpret_cast<size_t>(thread->kernelStack->userRegisterState())
            + sizeof(RegisterState);
    loadTaskStateSegment(thread->currentProcessor()->id);
}

TaskStateSegment::~TaskStateSegment() {
    if (IST1 != nullptr) {
        delete (IST1 - IST_STACK_QWORDS);
    }
    if (IST2 != nullptr) {
        delete (IST2 - IST_STACK_QWORDS);
    }
}
