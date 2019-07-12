#include <common/common.hpp>
#include <system/System.hpp>


void System::addBootProcessor() {
    Log << "Initializing first processor..." << EndLine;
    CurrentProcessor = new Processor;
    CurrentProcessor->activeMasterPageTable = CurrentSystem.kernelOnlyMasterPageTable;
    Log << "Processor object created. registering..." << EndLine;
    processors.pushBack(CurrentProcessor);
    Log << "Processor initialized." << EndLine;
}
