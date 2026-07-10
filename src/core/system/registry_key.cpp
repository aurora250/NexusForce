#include <NeForce/core/system/registry_key.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <winerror.h>
#    include <synchapi.h>
#    include <handleapi.h>
#    include <WinBase.h>
NEFORCE_BEGIN_NAMESPACE__

wstring registry_key::value_info::to_string() const {
    if (type == value_type::string || type == value_type::expand_string) {
        if (!data.empty()) {
            return {reinterpret_cast<const wchar_t*>(data.data()), (data.size() / sizeof(wchar_t)) - 1};
        }
    }
    return L"";
}

::DWORD registry_key::value_info::to_dword() const noexcept {
    if (type == value_type::dword && data.size() >= sizeof(::DWORD)) {
        return *reinterpret_cast<const ::DWORD*>(data.data());
    }
    return 0;
}

::ULONGLONG registry_key::value_info::to_qword() const noexcept {
    if (type == value_type::qword && data.size() >= sizeof(::ULONGLONG)) {
        return *reinterpret_cast<const ::ULONGLONG*>(data.data());
    }
    return 0;
}

vector<wstring> registry_key::value_info::to_multi_string() const {
    vector<wstring> result;
    if (type != value_type::multi_string || data.empty()) {
        return result;
    }

    const auto* p = reinterpret_cast<const wchar_t*>(data.data());
    const size_t count = data.size() / sizeof(wchar_t);

    for (size_t i = 0; i < count && *p != 0U;) {
        wstring str(p);
        i += str.length() + 1;
        p += str.length() + 1;
        result.push_back(move(str));
    }
    return result;
}

registry_key::registry_key(const ::HKEY key) :
hkey_(key) {
    if (key != nullptr) {
        owns_handle_ = true;
    }
}

registry_key::~registry_key() { close(); }

registry_key::registry_key(registry_key&& other) noexcept :
hkey_(other.hkey_),
owns_handle_(other.owns_handle_) {
    other.hkey_ = nullptr;
    other.owns_handle_ = false;
}

registry_key& registry_key::operator=(registry_key&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    close();
    hkey_ = other.hkey_;
    owns_handle_ = other.owns_handle_;
    other.hkey_ = nullptr;
    other.owns_handle_ = false;

    return *this;
}

void registry_key::close() noexcept {
    if (hkey_ != nullptr && owns_handle_) {
        ::RegCloseKey(hkey_);
        hkey_ = nullptr;
        owns_handle_ = false;
    }
}

void registry_key::throw_if_invalid() const {
    if (hkey_ == nullptr) {
        throw registry_key_exception("Invalid registry key handle");
    }
}

void registry_key::create_sub_key(const wstring& name) {
    throw_if_invalid();
    ::HKEY sub_key = nullptr;
    ::DWORD disposition = 0;

    const ::LONG result = ::RegCreateKeyExW(hkey_, name.data(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                            nullptr, &sub_key, &disposition);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to create registry key");
    }
    ::RegCloseKey(sub_key);
}

void registry_key::open(const ::HKEY root, const wstring& path, const ::REGSAM sam_desired) {
    close();
    const ::LONG result = ::RegOpenKeyExW(root, path.data(), 0, sam_desired, &hkey_);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to open registry key");
    }
    owns_handle_ = true;
}

registry_key registry_key::open_sub_key(const wstring& name, const ::REGSAM sam_desired) const {
    throw_if_invalid();
    ::HKEY sub_key = nullptr;
    const ::LONG result = ::RegOpenKeyExW(hkey_, name.data(), 0, sam_desired, &sub_key);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to open sub key");
    }
    return registry_key(sub_key);
}

void registry_key::delete_sub_key(const wstring& name) {
    throw_if_invalid();
    const ::LONG result = ::RegDeleteKeyW(hkey_, name.data());
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to delete sub key");
    }
}

void registry_key::delete_value(const wstring& name) {
    throw_if_invalid();
    const ::LONG result = ::RegDeleteValueW(hkey_, name.data());
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to delete value");
    }
}

