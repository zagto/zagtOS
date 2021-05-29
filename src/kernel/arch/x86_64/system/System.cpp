#include <paging/PagingContext.hpp>
#include <system/System.hpp>


hos_v1::System *_HandOverSystem;

System::System() :
        CommonSystem(*_HandOverSystem, handOverStatus),
        ACPIRoot{_HandOverSystem->firmwareRoot} {
    if (!handOverStatus) {
        cout << "Exception during System initialization" << endl;
        Panic();
    }
    /* TODO: support for Intel PCIDs could be added here */
    tlbContextsPerProcessor = 1;
}
