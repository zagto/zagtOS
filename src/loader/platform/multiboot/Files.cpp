#include <Files.hpp>
#include <Multiboot.hpp>

void *LoadKernelImage() {
    ModuleTag *mod = MultibootInfo->getTag<ModuleTag>(0);
    if (!mod) {
        cout << "Kernel and SystemEnvironment need to be loaded as Multiboot2 Modules!" << endl;
        Panic();
    }
    return reinterpret_cast<void *>(mod->startAddress);
}

void *LoadProcessImage() {
    ModuleTag *mod = MultibootInfo->getTag<ModuleTag>(1);
    if (!mod) {
        cout << "Kernel and SystemEnvironment need to be loaded as Multiboot2 Modules!" << endl;
        Panic();
    }
    return reinterpret_cast<void *>(mod->startAddress);
}
