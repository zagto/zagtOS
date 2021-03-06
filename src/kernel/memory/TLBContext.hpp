#pragma once

#include <common/common.hpp>
#include <vector>
#include <memory/PageOutContext.hpp>

class PagingContext;
class Frame;

class TLBContext {
private:
    friend class CommonProcessor;
    friend class InvalidateQueue;
    /* Use 16-bit IDs internally for compact memory use. We create a vector of all possible TLB
     * contexts, and multi-core x86_64 Processors can have quite a lot */
    PagingContext *activePagingContext = nullptr;
    uint16_t nextLocalID;
    uint16_t previousLocalID;

    Processor &processor() const noexcept;
    static TLBContextID localIDToGlobal(uint16_t localID, uint16_t processorID) noexcept;
    TLBContextID id() const noexcept;
    void localInvalidate(UserVirtualAddress address) noexcept;

public:
    const uint16_t processorID;
    const uint16_t localID;

    TLBContext();

    void activate() noexcept;
    void activatePagingContext(PagingContext *pagingContext) noexcept;
    /* removes pagingContext if TLBContext still holds this one */
    void remove(PagingContext *pagingContext) noexcept;
    /* The TLBContext's Processor can replace the TLB context at any time, so the result may
     * actually be incorrect at the time it is returned. However when unmapping a Frame, a result
     * of false means the TLBContext did not hold the PagingContext at a time, meaning it is safe
     * to assume this Frame's mapping address does not need to be invalidated */
    bool potentiallyHolds(PagingContext *pagingContext) noexcept;
    PageOutContext requestInvalidate(Frame *frame,
                                     ProcessAddressSpace *addressSpace,
                                     UserVirtualAddress address) noexcept;

};

static constexpr TLBContextID TLB_CONTEXT_ID_NONE = static_cast<uint32_t>(-1);
extern TLBContext *TLBContexts;
