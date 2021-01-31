#include <common/common.hpp>
#include <system/System.hpp>


void CommonSystem::addBootProcessor() {
    cout << "Initializing first processor..." << endl;
    CurrentProcessor = new Processor(true);
    CurrentProcessor->activePagingContext = &CurrentSystem.kernelOnlyPagingContext;
    cout << "Processor object created at " << CurrentProcessor << ". registering..." << endl;
    /* register variables can't be pushed */
    auto tmp = CurrentProcessor;
    processors.push_back(tmp);
    cout << "Processor initialized." << endl;
}
