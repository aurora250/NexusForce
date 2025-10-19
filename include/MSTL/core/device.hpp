#ifndef MSTL_DEVICE_HPP__
#define MSTL_DEVICE_HPP__
#include "environment.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include "vector.hpp"
#include "unordered_map.hpp"
#include "vsprintf.hpp"
#include <SetupAPI.h>
#include <devguid.h>
#include <Dbt.h>
#include <winioctl.h>
#pragma comment(lib, "SetupAPI.lib")
#include <cfgmgr32.h>
#pragma comment(lib, "cfgmgr32.lib")
MSTL_BEGIN_NAMESPACE__

enum class DEVICE_CLASS {
    C1394, C1394DEBUG, C61833, ADAPTER, APM_SUPPORT, AUDIO_PROCESSING_OBJECT,
    AVC, BATTERY, BIO_METRIC, BLUETOOTH, CAMERA, CDROM, COMPUTE_ACCELERATOR,
    COMPUTER, DECODER, DISK_DRIVE, DISPLAY, DOT4, DOT4_PRINT, EHSTORAGESILO,
    ENUM1394, EXTENSION, FDC, FIRMWARE, FLOPPY_DISK, GENERIC, GPS, HDC,
    HID_CLASS, HOLOGRAPHIC, I3C, IMAGE, INFINIBAND, KEYBOARD, LEGACY_DRIVER,
    MEDIA, MEDIUM_CHANGER, MEMORY, MODEM, MTD, MULTIFUNCTION, MULTIPORTSERIAL,
    NET, NET_CLIENT, NET_DRIVER, NET_SERVICE, NET_TRANS, NET_UIO, NODRIVER,
    PCMCIA, NPN_PRINTERS, PORTS, PRIMITIVE, PRINTER, PRINTER_RUPGRADE,
    PRINT_QUEUE, PROCESSOR, SBP2, SCM_DISK, SCM_VOLUME, SCSI_ADAPTER,
    SECURITY_ACCELERATOR, SENSOR, SIDE_SHOW, SMART_CARDREADER, SMR_DISK,
    SMR_VOLUME, SOFTWARE_COMPONENT, SOUND, SYSTEM, TAPE_DRIVE, THERMAL, UNKNOWN,
    UCM, USB, VOLUME, VOLUME_SNAPSHOT, WCEUSBS, WPD
};


class device : public istringify<device> {
private:
    DEVINST dev_inst_;
    DWORD status_;
    DWORD problem_code_;
    string name_;
    string hardware_id_;
    DEVICE_CLASS class_;
    string manufacturer_;
    string location_;
    bool is_present_ = false;

    void refresh_status() {
        if (dev_inst_ != 0) {
            CM_Get_DevNode_Status(&status_, &problem_code_, dev_inst_, 0);
        }
    }

