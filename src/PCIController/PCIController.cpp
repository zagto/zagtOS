#include <iostream>
#include <tuple>
#include <zagtos/Messaging.hpp>
#include <zagtos/Controller.hpp>
#include <zagtos/PCI.hpp>

using namespace zagtos;
using namespace zagtos::pci;


int main() {
    auto msg = decodeRunMessage<std::tuple<RemotePort, zbon::EncodedData>>(MSG_START_CONTROLLER);
    std::vector<SegmentGroup> segmentGroups;
    if (!zbon::decode(std::get<1>(msg), segmentGroups)) {
        throw new std::logic_error("Got malformed segment group info.");
    }
    RemotePort envPort = std::move(std::get<0>(msg));

    std::cout << "Starting PCI Controller..." << std::endl;

}
