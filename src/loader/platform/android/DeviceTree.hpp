#pragma once
#include <common/inttypes.hpp>
#include <common/Endian.hpp>
#include <common/Region.hpp>
#include <optional>

class Logger;

namespace deviceTree {


using Token = BigEndian<int32_t>;
class Tree;
struct TreeHeader;

class String {
private:
    friend class Property;
    size_t _length;
    const char *data;

public:
    String():
        _length{0}, data{nullptr} {}
    String(const char *data, size_t length):
        _length{length}, data{data} {}
    String(const char *data);
    bool operator==(const String &other) const;
    bool operator!=(const String &other) const {
        return !(*this == other);
    }
    size_t length() const {
        return _length;
    }
    friend Logger &operator<<(Logger &logger, String string);
};
Logger &operator<<(Logger &logger, String string);


class Property {
private:
    const Tree& tree;
    uint32_t _nameOffset;
    size_t _length;
    const uint8_t *_data;

public:
    Property(const Tree &tree, const Token *token);
    const String name() const;
    uint32_t nameOffset() const {
        return _nameOffset;
    }
    size_t length() const {
        return _length;
    }
    const uint8_t *data() const {
        return _data;
    }
    const Token *followingToken() const;
    template<typename T>
    T getInt(size_t index = 0, size_t totalNumElements = 1) const;
    uint64_t getIntAutoSize() const;
    String getString() const;
    bool findString(String string) const;
};

class Node;
using NodeMatchCallback = bool(*)(const Node &node);

class Node {
private:
    enum class ParseAction {
        FIND_CHILD, FIND_PROPERTY, FIND_FOLLOWING
    };

    const Tree &_tree;
    const Token *beginToken;
    const Token *firstInnerToken;
    String name;
    optional<String> unitAddress;
    const size_t numAddressCells;
    const size_t numSizeCells;

    const Token *parse(ParseAction action,
                       optional<String> searchName = {},
                       optional<uint32_t> searchNameOffset = {},
                       const Token *startPoint = nullptr) const;

public:
    static constexpr uint32_t DEFAULT_NUM_ADDRESS_CELLS = 2;
    static constexpr uint32_t DEFAULT_NUM_SIZE_CELLS = 1;

    Node(const Tree &tree,
         const Token *beginToken,
         size_t numAddressCells = DEFAULT_NUM_ADDRESS_CELLS,
         size_t numSizeCells = DEFAULT_NUM_SIZE_CELLS);
    const Token *findFollowingToken() const;
    optional<Node> findNodeByPHandle(uint32_t phandle) const;
    optional<Node> findChildNode(optional<String> name, const Token *startPoint = nullptr) const;
    optional<Node> findChildWithProperty(optional<String> nodeName, String propertyName) const;
    optional<Node> findChildByCallback(optional<String> nodeName, NodeMatchCallback callback) const;
    /* versions of findChildNode and findChildWithProperty that take a pure C string as argument */
    optional<Node> findChildNode(const char *name) const {
        return findChildNode(String(name));
    }
    optional<Node> findChildNode() const {
        return findChildNode(optional<String>());
    }
    optional<Node> findChildWithProperty(const char *nodeName, String propertyName) const {
        return findChildWithProperty(String(nodeName), propertyName);
    }
    optional<Property> findProperty(uint32_t nameOffset) const;
    optional<Property> findProperty(String name) const;
    size_t getNumRegions() const;
    Region getRegionProperty(size_t regionIndex = 0) const;
    const Tree &tree() const {
        return _tree;
    }
    bool checkCompatible(String string) const;
};

class Tree {
private:
    TreeHeader *header;
    const char *stringBuffer;
    size_t stringBufferSize;
    uint32_t _phandleStringOffset;
    uint32_t _addressCellsStringOffset;
    uint32_t _sizeCellsStringOffset;
    uint32_t _regStringOffset;
    uint32_t _compatibleStringOffset;
    optional<uint32_t> _statusStringOffset;

public:
    const Node rootNode;

    Tree();
    Tree(const Tree &other) = delete;
    String getString(uint32_t offset) const;
    optional<uint32_t> getStringOffset(String string) const;
    uint32_t phandleStringOffset() const {
        return _phandleStringOffset;
    }
    uint32_t addressCellsStringOffset() const {
        return _addressCellsStringOffset;
    }
    uint32_t sizeCellsStringOffset() const {
        return _sizeCellsStringOffset;
    }
    uint32_t regStringOffset() const {
        return _regStringOffset;
    }
    uint32_t compatibleStringOffset() const {
        return _compatibleStringOffset;
    }
    uint32_t statusStringOffset() const {
        return _statusStringOffset;
    }
    Region memoryRegion() const;
    Region reservationBlockEntry(size_t index) const;
};

}
