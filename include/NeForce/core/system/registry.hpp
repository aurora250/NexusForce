#ifndef NEFORCE_CORE_SYSTEM_REGISTRY_HPP__
#define NEFORCE_CORE_SYSTEM_REGISTRY_HPP__
#include "NeForce/core/config/windef.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include "NeForce/core/container/vector.hpp"
#    include "NeForce/core/string/string.hpp"
#    include <windef.h>
#    include <winreg.h>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct registry_key_exception
 * @brief 系统注册表操作异常
 */
struct registry_key_exception final : system_exception {
    explicit registry_key_exception(const char* info = "Registry Key Operation Failed.", const char* type = static_type,
                                    const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit registry_key_exception(const exception& e) :
    system_exception(e) {}

    ~registry_key_exception() override = default;
    static constexpr auto static_type = "registry_key_exception";
};

/** @} */ // Exceptions


class NEFORCE_API registry_key {
public:
    struct NEFORCE_API root_key {
        static ::HKEY classes_root() { return HKEY_CLASSES_ROOT; }
        static ::HKEY current_user() { return HKEY_CURRENT_USER; }
        static ::HKEY local_machine() { return HKEY_LOCAL_MACHINE; }
        static ::HKEY users() { return HKEY_USERS; }
        static ::HKEY current_config() { return HKEY_CURRENT_CONFIG; }
    };

    enum class value_type : ::DWORD {
        none = REG_NONE,
        string = REG_SZ,
        expand_string = REG_EXPAND_SZ,
        binary = REG_BINARY,
        dword = REG_DWORD,
        qword = REG_QWORD,
        multi_string = REG_MULTI_SZ
    };

    struct NEFORCE_API value_info {
        wstring name;
        value_type type;
        vector<byte_t> data;

        NEFORCE_NODISCARD wstring to_string() const;
        NEFORCE_NODISCARD ::DWORD to_dword() const;
        NEFORCE_NODISCARD ::ULONGLONG to_qword() const;
        NEFORCE_NODISCARD vector<wstring> to_multi_string() const;
    };

private:
    void close();
    void throw_if_invalid() const;
    NEFORCE_NODISCARD value_info get_value_info(const wstring& name) const;

    ::HKEY m_key = nullptr;
    bool m_owns_handle = false;

public:
    registry_key() = default;
    explicit registry_key(::HKEY key);
    ~registry_key();

    registry_key(const registry_key&) = delete;
    registry_key& operator=(const registry_key&) = delete;
    registry_key(registry_key&& other) noexcept;
    registry_key& operator=(registry_key&& other) noexcept;

    NEFORCE_NODISCARD bool is_valid() const noexcept { return m_key != nullptr; }
    explicit operator bool() const noexcept { return is_valid(); }

    void create_sub_key(const wstring& name);

    void open(::HKEY root, const wstring& path, ::REGSAM sam_desired = KEY_READ | KEY_WRITE);
    NEFORCE_NODISCARD registry_key open_sub_key(const wstring& name, REGSAM sam_desired = KEY_READ) const;

    void delete_sub_key(const wstring& name);
    void delete_value(const wstring& name);
    static void delete_key_tree(::HKEY root, const wstring& path);

    NEFORCE_NODISCARD bool has_sub_key(const wstring& name) const;
    NEFORCE_NODISCARD bool has_value(const wstring& name) const;

    NEFORCE_NODISCARD vector<wstring> enum_sub_key_names() const;
    NEFORCE_NODISCARD vector<value_info> enum_values() const;

    void set_string_value(const wstring& name, const wstring& value);
    void set_expand_string_value(const wstring& name, const wstring& value);
    void set_dword_value(const wstring& name, ::DWORD value);
    void set_qword_value(const wstring& name, ::ULONGLONG value);
    void set_binary_value(const wstring& name, const ::BYTE* data, ::DWORD size);
    void set_multi_string_value(const wstring& name, const vector<wstring>& values);

    NEFORCE_NODISCARD wstring get_string_value(const wstring& name, const wstring& default_val = L"") const;
    NEFORCE_NODISCARD ::DWORD get_dword_value(const wstring& name, DWORD default_val = 0) const;
    NEFORCE_NODISCARD ::ULONGLONG get_qword_value(const wstring& name, ULONGLONG default_val = 0) const;
    NEFORCE_NODISCARD vector<byte_t> get_binary_value(const wstring& name) const;
    NEFORCE_NODISCARD vector<wstring> get_multi_string_value(const wstring& name) const;

    NEFORCE_NODISCARD ::HKEY native_handle() const noexcept { return m_key; }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_SYSTEM_REGISTRY_HPP__
