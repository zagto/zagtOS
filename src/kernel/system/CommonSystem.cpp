#include <common/common.hpp>
#include <system/System.hpp>


Status CommonSystem::addBootProcessor() {
    cout << "Initializing first processor..." << endl;
    Result<Processor *> processor = make_raw<Processor>(true);
    if (!processor) {
        return processor.status();
    }
    CurrentProcessor = *processor;
    CurrentProcessor->activePagingContext = &CurrentSystem.kernelOnlyPagingContext;
    cout << "Processor object created at " << CurrentProcessor << ". registering..." << endl;
    cout << "Processor initialized." << endl;
    return processors.push_back(*processor);
}
