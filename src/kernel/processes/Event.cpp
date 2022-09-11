#include <processes/Event.hpp>
#include <processes/Process.hpp>
#include <processes/UserApi.hpp>
#include <syscalls/UserSpaceObject.hpp>

void Event::writeEventInfo(shared_ptr<Process> process, size_t userAddress) {
    process->addressSpace.copyTo(userAddress,
                                 reinterpret_cast<const uint8_t *>(&eventInfo),
                                 sizeof(userApi::ZoEventInfo),
                                 true);
}
