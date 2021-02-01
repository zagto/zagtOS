#include <interrupts/RegisterState.hpp>
#include <interrupts/util.hpp>
#include <interrupts/TaskStateSegment.hpp>
#include <processes/Thread.hpp>


TaskStateSegment::TaskStateSegment(uint64_t *NMIStack, uint64_t *MCEStack) {
    IST1 = NMIStack;
    IST2 = MCEStack;
}


void TaskStateSegment::update(Thread *thread) {
    rsp = reinterpret_cast<size_t>(&thread->registerState) + sizeof(RegisterState);
    loadTaskStateSegment();
}
