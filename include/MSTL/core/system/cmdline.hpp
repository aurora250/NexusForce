#ifndef MSTL_CORE_SYSTEM_CMDLINE_HPP__
#define MSTL_CORE_SYSTEM_CMDLINE_HPP__
#include "../container/unordered_map.hpp"
#include "../container/vector.hpp"
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

struct cmdline_exception final : value_exception {
    explicit cmdline_exception(
        const char* info = "CmdLine Operation Failed.",
        const char* type = __type__) noexcept : value_exception(info, type) {}

    explicit cmdline_exception(
            const string& info = "CmdLine Operation Failed.",
            const char* type = __type__) noexcept
    : value_exception(type, type), msg(info) {
        this->info = msg.data();
    }

    ~cmdline_exception() override = default;

    static constexpr auto __type__ = "cmdline_exception";
    string msg;
};


class MSTL_API cmdline {
public:
    struct option {
        string long_name;
        char short_name = 0;
        string description;
        bool requires_value = false;
        bool allow_multiple = false;
        string default_value;

        _MSTL vector<string> values;

        option() = default;
        option(string lname, char sname, string desc,
            bool req_val, bool allow_multi, string def_val);
    };

    void add_option(const string& long_name, char short_name,
        const string& description, bool requires_value = false,
        bool allow_multiple = false, const string& default_value = "");

    void parse_os_args();
    void parse(int argc, char* argv[]);
    void parse(const _MSTL vector<string>& args);

    bool has(const string& long_name) const;
    size_t count(const string& long_name) const;

    const _MSTL vector<string>& positional_args() const { return positional_; }
    string program_name() const { return program_name_; }

    void print_help() const;

    static _MSTL vector<string> get_os_argv();

private:
    string program_name_;
    _MSTL vector<option> options_;
    _MSTL unordered_map<string, option*> options_long_;
    _MSTL unordered_map<char, option*> options_short_;
    _MSTL vector<string> positional_;

    option* find_option_long(const string& name);
    option* find_option_short(char name);

    void parse_long_option(const string& arg, const _MSTL vector<string>& args, size_t& i);
    void parse_short_options(const string& arg, const _MSTL vector<string>& args, size_t& i);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_CMDLINE_HPP__
