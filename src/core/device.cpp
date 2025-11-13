#include <MSTL/core/device.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/unordered_map.hpp>
#include <MSTL/core/packages.hpp>
#include <MSTL/core/character.hpp>
#include <MSTL/core/format.hpp>
#include <devguid.h>
#include <Dbt.h>
MSTL_BEGIN_NAMESPACE__

void device::refresh_status() {
    if (dev_inst_ != 0) {
        CM_Get_DevNode_Status(&status_, &problem_code_, dev_inst_, 0);
    }
}

DEVICE_CLASS device::to_device_class(const string& class_str) {
    string upper = class_str;
    upper.uppercase();

    static const unordered_map<string, DEVICE_CLASS> class_map = {
            {"1394", DEVICE_CLASS::C1394},
            {"1394DEBUG", DEVICE_CLASS::C1394DEBUG},
            {"61883", DEVICE_CLASS::C61833},
            {"ADAPTER", DEVICE_CLASS::ADAPTER},
            {"APMSUPPORT", DEVICE_CLASS::APM_SUPPORT},
            {"AUDIOPROCESSINGOBJECT", DEVICE_CLASS::AUDIO_PROCESSING_OBJECT},
            {"AVC", DEVICE_CLASS::AVC},
            {"BATTERY", DEVICE_CLASS::BATTERY},
            {"BIOMETRIC", DEVICE_CLASS::BIO_METRIC},
            {"BLUETOOTH", DEVICE_CLASS::BLUETOOTH},
            {"CAMERA", DEVICE_CLASS::CAMERA},
            {"CDROM", DEVICE_CLASS::CDROM},
            {"COMPUTEACCELERATOR", DEVICE_CLASS::COMPUTE_ACCELERATOR},
            {"COMPUTER", DEVICE_CLASS::COMPUTER},
            {"DECODER", DEVICE_CLASS::DECODER},
            {"DISKDRIVE", DEVICE_CLASS::DISK_DRIVE},
            {"DISPLAY", DEVICE_CLASS::DISPLAY},
            {"DOT4", DEVICE_CLASS::DOT4},
            {"DOT4PRINT", DEVICE_CLASS::DOT4_PRINT},
            {"EHSTORAGESILO", DEVICE_CLASS::EHSTORAGESILO},
            {"ENUM1394", DEVICE_CLASS::ENUM1394},
            {"EXTENSION", DEVICE_CLASS::EXTENSION},
            {"FDC", DEVICE_CLASS::FDC},
            {"FIRMWARE", DEVICE_CLASS::FIRMWARE},
            {"FLOPPYDISK", DEVICE_CLASS::FLOPPY_DISK},
            {"GENERIC", DEVICE_CLASS::GENERIC},
            {"GPS", DEVICE_CLASS::GPS},
            {"HDC", DEVICE_CLASS::HDC},
            {"HIDCLASS", DEVICE_CLASS::HID_CLASS},
            {"HOLOGRAPHIC", DEVICE_CLASS::HOLOGRAPHIC},
            {"I3C", DEVICE_CLASS::I3C},
            {"IMAGE", DEVICE_CLASS::IMAGE},
            {"INFINIBAND", DEVICE_CLASS::INFINIBAND},
            {"KEYBOARD", DEVICE_CLASS::KEYBOARD},
            {"LEGACYDRIVER", DEVICE_CLASS::LEGACY_DRIVER},
            {"MEDIA", DEVICE_CLASS::MEDIA},
            {"MEDIUMCHANGER", DEVICE_CLASS::MEDIUM_CHANGER},
            {"MEMORY", DEVICE_CLASS::MEMORY},
            {"MODEM", DEVICE_CLASS::MODEM},
            {"MTD", DEVICE_CLASS::MTD},
            {"MULTIFUNCTION", DEVICE_CLASS::MULTIFUNCTION},
            {"MULTIPORTSERIAL", DEVICE_CLASS::MULTIPORTSERIAL},
            {"NET", DEVICE_CLASS::NET},
            {"NETCLIENT", DEVICE_CLASS::NET_CLIENT},
            {"NETDRIVER", DEVICE_CLASS::NET_DRIVER},
            {"NETSERVICE", DEVICE_CLASS::NET_SERVICE},
            {"NETTRANS", DEVICE_CLASS::NET_TRANS},
            {"NETUIO", DEVICE_CLASS::NET_UIO},
            {"NODRIVER", DEVICE_CLASS::NODRIVER},
            {"PCMCIA", DEVICE_CLASS::PCMCIA},
            {"NPNPRINTERS", DEVICE_CLASS::NPN_PRINTERS},
            {"PORTS", DEVICE_CLASS::PORTS},
            {"PRIMITIVE", DEVICE_CLASS::PRIMITIVE},
            {"PRINTER", DEVICE_CLASS::PRINTER},
            {"PRINTERUPGRADE", DEVICE_CLASS::PRINTER_RUPGRADE},
            {"PRINTQUEUE", DEVICE_CLASS::PRINT_QUEUE},
            {"PROCESSOR", DEVICE_CLASS::PROCESSOR},
            {"SBP2", DEVICE_CLASS::SBP2},
            {"SCMDISK", DEVICE_CLASS::SCM_DISK},
            {"SCMVOLUME", DEVICE_CLASS::SCM_VOLUME},
            {"SCSIADAPTER", DEVICE_CLASS::SCSI_ADAPTER},
            {"SECURITYACCELERATOR", DEVICE_CLASS::SECURITY_ACCELERATOR},
            {"SENSOR", DEVICE_CLASS::SENSOR},
            {"SIDESHOW", DEVICE_CLASS::SIDE_SHOW},
            {"SMARTCARDREADER", DEVICE_CLASS::SMART_CARDREADER},
            {"SMRDISK", DEVICE_CLASS::SMR_DISK},
            {"SMRVOLUME", DEVICE_CLASS::SMR_VOLUME},
            {"SOFTWARECOMPONENT", DEVICE_CLASS::SOFTWARE_COMPONENT},
            {"SOUND", DEVICE_CLASS::SOUND},
            {"SYSTEM", DEVICE_CLASS::SYSTEM},
            {"TAPEDRIVE", DEVICE_CLASS::TAPE_DRIVE},
            {"THERMAL", DEVICE_CLASS::THERMAL},
            {"UCM", DEVICE_CLASS::UCM},
            {"USB", DEVICE_CLASS::USB},
            {"VOLUME", DEVICE_CLASS::VOLUME},
            {"VOLUMESNAPSHOT", DEVICE_CLASS::VOLUME_SNAPSHOT},
            {"WCEUSBS", DEVICE_CLASS::WCEUSBS},
            {"WPD", DEVICE_CLASS::WPD}};

    const auto it = class_map.find(upper);
    return (it != class_map.end()) ? it->second : DEVICE_CLASS::UNKNOWN;
}