void registry_key::delete_key_tree(const ::HKEY root, const wstring& path) {
    const ::LONG result = ::RegDeleteTreeW(root, path.data());
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to delete key tree");
    }
}

bool registry_key::has_sub_key(const wstring& name) const {
    throw_if_invalid();
    ::HKEY sub_key = nullptr;
    const ::LONG result = ::RegOpenKeyExW(hkey_, name.data(), 0, KEY_READ, &sub_key);
    if (result == ERROR_SUCCESS) {
        ::RegCloseKey(sub_key);
        return true;
    }
    return false;
}

bool registry_key::has_value(const wstring& name) const {
    throw_if_invalid();
    ::DWORD type = 0;
    const ::LONG result = ::RegQueryValueExW(hkey_, name.data(), nullptr, &type, nullptr, nullptr);
    return (result == ERROR_SUCCESS);
}

vector<wstring> registry_key::enum_sub_key_names() const {
    throw_if_invalid();
    vector<wstring> names;
    ::DWORD index = 0;
    wchar_t name_buffer[256];
    ::DWORD name_size = 256;

    while (::RegEnumKeyExW(hkey_, index, name_buffer, &name_size, nullptr, nullptr, nullptr, nullptr) ==
           ERROR_SUCCESS) {
        names.push_back(name_buffer);
        index++;
        name_size = 256;
    }
    return names;
}

vector<registry_key::value_info> registry_key::enum_values() const {
    throw_if_invalid();
    vector<value_info> values;
    ::DWORD index = 0;
    wchar_t name_buffer[16384];
    ::DWORD name_size = 16384;
    ::DWORD type = 0;
    vector<::BYTE> data_buffer(65536);
    ::DWORD data_size = 65536;

    while (true) {
        name_size = 16384;
        data_size = 65536;
        const ::LONG result =
                ::RegEnumValueW(hkey_, index, name_buffer, &name_size, nullptr, &type, data_buffer.data(), &data_size);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (result != ERROR_SUCCESS) {
            throw registry_key_exception("Failed to enumerate values");
        }

        value_info info;
        info.name = name_buffer;
        info.type = static_cast<value_type>(type);
        info.data.assign(data_buffer.begin(), data_buffer.begin() + data_size);
        values.push_back(move(info));
        index++;
    }
    return values;
}

void registry_key::set_string_value(const wstring& name, const wstring& value) {
    throw_if_invalid();
    const auto size = static_cast<::DWORD>((value.length() + 1) * sizeof(wchar_t));
    const ::LONG result =
            ::RegSetValueExW(hkey_, name.data(), 0, REG_SZ, reinterpret_cast<const ::BYTE*>(value.data()), size);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to set string value");
    }
}

void registry_key::set_expand_string_value(const wstring& name, const wstring& value) {
    throw_if_invalid();
    const auto size = static_cast<::DWORD>((value.length() + 1) * sizeof(wchar_t));
    const ::LONG result =
            ::RegSetValueExW(hkey_, name.data(), 0, REG_EXPAND_SZ, reinterpret_cast<const ::BYTE*>(value.data()), size);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to set expand string value");
    }
}

void registry_key::set_dword_value(const wstring& name, const ::DWORD value) {
    throw_if_invalid();
    const ::LONG result = ::RegSetValueExW(hkey_, name.data(), 0, REG_DWORD, reinterpret_cast<const ::BYTE*>(&value),
                                           sizeof(::DWORD));
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to set DWORD value");
    }
}

void registry_key::set_qword_value(const wstring& name, const ::ULONGLONG value) {
    throw_if_invalid();
    const ::LONG result = ::RegSetValueExW(hkey_, name.data(), 0, REG_QWORD, reinterpret_cast<const ::BYTE*>(&value),
                                           sizeof(::ULONGLONG));
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to set QWORD value");
    }
}

