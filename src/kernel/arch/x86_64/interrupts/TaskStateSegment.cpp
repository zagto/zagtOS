#include <interrupts/RegisterState.hpp>
#include <interrupts/util.hpp>
#include <interrupts/TaskStateSegment.hpp>
#include <tasks/Thread.hpp>


void TaskStateSegment::update(Thread *thread)
{
    usize rsp = reinterpret_cast<usize>(&thread->registerState) + sizeof(RegisterState);
    rspLow = static_cast<u32>(rsp);
    rspHigh = static_cast<u32>(rsp >> 32);

    loadTaskStateSegment();
}