    static DEVICE_CLASS to_device_class(const string& class_str) {
        string upper = class_str;
        upper.uppercase();

        static const unordered_map<string, DEVICE_CLASS> class_map = {
            { "1394", DEVICE_CLASS::C1394 },
            { "1394DEBUG", DEVICE_CLASS::C1394DEBUG },
            { "61883", DEVICE_CLASS::C61833 },
            { "ADAPTER", DEVICE_CLASS::ADAPTER },
            { "APMSUPPORT", DEVICE_CLASS::APM_SUPPORT },
            { "AUDIOPROCESSINGOBJECT", DEVICE_CLASS::AUDIO_PROCESSING_OBJECT },
            { "AVC", DEVICE_CLASS::AVC },
            { "BATTERY", DEVICE_CLASS::BATTERY },
            { "BIOMETRIC", DEVICE_CLASS::BIO_METRIC },
            { "BLUETOOTH", DEVICE_CLASS::BLUETOOTH },
            { "CAMERA", DEVICE_CLASS::CAMERA },
            { "CDROM", DEVICE_CLASS::CDROM },
            { "COMPUTEACCELERATOR", DEVICE_CLASS::COMPUTE_ACCELERATOR },
            { "COMPUTER", DEVICE_CLASS::COMPUTER },
            { "DECODER", DEVICE_CLASS::DECODER },
            { "DISKDRIVE", DEVICE_CLASS::DISK_DRIVE },
            { "DISPLAY", DEVICE_CLASS::DISPLAY },
            { "DOT4", DEVICE_CLASS::DOT4 },
            { "DOT4PRINT", DEVICE_CLASS::DOT4_PRINT },
            { "EHSTORAGESILO", DEVICE_CLASS::EHSTORAGESILO },
            { "ENUM1394", DEVICE_CLASS::ENUM1394 },
            { "EXTENSION", DEVICE_CLASS::EXTENSION },
            { "FDC", DEVICE_CLASS::FDC },
            { "FIRMWARE", DEVICE_CLASS::FIRMWARE },
            { "FLOPPYDISK", DEVICE_CLASS::FLOPPY_DISK },
            { "GENERIC", DEVICE_CLASS::GENERIC },
            { "GPS", DEVICE_CLASS::GPS },
            { "HDC", DEVICE_CLASS::HDC },
            { "HIDCLASS", DEVICE_CLASS::HID_CLASS },
            { "HOLOGRAPHIC", DEVICE_CLASS::HOLOGRAPHIC },
            { "I3C", DEVICE_CLASS::I3C },
            { "IMAGE", DEVICE_CLASS::IMAGE },
            { "INFINIBAND", DEVICE_CLASS::INFINIBAND },
            { "KEYBOARD", DEVICE_CLASS::KEYBOARD },
            { "LEGACYDRIVER", DEVICE_CLASS::LEGACY_DRIVER },
            { "MEDIA", DEVICE_CLASS::MEDIA },
            { "MEDIUMCHANGER", DEVICE_CLASS::MEDIUM_CHANGER },
            { "MEMORY", DEVICE_CLASS::MEMORY },
            { "MODEM", DEVICE_CLASS::MODEM },
            { "MTD", DEVICE_CLASS::MTD },
            { "MULTIFUNCTION", DEVICE_CLASS::MULTIFUNCTION },
            { "MULTIPORTSERIAL", DEVICE_CLASS::MULTIPORTSERIAL },
            { "NET", DEVICE_CLASS::NET },
            { "NETCLIENT", DEVICE_CLASS::NET_CLIENT },
            { "NETDRIVER", DEVICE_CLASS::NET_DRIVER },
            { "NETSERVICE", DEVICE_CLASS::NET_SERVICE },
            { "NETTRANS", DEVICE_CLASS::NET_TRANS },
            { "NETUIO", DEVICE_CLASS::NET_UIO },
            { "NODRIVER", DEVICE_CLASS::NODRIVER },
            { "PCMCIA", DEVICE_CLASS::PCMCIA },
            { "NPNPRINTERS", DEVICE_CLASS::NPN_PRINTERS },
            { "PORTS", DEVICE_CLASS::PORTS },
            { "PRIMITIVE", DEVICE_CLASS::PRIMITIVE },
            { "PRINTER", DEVICE_CLASS::PRINTER },
            { "PRINTERUPGRADE", DEVICE_CLASS::PRINTER_RUPGRADE },
            { "PRINTQUEUE", DEVICE_CLASS::PRINT_QUEUE },
            { "PROCESSOR", DEVICE_CLASS::PROCESSOR },
            { "SBP2", DEVICE_CLASS::SBP2 },
            { "SCMDISK", DEVICE_CLASS::SCM_DISK },
            { "SCMVOLUME", DEVICE_CLASS::SCM_VOLUME },
            { "SCSIADAPTER", DEVICE_CLASS::SCSI_ADAPTER },
            { "SECURITYACCELERATOR", DEVICE_CLASS::SECURITY_ACCELERATOR },
            { "SENSOR", DEVICE_CLASS::SENSOR },
            { "SIDESHOW", DEVICE_CLASS::SIDE_SHOW },
            { "SMARTCARDREADER", DEVICE_CLASS::SMART_CARDREADER },
            { "SMRDISK", DEVICE_CLASS::SMR_DISK },
            { "SMRVOLUME", DEVICE_CLASS::SMR_VOLUME },
            { "SOFTWARECOMPONENT", DEVICE_CLASS::SOFTWARE_COMPONENT },
            { "SOUND", DEVICE_CLASS::SOUND },
            { "SYSTEM", DEVICE_CLASS::SYSTEM },
            { "TAPEDRIVE", DEVICE_CLASS::TAPE_DRIVE },
            { "THERMAL", DEVICE_CLASS::THERMAL },
            { "UCM", DEVICE_CLASS::UCM },
            { "USB", DEVICE_CLASS::USB },
            { "VOLUME", DEVICE_CLASS::VOLUME },
            { "VOLUMESNAPSHOT", DEVICE_CLASS::VOLUME_SNAPSHOT },
            { "WCEUSBS", DEVICE_CLASS::WCEUSBS },
            { "WPD", DEVICE_CLASS::WPD }
        };

        auto it = class_map.find(upper);
        return (it != class_map.end()) ? it->second : DEVICE_CLASS::UNKNOWN;
    }

public:
    explicit device(
        const string& name = "", const string& hardware_id = "",
        const DEVICE_CLASS device_class = DEVICE_CLASS::UNKNOWN, const string& manufacturer = "",
        const string& location = "", bool is_present = false)
    : name_(name), hardware_id_(hardware_id), class_(device_class),
      manufacturer_(manufacturer), location_(location), is_present_(is_present) {}

