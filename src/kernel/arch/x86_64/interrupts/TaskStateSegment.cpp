#include <interrupts/RegisterState.hpp>
#include <interrupts/util.hpp>
#include <interrupts/TaskStateSegment.hpp>
#include <processes/Thread.hpp>


void TaskStateSegment::update(Thread *thread)
{
    size_t rsp = reinterpret_cast<size_t>(&thread->registerState) + sizeof(RegisterState);
    rspLow = static_cast<uint32_t>(rsp);
    rspHigh = static_cast<uint32_t>(rsp >> 32);

    loadTaskStateSegment();
}
