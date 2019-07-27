#include <common/common.hpp>
#include <system/System.hpp>


void System::addBootProcessor() {
    cout << "Initializing first processor..." << endl;
    CurrentProcessor = new Processor;
    CurrentProcessor->activeMasterPageTable = CurrentSystem.kernelOnlyMasterPageTable;
    cout << "Processor object created. registering..." << endl;
    processors.push_back(CurrentProcessor);
    cout << "Processor initialized." << endl;
}