    const string& name() const { return name_; }
    const string& hardware_id() const { return hardware_id_; }
    DEVICE_CLASS device_class() const { return class_; }
    const string& manufacturer() const { return manufacturer_; }
    const string& location() const { return location_; }
    bool is_present() const { return is_present_; }

    bool enable() {
        if (dev_inst_ == 0) return false;
        CONFIGRET ret = CM_Enable_DevNode(dev_inst_, 0);
        if (ret == CR_SUCCESS) {
            refresh_status();
            return true;
        }
        return false;
    }

    bool disable() {
        if (dev_inst_ == 0)
            return false;
        const CONFIGRET ret = CM_Disable_DevNode(dev_inst_, 0);
        if (ret == CR_SUCCESS) {
            refresh_status();
            return true;
        }
        return false;
    }

    bool restart() {
        if (dev_inst_ == 0) return false;
        CONFIGRET ret = CM_Query_And_Remove_SubTree(dev_inst_, nullptr, nullptr, 0, CM_REMOVE_NO_RESTART);
        if (ret == CR_SUCCESS) {
            ret = CM_Setup_DevNode(dev_inst_, CM_SETUP_DEVNODE_READY);
            refresh_status();
            return ret == CR_SUCCESS;
        }
        return false;
    }

    bool uninstall() {
        if (dev_inst_ == 0) return false;
        CONFIGRET ret = CM_Uninstall_DevNode(dev_inst_, 0);
        return ret == CR_SUCCESS;
    }


    static string device_property(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property) {
        DWORD dataType;
        DWORD bufferSize = 0;

        SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, deviceInfoData, property,
            &dataType, nullptr, 0, &bufferSize
        );
        if (bufferSize == 0) {
            return "";
        }

        wstring wBuffer(bufferSize / sizeof(wchar_t) + 1);
        if (!SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, deviceInfoData, property,
            &dataType, reinterpret_cast<PBYTE>(wBuffer.data()), bufferSize, nullptr)
        ) {
            return "";
        }

        return _MSTL to_string(wBuffer);
    }

    static vector<device> enumerate_all() {
        vector<device> devices;

        HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
            nullptr,
            nullptr,
            nullptr,
            DIGCF_PRESENT | DIGCF_ALLCLASSES
        );

        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            return devices;
        }

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD deviceIndex = 0;
             SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData);
             deviceIndex++) {

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

    static vector<device> enumerate_by_class(const GUID& classGuid) {
        vector<device> devices;

        HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
            &classGuid,
            nullptr,
            nullptr,
            DIGCF_PRESENT
        );

        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            return devices;
        }

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD deviceIndex = 0;
             SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData);
             deviceIndex++) {

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

    string to_string() const {
        string res;
        res += "设备名称: " + (name_.empty() ? "UNKNOWN" : name_) + "\n";
        res += "硬件ID: " + (hardware_id_.empty() ? "UNKNOWN" : hardware_id_) + "\n";
        res += "设备类别: " + _MSTL to_string(static_cast<size_t>(class_)) + "\n";
        res += "制造商: " + (manufacturer_.empty() ? "UNKNOWN" : manufacturer_) + "\n";
        res += "位置: " + (location_.empty() ? "UNKNOWN" : location_) + "\n";
        res += "状态: " + string(is_present_ ? "LINKED" : "UNLINK") + "\n";
        return res;
    }
};


