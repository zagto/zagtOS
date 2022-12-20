#include <DeviceTree.hpp>
#include <common/utils.hpp>
#include <iostream>

extern "C" size_t DeviceTreeAddress;

namespace deviceTree {
struct TreeHeader {
    BigEndian<uint32_t> magic;
    BigEndian<uint32_t> totalSize;
    BigEndian<uint32_t> structureOffset;
    BigEndian<uint32_t> stringsOffset;
    BigEndian<uint32_t> memoryReservationBlockOffset;
    BigEndian<uint32_t> version;
    BigEndian<uint32_t> lastCompatibleVersion;
    BigEndian<uint32_t> bootProcessorID;
    BigEndian<uint32_t> stringsLength;
    BigEndian<uint32_t> stringsStructure;
};

struct PropertyHeader {
    BigEndian<uint32_t> innerLength;
    BigEndian<uint32_t> nameOffset;
};

const Token BEGIN_NODE = 1;
const Token END_NODE = 2;
const Token PROPERTY = 3;
const Token NOP = 4;
const Token END = 9;

String::String(const char *data):
    data{data} {

    size_t index = 0;
    for (index = 0; data[index] != 0; index++) {}
    _length = index;
}

bool String::operator==(const String &other) const {
    if (_length != other._length) {
        return false;
    }
    for (size_t index = 0; index < _length; index++) {
        if (data[index] != other.data[index]) {
            return false;
        }
    }
    return true;
}

Logger &operator<<(Logger &logger, String string) {
    for (size_t index = 0; index < string._length; index++) {
        logger << string.data[index];
    }
    return logger;
}

Node::Node(const deviceTree::Tree &tree,
           const Token *beginToken,
           size_t numAddressCells,
           size_t numSizeCells):
        _tree{tree},
        beginToken{beginToken},
        numAddressCells{numAddressCells},
        numSizeCells{numSizeCells} {
    assert(*beginToken == BEGIN_NODE);
    const char *nameData = reinterpret_cast<const char *>(beginToken + 1);
    optional<size_t> unitAddressSeparator;
    size_t index;
    for (index = 0; nameData[index] != 0; index++) {
        if (nameData[index] == '@' && !unitAddressSeparator) {
            unitAddressSeparator = index;
        }
    }
    if (unitAddressSeparator) {
        name = String(nameData, *unitAddressSeparator);
        unitAddress = String(nameData + *unitAddressSeparator + 1,
                             index - *unitAddressSeparator - 1);
    } else {
        name = String(nameData, index);
    }
    size_t numTokenSizes = index / 4 + 2;
    firstInnerToken = beginToken + numTokenSizes;
}

const Token *Node::parse(ParseAction action,
                         optional<String> searchName,
                         optional<uint32_t> searchNameOffset,
                         const Token *startPoint) const {
    /* startPoint parameter is optional, if absent start at the first token */
    const Token *currentToken = startPoint != nullptr ? startPoint : firstInnerToken;
    size_t nestingLevel = 1;
    while (nestingLevel > 0) {
        if (*currentToken == BEGIN_NODE) {
            Node innerNode(_tree, currentToken);
            if (action == ParseAction::FIND_CHILD && nestingLevel == 1) {
                /* return any nodes if there is no searchName */
                if (!searchName || *searchName == innerNode.name) {
                    return currentToken;
                }
            }
            currentToken = innerNode.firstInnerToken;
            nestingLevel++;
        } else if (*currentToken == END_NODE) {
            currentToken++;
            nestingLevel--;
        } else if (*currentToken == PROPERTY) {
            Property property(_tree, currentToken);
            if (action == ParseAction::FIND_PROPERTY) {
                if (nestingLevel == 1 && *searchNameOffset == property.nameOffset()) {
                    return currentToken;
                }
            }
            currentToken = property.followingToken();
        } else if (*currentToken == NOP) {
            currentToken++;
        } else if (*currentToken == END) {
            cout << "DeviceTree: malformed tree: unexpected END token" << endl;
            Panic();
        } else {
            cout << "DeviceTree: unknown token type " << static_cast<uint32_t>(*currentToken)
                 << endl;
            Panic();
        }
    }
    if (action == ParseAction::FIND_FOLLOWING) {
        return currentToken;
    } else {
        return nullptr;
    }
}

const Token *Node::findFollowingToken() const {
    return parse(ParseAction::FIND_FOLLOWING);
}

optional<Node> Node::findNodeByPHandle(uint32_t phandle) const {
    optional<Property> phandleProperty = findProperty(_tree.phandleStringOffset());
    if (phandleProperty && phandleProperty->getInt<uint32_t>() == phandle) {
        return *this;
    }
    optional<Node> childNode = findChildNode(optional<String>() );
    while (childNode) {
        optional<Node> potentialResult = childNode->findNodeByPHandle(phandle);
        if (potentialResult) {
            return potentialResult;
        }
        childNode = findChildNode(optional<String>(), childNode->findFollowingToken());
    }
    return {};
}

optional<Node> Node::findChildWithProperty(optional<String> nodeName, String propertyName) const {
    optional<uint32_t> nameOffset = _tree.getStringOffset(propertyName);
    if (!nameOffset) {
        return {};
    }
    optional<Node> childNode = findChildNode(nodeName);
    while (childNode) {
        if (childNode->findProperty(*nameOffset)) {
            return childNode;
        }
        childNode = findChildNode(nodeName, childNode->findFollowingToken());
    }
    return {};
}

optional<Node> Node::findChildByCallback(optional<String> nodeName,
                                         NodeMatchCallback callback) const {
    optional<Node> childNode = findChildNode(nodeName);
    while (childNode) {
        if (callback(*childNode)) {
            return childNode;
        }
        childNode = findChildNode(nodeName, childNode->findFollowingToken());
    }
    return {};
}

optional<Node> Node::findChildNode(optional<String> name, const Token *startPoint) const {
    const Token *token = parse(ParseAction::FIND_CHILD, name, {}, startPoint);
    if (token == nullptr) {
        return {};
    }
    uint32_t childAddressCells = DEFAULT_NUM_ADDRESS_CELLS;
    uint32_t childSizeCells = DEFAULT_NUM_SIZE_CELLS;
    optional<Property> addressCellsPropery = findProperty(_tree.addressCellsStringOffset());
    if (addressCellsPropery) {
        childAddressCells = addressCellsPropery->getInt<uint32_t>();
    }
    optional<Property> sizeCellsPropery = findProperty(_tree.sizeCellsStringOffset());
    if (sizeCellsPropery) {
        childSizeCells = sizeCellsPropery->getInt<uint32_t>();
    }
    return Node(_tree, token, childAddressCells, childSizeCells);
}

optional<Property> Node::findProperty(String name) const {
    optional<uint32_t> offset = _tree.getStringOffset(name);
    if (offset) {
        return findProperty(*offset);
    } else {
        return {};
    }
}

optional<Property> Node::findProperty(uint32_t nameOffset) const {
    const Token *token = parse(ParseAction::FIND_PROPERTY, {}, nameOffset);
    if (token == nullptr) {
        return {};
    }
    return Property(_tree, token);
}

size_t Node::getNumRegions() const {
    const optional<Property> property = findProperty(_tree.regStringOffset());
    if (!property) {
        return 0;
    }
    const size_t cellsPerRegion = numAddressCells + numSizeCells;
    const size_t bytesPerCell = 4;
    assert(property->length() % (cellsPerRegion * bytesPerCell) == 0);
    return property->length() / (cellsPerRegion * bytesPerCell);
}

Region Node::getRegionProperty(size_t regionIndex) const {
    const optional<Property> property = findProperty(_tree.regStringOffset());
    assert(property);
    assert(numAddressCells <= PLATFORM_BITS / 32);
    assert(numSizeCells <= PLATFORM_BITS / 32);
    const size_t cellsPerRegion = numAddressCells + numSizeCells;
    const size_t totalCells = cellsPerRegion * getNumRegions();
    Region result{0, 0};
    for (size_t index = 0; index < numAddressCells; index++) {
        result.start = (result.start << 32) | property->getInt<uint32_t>(
                    regionIndex * cellsPerRegion + index, totalCells);
    }
    for (size_t index = numAddressCells; index < cellsPerRegion; index++) {
        result.length = (result.length << 32) | property->getInt<uint32_t>(
                    regionIndex * cellsPerRegion + index, totalCells);
    }
    return result;
}

bool Node::checkCompatible(String string) const {
    const optional<Property> property = findProperty(_tree.compatibleStringOffset());
    if (!property) {
        return false;
    }
    return property->findString(string);
}

Property::Property(const Tree &tree, const Token *token) :
        tree{tree} {
    assert(*token == PROPERTY);
    const PropertyHeader *header = reinterpret_cast<const PropertyHeader *>(token + 1);
    _nameOffset = header->nameOffset;
    _length = header->innerLength;
    _data = reinterpret_cast<const uint8_t *>(header) + sizeof(PropertyHeader);
}

const String Property::name() const {
    return tree.getString(_nameOffset);
}


const Token *Property::followingToken() const {
    size_t endAddress = reinterpret_cast<size_t>(_data) + _length;
    endAddress = align(endAddress, 4, AlignDirection::UP);
    return reinterpret_cast<const Token *>(endAddress);
}

template<typename T>
T Property::getInt(size_t index, size_t totalNumElements) const {
    const size_t expected_length = sizeof(T) * totalNumElements;
    assert(_length >= expected_length);
    if (_length > expected_length) {
        cout << "warning: DeviceTree property " << name() << " larger than expected: " << _length
             << "bytes, while expecting only " << expected_length << "bytes" << endl;
    }
    BigEndian<T> result;
    memcpy(&result, _data + index * sizeof(T), sizeof(T));
    return result;
}

template uint32_t Property::getInt<uint32_t>(size_t index, size_t totalNumElements) const;
template uint64_t Property::getInt<uint64_t>(size_t index, size_t totalNumElements) const;

uint64_t Property::getIntAutoSize() const {
    if (_length == 4) {
        return getInt<uint32_t>();
    } else {
        assert(_length == 8);
        return getInt<uint64_t>();
    }
}


String Property::getString() const {
    return String(reinterpret_cast<const char *>(_data), _length);
}

bool Property::findString(String string) const {
    size_t properyPosition = 0;
    size_t stringPosition = 0;

    while (properyPosition < _length) {
        if (stringPosition == string.length()) {
            if (_data[properyPosition] == '\0') {
                /* reached end of both strings without finding a difference */
                return true;
            }

            while (_data[properyPosition] != '\0') {
                properyPosition++;
            }
            properyPosition++;
            stringPosition = 0;
        } else {
            /* stringPosition not at end */
            if (_data[properyPosition] == '\0') {
                /* properyPosition at end of one string */
                stringPosition = 0;
                properyPosition++;
            } else {
                /* neither positions at end of string */
                if (_data[properyPosition] == string.data[stringPosition]) {
                    stringPosition++;
                    properyPosition++;
                } else {
                    /* found mismatch */
                    stringPosition = 0;
                    while (_data[properyPosition] != '\0') {
                        properyPosition++;
                    }
                    properyPosition++;
                }
            }
        }
    }
    return false;
}


Tree::Tree() :
    header{reinterpret_cast<TreeHeader *>(DeviceTreeAddress)},
    rootNode(*this, reinterpret_cast<const Token *>(DeviceTreeAddress + header->structureOffset)) {

    assert(header->magic == 0xd00dfeed);
    stringBuffer = reinterpret_cast<char *>(DeviceTreeAddress + header->stringsOffset);
    stringBufferSize = header->stringsLength;
    _phandleStringOffset = *getStringOffset("phandle");
    _addressCellsStringOffset = *getStringOffset("#address-cells");
    _sizeCellsStringOffset = *getStringOffset("#size-cells");
    _regStringOffset = *getStringOffset("reg");
    _compatibleStringOffset = *getStringOffset("compatible");
    _statusStringOffset = getStringOffset("status");
}

String Tree::getString(uint32_t offset) const {
    assert(offset < header->stringsLength);
    return String(stringBuffer + offset);
}

optional<uint32_t> Tree::getStringOffset(String string) const {
    uint32_t startOffset = 0;
    while (startOffset < stringBufferSize) {
        String currentString = String(stringBuffer + startOffset);
        if (currentString == string) {
            return startOffset;
        }
        startOffset += currentString.length() + 1;
    }
    cout << "could not find offset for: " << string << endl;
    return {};
}

Region Tree::memoryRegion() const {
    return Region(reinterpret_cast<size_t>(header), header->totalSize);
}

struct ReservationBlockEntry {
    BigEndian<uint64_t> start;
    BigEndian<uint64_t> length;
};

Region Tree::reservationBlockEntry(size_t index) const {
    const size_t headerAddress = reinterpret_cast<size_t>(header);
    assert(headerAddress % sizeof(size_t) == 0);
    const ReservationBlockEntry *entries = reinterpret_cast<const ReservationBlockEntry *>(
                headerAddress + header->memoryReservationBlockOffset);
    return Region(entries[index].start, entries[index].length);
}


}