void registry_key::set_binary_value(const wstring& name, const ::BYTE* data, const ::DWORD size) {
    throw_if_invalid();
    const ::LONG result = ::RegSetValueExW(hkey_, name.data(), 0, REG_BINARY, data, size);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to set binary value");
    }
}

void registry_key::set_multi_string_value(const wstring& name, const vector<wstring>& values) {
    throw_if_invalid();
    size_t total_size = sizeof(wchar_t);
    for (const auto& str: values) {
        total_size += (str.length() + 1) * sizeof(wchar_t);
    }

    vector<::BYTE> buffer(total_size);
    auto* p = reinterpret_cast<wchar_t*>(buffer.data());
    for (const auto& str: values) {
        string_copy(p, str.data(), total_size / sizeof(wchar_t) - (p - reinterpret_cast<wchar_t*>(buffer.data())));
        p += str.length() + 1;
    }
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    *p = L'\0';

    const ::LONG result =
            ::RegSetValueExW(hkey_, name.data(), 0, REG_MULTI_SZ, buffer.data(), static_cast<::DWORD>(total_size));
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to set multi-string value");
    }
}

registry_key::value_info registry_key::get_value_info(const wstring& name) const {
    throw_if_invalid();
    ::DWORD type = 0;
    ::DWORD data_size = 0;

    ::LONG result = ::RegQueryValueExW(hkey_, name.data(), nullptr, &type, nullptr, &data_size);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to query value info");
    }

    vector<::BYTE> data(data_size);
    result = ::RegQueryValueExW(hkey_, name.data(), nullptr, nullptr, data.data(), &data_size);
    if (result != ERROR_SUCCESS) {
        throw registry_key_exception("Failed to read value data");
    }

    value_info info;
    info.name = name;
    info.type = static_cast<value_type>(type);
    info.data = move(data);
    return info;
}

wstring registry_key::get_string_value(const wstring& name, const wstring& default_val) const {
    try {
        return get_value_info(name).to_string();
    } catch (...) {
        return default_val;
    }
}

::DWORD registry_key::get_dword_value(const wstring& name, const ::DWORD default_val) const noexcept {
    try {
        return get_value_info(name).to_dword();
    } catch (...) {
        return default_val;
    }
}

::ULONGLONG registry_key::get_qword_value(const wstring& name, const ::ULONGLONG default_val) const noexcept {
    try {
        return get_value_info(name).to_qword();
    } catch (...) {
        return default_val;
    }
}

vector<::BYTE> registry_key::get_binary_value(const wstring& name) const { return get_value_info(name).data; }

vector<wstring> registry_key::get_multi_string_value(const wstring& name) const {
    return get_value_info(name).to_multi_string();
}

void registry_key::open(const ::HKEY root, const wstring& path, const wow64_view view, const ::REGSAM sam_desired) {
    open(root, path, sam_desired | static_cast<::REGSAM>(view));
}

bool registry_key::notify_change(const bool watch_subtree, const int timeout_ms) {
    throw_if_invalid();

    constexpr ::DWORD filter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES | REG_NOTIFY_CHANGE_LAST_SET |
                               REG_NOTIFY_CHANGE_SECURITY;

    const ::HANDLE event = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (event == nullptr) {
        NEFORCE_THROW_EXCEPTION(registry_key_exception("CreateEvent failed for registry notify"));
    }

    const ::LONG ret = ::RegNotifyChangeKeyValue(hkey_, watch_subtree ? TRUE : FALSE, filter, event, TRUE);

    if (ret != ERROR_SUCCESS) {
        ::CloseHandle(event);
        NEFORCE_THROW_EXCEPTION(registry_key_exception("RegNotifyChangeKeyValue failed"));
    }

    const ::DWORD wait_ms = (timeout_ms < 0) ? INFINITE : static_cast<::DWORD>(timeout_ms);
    const ::DWORD wait_ret = ::WaitForSingleObject(event, wait_ms);
    ::CloseHandle(event);

    return wait_ret == WAIT_OBJECT_0;
}

NEFORCE_END_NAMESPACE__
#endif