enum class DISKDRIVE_TYPE {
    UNKNOWN = DRIVE_UNKNOWN,
    NO_ROOT_DIR = DRIVE_NO_ROOT_DIR,
    REMOVABLE = DRIVE_REMOVABLE,
    FIXED = DRIVE_FIXED,
    REMOTE = DRIVE_REMOTE,
    CDROM = DRIVE_CDROM,
    RAMDISK = DRIVE_RAMDISK,
};

MSTL_CONSTEXPR20 string to_string(const DISKDRIVE_TYPE& e) {
    switch (e) {
        case DISKDRIVE_TYPE::UNKNOWN: return "UNKNOWN";
        case DISKDRIVE_TYPE::NO_ROOT_DIR: return "NO_ROOT_DIR";
        case DISKDRIVE_TYPE::REMOVABLE: return "REMOVABLE";
        case DISKDRIVE_TYPE::FIXED: return "FIXED";
        case DISKDRIVE_TYPE::REMOTE: return "REMOTE";
        case DISKDRIVE_TYPE::CDROM: return "CDROM";
        case DISKDRIVE_TYPE::RAMDISK: return "RAMDISK";
        default: return "UNKNOWN";
    }
}


class diskdrive : public istringify<diskdrive> {
private:
    device base_drive_;
    string device_path_;
    string volume_path_;
    string volume_label_;
    string file_system_;
    size_t total_capacity_ = 0;
    size_t free_capacity_ = 0;
    size_t used_capacity_ = 0;
    DISKDRIVE_TYPE drive_type_ = DISKDRIVE_TYPE::UNKNOWN;
    size_t serial_number_ = 0;
    bool is_removable_ = false;
    bool is_read_only_ = false;

    static string format_size(const size_t bytes) {
        constexpr const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        double size = static_cast<double>(bytes);
        int unit_index = 0;

        while (size >= 1024.0 && unit_index < 4) {
            size /= 1024.0;
            unit_index++;
        }
        return _MSTL format("{.2f} ", size) + units[unit_index];
    }

    static bool get_drive_geometry(const HANDLE hDevice, DISK_GEOMETRY& geometry) {
        DWORD bytesReturned;
        return DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                             nullptr, 0, &geometry, sizeof(geometry),
                             &bytesReturned, nullptr);
    }

    static bool get_drive_layout(HANDLE hDevice, DRIVE_LAYOUT_INFORMATION_EX& layout) {
        DWORD bytesReturned;
        DWORD bufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 128;
        vector<BYTE> buffer(bufferSize);

        return DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                             nullptr, 0, buffer.data(), bufferSize,
                             &bytesReturned, nullptr);
    }

