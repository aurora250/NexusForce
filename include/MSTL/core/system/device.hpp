#ifndef MSTL_CORE_SYSTEM_DEVICE_HPP__
#define MSTL_CORE_SYSTEM_DEVICE_HPP__
#include "../config/c++config.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include "../container/vector.hpp"
#include "../interface/istringify.hpp"
#include <Windows.h>
#include <SetupAPI.h>
#pragma comment(lib, "SetupAPI.lib")
#include <winioctl.h>
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


class MSTL_API device : public istringify<device> {
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

    void refresh_status();
    static DEVICE_CLASS to_device_class(const string& class_str);

public:
    explicit device(
        const string &name = "", const string &hardware_id = "",
        const DEVICE_CLASS device_class = DEVICE_CLASS::UNKNOWN, const string &manufacturer = "",
        const string &location = "", const bool is_present = false)
    : dev_inst_(0), status_(0), problem_code_(0), name_(name), hardware_id_(hardware_id), class_(device_class),
    manufacturer_(manufacturer), location_(location), is_present_(is_present) {}

    const string& name() const { return name_; }
    const string& hardware_id() const { return hardware_id_; }
    DEVICE_CLASS device_class() const { return class_; }
    const string& manufacturer() const { return manufacturer_; }
    const string& location() const { return location_; }
    bool is_present() const { return is_present_; }

    bool enable();
    bool disable();

    bool restart();
    bool uninstall() const;

    static string device_property(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property);

    static vector<device> enumerate_all();
    static vector<device> enumerate_by_class(const GUID& classGuid);

    string to_string() const;
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

MSTL_CONSTEXPR20 string to_string(const DISKDRIVE_TYPE& dt) {
    switch (dt) {
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


class MSTL_API diskdrive : public istringify<diskdrive> {
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

    static string format_size(size_t bytes);

    static bool get_drive_geometry(HANDLE hDevice, DISK_GEOMETRY& geometry);
    static bool get_drive_layout(HANDLE hDevice, DRIVE_LAYOUT_INFORMATION_EX& layout);

    bool disable_device_interface() const;

public:
    diskdrive() = default;

    explicit diskdrive(const device& base_drive, const string& device_path = "")
    : base_drive_(base_drive), device_path_(device_path) {
        update_volume_info();
    }

    device& base_drive() { return base_drive_; }
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

    bool update_volume_info();
    bool eject();

    static vector<diskdrive> enumerate_all();
    static vector<diskdrive> enumerate_physical_drives();

    bool unmount_volume() const;
    bool mount_volume() const;

    bool lock_volume();
    bool dismount_volume();
    bool force_dismount() const;
    bool prevent_access();

    string to_string() const;
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_CORE_SYSTEM_DEVICE_HPP__