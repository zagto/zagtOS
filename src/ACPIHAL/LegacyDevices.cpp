#include <LegacyDevices.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/protocols/Hal.hpp>

using namespace zagtos;

void initLegacyDevices(RemotePort &environmentPort) {
    /* PS/2 Controller */
    environmentPort.sendMessage(hal::MSG_FOUND_CONTROLLER,
                                zbon::encode(hal::CONTROLLER_TYPE_PS2));
}