public:
    diskdrive() = default;

    explicit diskdrive(const device& base_drive, const string& device_path = "")
        : base_drive_(base_drive), device_path_(device_path) {
        // 尝试获取卷信息
        update_volume_info();
    }

    const device& base_drive() const { return base_drive_; }
    const string& device_path() const { return device_path_; }
    const string& volume_path() const { return volume_path_; }
    const string& volume_label() const { return volume_label_; }
    const string& file_system() const { return file_system_; }
    size_t total_capacity_bytes() const { return total_capacity_; }
    size_t free_capacity_bytes() const { return free_capacity_; }
    size_t used_capacity_bytes() const { return used_capacity_; }
    DISKDRIVE_TYPE drive_type() const { return drive_type_; }
    size_t serial_number() const { return serial_number_; }
    bool is_removable() const { return is_removable_; }
    bool is_read_only() const { return is_read_only_; }

    string total_capacity() const { return format_size(total_capacity_); }
    string free_capacity() const { return format_size(free_capacity_); }
    string used_capacity() const { return format_size(used_capacity_); }

    double usage_percentage() const {
        if (total_capacity_ == 0) return 0.0;
        return (static_cast<double>(used_capacity_) / total_capacity_) * 100.0;
    }

    bool update_volume_info() {
        if (device_path_.empty()) return false;

        // 获取驱动器类型
        drive_type_ = static_cast<DISKDRIVE_TYPE>(GetDriveTypeA(device_path_.c_str()));

        // 获取磁盘空间信息
        ULARGE_INTEGER totalBytes, freeBytes, availableBytes;
        if (GetDiskFreeSpaceExA(device_path_.c_str(), &availableBytes, &totalBytes, &freeBytes)) {
            total_capacity_ = totalBytes.QuadPart;
            free_capacity_ = freeBytes.QuadPart;
            used_capacity_ = total_capacity_ - free_capacity_;
        }

        // 获取卷信息
        char volumeNameBuffer[MAX_PATH];
        char fileSystemBuffer[MAX_PATH];
        DWORD serialNumber, maxComponentLength, fileSystemFlags;

        if (GetVolumeInformationA(device_path_.c_str(),
                                volumeNameBuffer, MAX_PATH,
                                &serialNumber, &maxComponentLength,
                                &fileSystemFlags, fileSystemBuffer, MAX_PATH)) {
            volume_label_ = volumeNameBuffer;
            file_system_ = fileSystemBuffer;
            serial_number_ = serialNumber;
                                }

        const DWORD attributes = ::GetFileAttributesA(device_path_.c_str());
        is_read_only_ = (attributes & FILE_ATTRIBUTE_READONLY);
        is_removable_ = (drive_type_ == DISKDRIVE_TYPE::REMOVABLE);

        return true;
    }

    bool eject() {
        if (!is_removable_) return false;

        char driveRoot[4] = { device_path_[0], ':', '\\', '\0'};
        return FALSE != PostMessageA(HWND_BROADCAST, WM_DEVICECHANGE, DBT_DEVICEREMOVECOMPLETE,
                                   reinterpret_cast<LPARAM>(driveRoot));
    }

    static vector<diskdrive> enumerate_all() {
        vector<diskdrive> drives;

        // 枚举逻辑驱动器
        DWORD driveMask = GetLogicalDrives();
        if (driveMask == 0) return drives;

        for (char drive = 'A'; drive <= 'Z'; drive++) {
            if (driveMask & 1) {
                string device_path = string(1, drive) + ":\\";

                // 创建基础设备信息
                device base_drive;
                base_drive = device("Disk Drive", "", DEVICE_CLASS::DISK_DRIVE, "", device_path, true);

                diskdrive drive_obj(base_drive, device_path);
                drives.push_back(drive_obj);
            }
            driveMask >>= 1;
        }

        return drives;
    }

    static vector<diskdrive> enumerate_physical_drives() {
        vector<diskdrive> drives;

        HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
            &GUID_DEVINTERFACE_DISK,
            nullptr,
            nullptr,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
        );

        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            return drives;
        }

        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        for (DWORD deviceIndex = 0; SetupDiEnumDeviceInterfaces(
            deviceInfoSet, nullptr, &GUID_DEVINTERFACE_DISK, deviceIndex, &deviceInterfaceData);
            deviceIndex++
            ) {

            DWORD requiredSize;
            SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &deviceInterfaceData,
                                           nullptr, 0, &requiredSize, nullptr);

            vector<BYTE> detailData(requiredSize);
            PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailDataPtr =
                reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailData.data());
            detailDataPtr->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

            SP_DEVINFO_DATA deviceInfoData;
            deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            if (SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet, &deviceInterfaceData, detailDataPtr, requiredSize, nullptr, &deviceInfoData))
                {

                string name = device::device_property(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC);
                string hardware_id = device::device_property(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID);
                string manufacturer = device::device_property(deviceInfoSet, &deviceInfoData, SPDRP_MFG);

                device base_drive(name, hardware_id, DEVICE_CLASS::DISK_DRIVE, manufacturer, "", true);

                // 转换设备路径
                wstring wPath = detailDataPtr->DevicePath;
                string device_path = _MSTL to_string(wPath);

                drives.emplace_back(base_drive, device_path);
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return drives;
    }

    string to_string() const {
        string res = base_drive_.to_string();
        res += "设备路径: " + (device_path_.empty() ? "UNKNOWN" : device_path_) + "\n";
        res += "卷路径: " + (volume_path_.empty() ? "UNKNOWN" : volume_path_) + "\n";
        res += "卷标: " + (volume_label_.empty() ? "NULL" : volume_label_) + "\n";
        res += "文件系统: " + (file_system_.empty() ? "UNKNOWN" : file_system_) + "\n";
        res += "驱动器类型: " + _MSTL to_string(drive_type_) + "\n";
        res += "总容量: " + total_capacity() + "\n";
        res += "已用容量: " + used_capacity() + "\n";
        res += "可用容量: " + free_capacity() + "\n";
        res += "使用率: " + _MSTL to_string(usage_percentage()) + "%\n";
        res += "序列号: " + _MSTL to_string(serial_number_) + "\n";
        return res;
    }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DEVICE_HPP__