bool device::enable() {
    if (dev_inst_ == 0) return false;
    const CONFIGRET ret = CM_Enable_DevNode(dev_inst_, 0);
    if (ret == CR_SUCCESS) {
        refresh_status();
        return true;
    }
    return false;
}

bool device::disable() {
    if (dev_inst_ == 0) return false;
    const CONFIGRET ret = CM_Disable_DevNode(dev_inst_, 0);
    if (ret == CR_SUCCESS) {
        refresh_status();
        return true;
    }
    return false;
}

bool device::restart() {
    if (dev_inst_ == 0) return false;
    CONFIGRET ret = CM_Query_And_Remove_SubTreeA(
        dev_inst_, nullptr,
        nullptr, 0,
        CM_REMOVE_NO_RESTART
        );
    if (ret == CR_SUCCESS) {
        ret = CM_Setup_DevNode(dev_inst_, CM_SETUP_DEVNODE_READY);
        refresh_status();
        return ret == CR_SUCCESS;
    }
    return false;
}

bool device::uninstall() const {
    if (dev_inst_ == 0) return false;
    const CONFIGRET ret = CM_Uninstall_DevNode(dev_inst_, 0);
    return ret == CR_SUCCESS;
}

string device::device_property(const HDEVINFO deviceInfoSet,
    const PSP_DEVINFO_DATA deviceInfoData, const DWORD property) {
    DWORD dataType;
    DWORD bufferSize = 0;

    SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet, deviceInfoData, property,
        &dataType, nullptr,
        0, &bufferSize
        );
    if (bufferSize == 0) return "";

    wstring wBuffer(bufferSize / sizeof(wchar_t) + 1);
    if (!SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet, deviceInfoData, property,
        &dataType, reinterpret_cast<PBYTE>(wBuffer.data()),
        bufferSize, nullptr)
        ) return "";

    return _MSTL to_string(wBuffer);
}

