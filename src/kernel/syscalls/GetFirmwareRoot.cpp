#include <syscalls/GetFirmwareRoot.hpp>
#include <system/System.hpp>
#include <syscalls/UserSpaceObject.hpp>
#include <processes/UserApi.hpp>

size_t GetFirmwareRoot(const shared_ptr<Process> &process,
                       size_t infoAddress,
                       size_t,
                       size_t,
                       size_t,
                       size_t) {
    if (process->canAccessPhysicalMemory()) {
        UserSpaceObject<userApi::ZoFirmwareInfo, USOOperation::WRITE> userInfo(infoAddress);
        const hos_v1::FirmwareInfo &kernelInfo = CurrentSystem.firmwareInfo;
        switch (kernelInfo.type) {
        case hos_v1::FirmwareType::ACPI:
            userInfo.object.type = userApi::ZAGTOS_FIRMWARE_TYPE_ACPI;
            break;
        case hos_v1::FirmwareType::DTB:
            userInfo.object.type = userApi::ZAGTOS_FIRMWARE_TYPE_DTB;
            break;
        default:
            cout << "unexpected value for firmware type field" << endl;
            Panic();
        }
        userInfo.object.rootAddress = kernelInfo.rootAddress.value();
        userInfo.object.regionLength = kernelInfo.regionLength;
        userInfo.writeOut();
        return 0;
    } else {
        throw BadUserSpace(process);
    }
}
