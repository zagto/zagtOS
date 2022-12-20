#include <Serial.hpp>
#include <DeviceTree.hpp>

static hos_v1::SerialInfo info;
using namespace deviceTree;

bool callback(const deviceTree::Node &node) {
    if (!node.checkCompatible("arm,pl011")) {
        return false;
    }
    if (!node.tree().statusStringOffset()) {
        /* assume ok status */
        return true;
    }
    const optional<Property> status = node.findProperty(node.tree().statusStringOffset());
    if (!status) {
        /* assume ok status */
        return true;
    }
    return status->getString() == "okay";
}

bool findInNode(const deviceTree::Node &node) {
    const optional<Node> uartNode = node.findChildByCallback({}, callback);
    if (!uartNode) {
        return false;
    }
    const optional<Region> region = uartNode->getRegionProperty();
    if (!region) {
        return false;
    }
    info.type = hos_v1::SerialType::PRIMECELL_PL011;
    info.baseAddress = region->start;
    info.memoryLength = region->length;
    return true;
}

hos_v1::SerialInfo &InitSerial() {
    info.type = hos_v1::SerialType::NO_SERIAL;

    deviceTree::Tree tree;
    if (findInNode(tree.rootNode)) {
        return info;
    }
    const optional<Node> socNode = tree.rootNode.findChildNode("soc");
    if (socNode) {
        if (findInNode(*socNode)) {
            return info;
        }
    }
    const optional<Node> ambaNode = tree.rootNode.findChildNode("amba");
    if (ambaNode) {
        if (findInNode(*ambaNode)) {
            return info;
        }
    }
    return info;
}