vector<device> device::enumerate_all() {
    vector<device> devices;

    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        nullptr,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );
    if (deviceInfoSet == INVALID_HANDLE_VALUE) return devices;

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD deviceIndex = 0; SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData); deviceIndex++) {
        string name = device_property(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC);
        string hardware_id = device_property(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID);
        DEVICE_CLASS device_class = to_device_class(device_property(deviceInfoSet, &deviceInfoData, SPDRP_CLASS));
        string manufacturer = device_property(deviceInfoSet, &deviceInfoData, SPDRP_MFG);
        string location = device_property(deviceInfoSet, &deviceInfoData, SPDRP_LOCATION_INFORMATION);

        DWORD status, problemNumber;
        bool is_present = (CM_Get_DevNode_Status(&status, &problemNumber, deviceInfoData.DevInst, 0) == CR_SUCCESS);

        devices.emplace_back(name, hardware_id, device_class, manufacturer, location, is_present);
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return devices;
}

vector<device> device::enumerate_by_class(const GUID& classGuid) {
    vector<device> devices;

    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &classGuid,
        nullptr,
        nullptr,
        DIGCF_PRESENT
    );
    if (deviceInfoSet == INVALID_HANDLE_VALUE) return devices;

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD deviceIndex = 0; SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData); deviceIndex++) {
        string name = device_property(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC);
        string hardware_id = device_property(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID);
        DEVICE_CLASS device_class = to_device_class(device_property(deviceInfoSet, &deviceInfoData, SPDRP_CLASS));
        string manufacturer = device_property(deviceInfoSet, &deviceInfoData, SPDRP_MFG);
        string location = device_property(deviceInfoSet, &deviceInfoData, SPDRP_LOCATION_INFORMATION);
        bool is_present = true;

        devices.emplace_back(name, hardware_id, device_class, manufacturer, location, is_present);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return devices;
}

string device::to_string() const {
    string res;
    res += "设备名称: " + (name_.empty() ? "UNKNOWN"_s : name_) + "\n";
    res += "硬件ID: " + (hardware_id_.empty() ? "UNKNOWN"_s : hardware_id_) + "\n";
    res += "设备类别: " + _MSTL to_string(static_cast<size_t>(class_)) + "\n";
    res += "制造商: " + (manufacturer_.empty() ? "UNKNOWN"_s : manufacturer_) + "\n";
    res += "位置: " + (location_.empty() ? "UNKNOWN"_s : location_) + "\n";
    res += "状态: " + is_present_ ? "LINKED"_s : "UNLINK"_s + "\n";
    return res;
}


string diskdrive::format_size(const size_t bytes) {
    constexpr const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit_index = 0;

    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    return _MSTL format("{.2f} ", size) + units[unit_index];
}

bool diskdrive::get_drive_geometry(const HANDLE hDevice, DISK_GEOMETRY& geometry) {
    DWORD bytesReturned;
    return DeviceIoControl(
        hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY,
        nullptr, 0, &geometry, sizeof(geometry),
        &bytesReturned, nullptr
        );
}

bool diskdrive::get_drive_layout(const HANDLE hDevice, DRIVE_LAYOUT_INFORMATION_EX &layout) {
    DWORD bytesReturned;
    constexpr DWORD bufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 128;
    vector<BYTE> buffer(bufferSize);

    return DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                         nullptr, 0, buffer.data(), bufferSize,
                         &bytesReturned, nullptr);
}

