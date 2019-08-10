#include <common/common.hpp>
#include <system/System.hpp>


void CommonSystem::addBootProcessor() {
    cout << "Initializing first processor..." << endl;
    CurrentProcessor = new Processor(true);
    CurrentProcessor->activePagingContext = &CurrentSystem.kernelOnlyPagingContext;
    cout << "Processor object created. registering..." << endl;
    processors.push_back(CurrentProcessor);
    cout << "Processor initialized." << endl;
}
