#ifndef MSTL_CORE_SYSTEM_ENV_VARIABLE_HPP__
#define MSTL_CORE_SYSTEM_ENV_VARIABLE_HPP__
#include "../container/unordered_map.hpp"
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API environment {
private:
    static string get_unsafe(const string& name);
    static bool set_unsafe(const string& name, const string& value, bool overwrite = true);

public:
#ifdef MSTL_PLATFORM_WINDOWS__
    static constexpr char delimiter = ';';
#else
    static constexpr char delimiter = ':';
#endif

    static string get(const string& name);
    static bool set(const string& name, const string& value, bool overwrite = true);
    static bool unset(const string& name);

    static bool exists(const string& name);

    static unordered_map<string, string> all_envs();

    static vector<string> path_list();
    static bool add_to_path(const string& path, int position = 1);

    static string current_directory();
    static string current_user();

    static string temp_directory();
    static string home_directory();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_ENV_VARIABLE_HPP__
