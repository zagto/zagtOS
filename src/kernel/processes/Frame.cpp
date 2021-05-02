#include <common/common.hpp>
#include <memory/FrameManagement.hpp>

class Frame {
private:
    PhysicalAddress address = PhysicalAddress::NULL;
    size_t copyOnWriteCount = 1;
    size_t pagedInCount = 0;
    size_t potentiallyLeftPagedInCount = 0;
    bool isForPhysicalAccess = false;

public:
    Frame(frameManagement::ZoneID zoneID, Status &status);
    Frame(PhysicalAddress address, Status &);
    Frame *ensureSinleInstance();
    Frame *copyOnWriteDuplicate();
    ~Frame();
};