bool diskdrive::disable_device_interface() const {
    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_DISK,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );
    if (deviceInfoSet == INVALID_HANDLE_VALUE) return false;

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr,
        &GUID_DEVINTERFACE_DISK, i, &deviceInterfaceData); i++) {

        DWORD requiredSize;
        SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet, &deviceInterfaceData,
            nullptr, 0,
            &requiredSize, nullptr
            );

        vector<BYTE> detailData(requiredSize);
        const auto detailDataPtr = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailData.data());
        detailDataPtr->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &deviceInterfaceData,
            detailDataPtr, requiredSize, nullptr, &deviceInfoData)) {
            string currentDevicePath = _MSTL to_string(detailDataPtr->DevicePath);

            if (currentDevicePath.find(device_path_) != string::npos) {
                SP_PROPCHANGE_PARAMS params;
                params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
                params.StateChange = DICS_DISABLE;
                params.Scope = DICS_FLAG_GLOBAL;
                params.HwProfile = 0;

                if (SetupDiSetClassInstallParamsW(deviceInfoSet, &deviceInfoData,
                    &params.ClassInstallHeader, sizeof(params)))
                    {
                    SetupDiChangeState(deviceInfoSet, &deviceInfoData);
                }
                SetupDiDestroyDeviceInfoList(deviceInfoSet);
                return true;
            }
        }
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return false;
}

bool diskdrive::update_volume_info() {
    if (device_path_.empty()) return false;
    drive_type_ = static_cast<DISKDRIVE_TYPE>(GetDriveTypeA(device_path_.c_str()));

    ULARGE_INTEGER totalBytes, freeBytes, availableBytes;
    if (GetDiskFreeSpaceExA(
        device_path_.c_str(),
        &availableBytes,
        &totalBytes,
        &freeBytes
        )) {
        total_capacity_ = totalBytes.QuadPart;
        free_capacity_ = freeBytes.QuadPart;
        used_capacity_ = total_capacity_ - free_capacity_;
    }

    char volumeNameBuffer[MAX_PATH];
    char fileSystemBuffer[MAX_PATH];
    DWORD serialNumber, maxComponentLength, fileSystemFlags;

    if (GetVolumeInformationA(
        device_path_.c_str(),
        volumeNameBuffer, MAX_PATH,
        &serialNumber, &maxComponentLength,
        &fileSystemFlags, fileSystemBuffer, MAX_PATH
        )) {
        volume_label_ = volumeNameBuffer;
        file_system_ = fileSystemBuffer;
        serial_number_ = serialNumber;
                            }

    const DWORD attributes = ::GetFileAttributesA(device_path_.c_str());
    is_read_only_ = (attributes & FILE_ATTRIBUTE_READONLY);
    is_removable_ = (drive_type_ == DISKDRIVE_TYPE::REMOVABLE);

    return true;
}

bool diskdrive::eject() {
    if (!is_removable_) return false;

    char driveRoot[4] = { device_path_[0], ':', '\\', '\0'};
    return FALSE != PostMessageA(
        HWND_BROADCAST, WM_DEVICECHANGE,
        DBT_DEVICEREMOVECOMPLETE, reinterpret_cast<LPARAM>(driveRoot)
        );
}

vector<diskdrive> diskdrive::enumerate_all() {
    vector<diskdrive> drives;

    DWORD driveMask = GetLogicalDrives();
    if (driveMask == 0) return drives;

    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (driveMask & 1) {
            string device_path = string(1, drive) + ":\\";
            device base_drive;
            base_drive = device("Disk Drive", "", DEVICE_CLASS::DISK_DRIVE, "", device_path, true);
            diskdrive drive_obj(base_drive, device_path);
            drives.push_back(drive_obj);
        }
        driveMask >>= 1;
    }
    return drives;
}

vector<diskdrive> diskdrive::enumerate_physical_drives() {
    vector<diskdrive> drives;

    const HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_DISK,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE) return drives;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD deviceIndex = 0; SetupDiEnumDeviceInterfaces(
        deviceInfoSet, nullptr, &GUID_DEVINTERFACE_DISK, deviceIndex, &deviceInterfaceData);
        deviceIndex++) {
        DWORD requiredSize;
        SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet, &deviceInterfaceData,
            nullptr, 0,
            &requiredSize, nullptr);

        vector<BYTE> detailData(requiredSize);
        const auto detailDataPtr = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailData.data());
        detailDataPtr->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetailW(deviceInfoSet,
            &deviceInterfaceData, detailDataPtr,
            requiredSize, nullptr, &deviceInfoData)) {

            string name = device::device_property(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC);
            string hardware_id = device::device_property(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID);
            string manufacturer = device::device_property(deviceInfoSet, &deviceInfoData, SPDRP_MFG);

            device base_drive(name, hardware_id, DEVICE_CLASS::DISK_DRIVE, manufacturer, "", true);
            string device_path = _MSTL to_string(detailDataPtr->DevicePath);
            drives.emplace_back(base_drive, device_path);
        }
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return drives;
}

