#include <NeForce/core/system/environment.hpp>
#include <NeForce/core/async/shared_mutex.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <processenv.h>
#include <urlmon.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <unistd.h>
#include <cstdlib>
#endif
NEFORCE_BEGIN_NAMESPACE__

static shared_mutex& get_mutex() {
    static shared_mutex mutex;
    return mutex;
}

static string get_unsafe(const string& name) {
#ifdef NEFORCE_COMPILER_MSVC
    char* value = nullptr;
    size_t size = 0;

    if (::_dupenv_s(&value, &size, name.data()) == 0 && value != nullptr) {
        string result(value);
        ::free(value);
        return result;
    }
    return "";
#else
    const char* value = ::getenv(name.data());
    return value ? string(value) : "";
#endif
}

static bool set_unsafe(const string& name, const string& value, const bool overwrite = true) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::_putenv_s(name.data(), value.data()) == 0;
#else
    return ::setenv(name.data(), value.data(), overwrite ? 1 : 0) == 0;
#endif
}

string environment::get(const string& name) {
    shared_lock<shared_mutex> lock(get_mutex());
    return get_unsafe(name);
}

bool environment::set(const string& name, const string& value, const bool overwrite) {
    lock<shared_mutex> lock(get_mutex());
    return set_unsafe(name, value, overwrite);
}

bool environment::unset(const string& name) {
    lock<shared_mutex> lock(get_mutex());
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::SetEnvironmentVariableA(name.data(), nullptr) != 0;
#else
    return ::unsetenv(name.data()) == 0;
#endif
}

bool environment::exists(const string& name) {
    shared_lock<shared_mutex> lock(get_mutex());
    return !get_unsafe(name).empty();
}

unordered_map<string, string> environment::all_envs() {
    shared_lock<shared_mutex> lock(get_mutex());

    unordered_map<string, string> env_map;

#ifdef NEFORCE_PLATFORM_WINDOWS
    char* env_block = ::GetEnvironmentStrings();
    if (env_block == nullptr) {
        return env_map;
    }

    const char* current = env_block;
    while (*current != '\0') {
        string env_str(current);
        size_t eq_pos = env_str.find('=');
        if (eq_pos != string::npos) {
            string name = env_str.substr(0, eq_pos);
            string value = env_str.substr(eq_pos + 1);
            env_map[name] = value;
        }
        current += env_str.length() + 1;
    }
    ::FreeEnvironmentStringsA(env_block);
#else
    for (char** env = ::environ; *env != nullptr; env++) {
        const string_view env_str(*env);
        size_t eq_pos = env_str.find('=');
        if (eq_pos != string::npos) {
            const string name = env_str.substr(0, eq_pos);
            const string value = (env_str.back() == '=') ? "" : env_str.substr(eq_pos + 1);
            env_map[name] = _NEFORCE move(value);
        }
    }
#endif

    return env_map;
}

vector<string> environment::path_list() {
    shared_lock<shared_mutex> lock(get_mutex());
    const string path_str = get_unsafe("PATH");
    vector<string> paths;

    size_t start = 0;
    size_t end = path_str.find(delimiter);

    while (end != string::npos) {
        paths.push_back(path_str.substr(start, end - start));
        start = end + 1;
        end = path_str.find(delimiter, start);
    }

    if (start < path_str.length()) {
        paths.push_back(path_str.substr(start));
    }

    return paths;
}

bool environment::add_to_path(const string& path, const int position) {
    lock<shared_mutex> lock(get_mutex());
    const string original_path = get_unsafe("PATH");

    if (original_path.empty()) {
        return set_unsafe("PATH", path);
    }

    string new_path;
    if (position == 0) {
        new_path = path + delimiter + original_path;
    } else {
        new_path = original_path + delimiter + path;
    }
    const bool success = set_unsafe("PATH", new_path);
    if (!success) {
        set_unsafe("PATH", original_path);
    }
    return success;
}

string environment::current_directory() {
    shared_lock<shared_mutex> lock(get_mutex());
#ifdef NEFORCE_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    const ::DWORD length = ::GetCurrentDirectoryA(MAX_PATH, buffer);
    if (length == 0) {
        throw_exception(system_exception("Failed to get current directory"));
    }
    return string(buffer);
#else
    char* buffer = ::getcwd(nullptr, 0);
    if (buffer == nullptr) {
        throw_exception(system_exception("Failed to get current directory"));
    }
    string result(buffer);
    ::free(buffer);
    return result;
#endif
}

string environment::current_user() {
    shared_lock<shared_mutex> lock(get_mutex());
#ifdef NEFORCE_PLATFORM_WINDOWS
    char username[256];
    ::DWORD size = sizeof(username);
    if (::GetUserNameA(username, &size)) {
        return string(username);
    }
    return "";
#else
    const char* username = ::getenv("USER");
    if (!username) {
        username = ::getenv("USERNAME");
    }
    return username ? string(username) : "";
#endif
}

string environment::temp_directory() {
    shared_lock<shared_mutex> lock(get_mutex());
#ifdef NEFORCE_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    const DWORD length = ::GetTempPathA(MAX_PATH, buffer);
    if (length == 0) {
        return "C:\\Temp";
    }
    return string(buffer);
#else
    const char* tmpdir = ::getenv("TMPDIR");
    if (tmpdir) return tmpdir;

    tmpdir = ::getenv("TEMP");
    if (tmpdir) return tmpdir;

    tmpdir = ::getenv("TMP");
    if (tmpdir) return tmpdir;

    return "/tmp";
#endif
}

string environment::home_directory() {
    shared_lock<shared_mutex> lock(get_mutex());
#ifdef NEFORCE_COMPILER_MSVC
    char* value = nullptr;
    size_t size = 0;
    string result;

    if (::_dupenv_s(&value, &size, "USERPROFILE") == 0 && value != nullptr) {
        result = string(value);
        ::free(value);
        return result;
    }

    char* homedrive = nullptr;
    char* homepath = nullptr;
    size_t homedrive_size = 0, homepath_size = 0;

    const bool has_drive = (::_dupenv_s(&homedrive, &homedrive_size, "HOMEDRIVE") == 0 && homedrive != nullptr);
    const bool has_path = (::_dupenv_s(&homepath, &homepath_size, "HOMEPATH") == 0 && homepath != nullptr);

    if (has_drive && has_path) {
        result = string(homedrive) + string(homepath);
    }

    if (homedrive) ::free(homedrive);
    if (homepath) ::free(homepath);

    return result;
#else
    const char* home = ::getenv("HOME");
    return home ? string(home) : "";
#endif
}

NEFORCE_END_NAMESPACE__