bool diskdrive::unmount_volume() const {
    if (device_path_.empty()) return false;
    string volume_path = device_path_;
    if (volume_path.back() == '\\') {
        volume_path.pop_back();
    }

    wchar_t volume_guid_path[MAX_PATH];

    if (!GetVolumeNameForVolumeMountPointW(
        _MSTL to_wstring(volume_path).c_str(),
        volume_guid_path, MAX_PATH)
        ) return false;

    if (!DeleteVolumeMountPointW(
        _MSTL to_wstring(device_path_).c_str())
        ) return false;

    return true;
}

bool diskdrive::mount_volume() const {
    if (device_path_.empty()) return false;
    wchar_t volume_guid_path[MAX_PATH];

    GetVolumeNameForVolumeMountPointW(
        _MSTL to_wstring(device_path_).c_str(),
        volume_guid_path, MAX_PATH
        );
    return SetVolumeMountPointW(_MSTL to_wstring(device_path_).c_str(), volume_guid_path);
}

bool diskdrive::lock_volume() {
    if (device_path_.empty()) return false;

    const string volume_path = "\\\\.\\" + string(1, device_path_[0]) + ":";
    const HANDLE hVolume = CreateFileA(
        volume_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hVolume == INVALID_HANDLE_VALUE) return false;

    DWORD bytesReturned;
    const BOOL result = DeviceIoControl(
        hVolume,
        FSCTL_LOCK_VOLUME,
        nullptr, 0,
        nullptr, 0,
        &bytesReturned,
        nullptr
    );
    CloseHandle(hVolume);
    return result;
}

bool diskdrive::dismount_volume() {
    if (device_path_.empty()) return false;

    const string volume_path = "\\\\.\\" + string(1, device_path_[0]) + ":";
    const HANDLE hVolume = CreateFileA(
        volume_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hVolume == INVALID_HANDLE_VALUE) return false;

    DWORD bytesReturned;
    const BOOL result = DeviceIoControl(
        hVolume,
        FSCTL_DISMOUNT_VOLUME,
        nullptr, 0,
        nullptr, 0,
        &bytesReturned,
        nullptr
    );
    CloseHandle(hVolume);
    return result;
}

bool diskdrive::force_dismount() const {
    if (device_path_.empty()) return false;

    wchar_t volume_name[MAX_PATH];
    string root_path = device_path_;
    if (root_path.back() == '\\') {
        root_path.pop_back();
    }

    if (!GetVolumeNameForVolumeMountPointW(
        _MSTL to_wstring(root_path).c_str(),
        volume_name, MAX_PATH)
        ) return false;

    if (!DeleteVolumeMountPointW(_MSTL to_wstring(device_path_).c_str())) return false;
    return true;
}

bool diskdrive::prevent_access() {
    if (device_path_.empty()) return false;
    force_dismount();
    base_drive_.disable();
    disable_device_interface();
    Sleep(3000);
    return true;
}

string diskdrive::to_string() const {
    string res = base_drive_.to_string();
    res += "设备路径: " + (device_path_.empty() ? "UNKNOWN"_s : device_path_) + "\n";
    res += "卷路径: " + (volume_path_.empty() ? "UNKNOWN"_s : volume_path_) + "\n";
    res += "卷标: " + (volume_label_.empty() ? "NULL"_s : volume_label_) + "\n";
    res += "文件系统: " + (file_system_.empty() ? "UNKNOWN"_s : file_system_) + "\n";
    res += "驱动器类型: " + _MSTL to_string(drive_type_) + "\n";
    res += "总容量: " + total_capacity() + "\n";
    res += "已用容量: " + used_capacity() + "\n";
    res += "可用容量: " + free_capacity() + "\n";
    res += "使用率: " + _MSTL to_string(usage_percentage()) + "%\n";
    res += "序列号: " + _MSTL to_string(serial_number_) + "\n";
    return res;
}

MSTL_END_NAMESPACE__
#endif
