#include <MSTL/core/system/cmdline.hpp>
#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/string/to_string.hpp>
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <MSTL/core/file/file.hpp>
#endif
MSTL_BEGIN_NAMESPACE__

cmdline::option::option(string lname, const char sname, string desc,
    const bool req_val, const bool allow_multi, string def_val)
    : long_name(_MSTL move(lname)),
    short_name(sname),
    description(_MSTL move(desc)),
    requires_value(req_val),
    allow_multiple(allow_multi),
    default_value(_MSTL move(def_val)) {}

void cmdline::add_option(const string& long_name, const char short_name,
    const string& description, const bool requires_value,
    const bool allow_multiple, const string& default_value) {
    if (long_name.empty() && short_name == 0) {
        throw_exception(cmdline_exception("Option must have at least one name"));
    }
    if (!long_name.empty() && options_long_.count(long_name)) {
        throw_exception(cmdline_exception("Duplicate long option: " + long_name));
    }
    if (short_name != 0 && options_short_.count(short_name)) {
        throw_exception(cmdline_exception(string("Duplicate short option: ") + short_name));
    }

    const option opt(long_name, short_name, description, requires_value, allow_multiple, default_value);
    options_.push_back(_MSTL move(opt));

    if (!long_name.empty()) options_long_[long_name] = &options_.back();
    if (short_name != 0) options_short_[short_name] = &options_.back();
}

void cmdline::parse_os_args() {
    const auto args = get_os_argv();
    parse(args);
}

void cmdline::parse(const int argc, char* argv[]) {
    _MSTL vector<string> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    parse(args);
}

void cmdline::parse(const _MSTL vector<string>& args) {
    if (args.empty()) return;

    program_name_ = args[0];
    bool end_of_options = false;

    for (size_t i = 1; i < args.size(); ++i) {
        const string& arg = args[i];

        if (end_of_options) {
            positional_.push_back(arg);
            continue;
        }

        if (arg == "--") {
            end_of_options = true;
        }
        else if (arg.compare(0, 2, "--") == 0) {
            parse_long_option(arg, args, i);
        }
        else if (arg.size() > 1 && arg[0] == '-') {
            parse_short_options(arg, args, i);
        }
        else {
            positional_.push_back(arg);
        }
    }
}

bool cmdline::has(const string& long_name) const {
    const auto it = options_long_.find(long_name);
    if (it == options_long_.end()) return false;
    return !it->second->values.empty();
}

size_t cmdline::count(const string& long_name) const {
    const auto it = options_long_.find(long_name);
    if (it == options_long_.end()) return 0;
    return it->second->values.size();
}

void cmdline::print_help() const {
    _MSTL println("Usage: ", program_name_, " [options] [positional...]\n\nOptions:");

    for (const auto& opt : options_) {
        string str;

        if (opt.short_name != 0) {
            str += "  -"_s + opt.short_name;
            if (!opt.long_name.empty()) {
                str += ", --" + opt.long_name;
            }
        } else {
            str += "      --" + opt.long_name;
        }
        if (opt.requires_value) {
            str += " <value>";
        }

        string opt_str = _MSTL move(str);
        print(opt_str);

        if (opt_str.length() < 30) {
            print(string(30 - opt_str.length(), ' '));
        }
        else {
            print("\n", string(30, ' '));
        }

        print(opt.description);

        if (!opt.default_value.empty()) {
            print(" (default: ", opt.default_value, ")");
        }
        if (opt.allow_multiple) {
            print(" [can be repeated]");
        }
        println();
    }
}

_MSTL vector<string> cmdline::get_os_argv() {
    _MSTL vector<string> args;

#ifdef MSTL_PLATFORM_WINDOWS__
    int argc = 0;
    ::LPWSTR* argv_wide = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (!argv_wide) {
        throw_exception(cmdline_exception("CommandLineToArgvW failed"));
    }
    for (int i = 0; i < argc; ++i) {
        args.push_back(_MSTL to_string(argv_wide[i]));
    }
    ::LocalFree(argv_wide);
#else
    file cmdline_file(path{"/proc/self/cmdline"});
    if (!cmdline_file.opened()) {
        throw_exception(cmdline_exception("Failed to open /proc/self/cmdline"));
    }

    string buffer(4096, '\0');
    const size_t bytes_read = cmdline_file.read_binary(buffer, buffer.size());
    if (bytes_read == 0) {
        throw_exception(cmdline_exception("No data read from /proc/self/cmdline"));
    }

    size_t start = 0;
    for (size_t i = 0; i < bytes_read; ++i) {
        if (buffer[i] == '\0') {
            if (i > start) {
                args.emplace_back(&buffer[start], i - start);
            }
            start = i + 1;
        }
    }
    if (start < bytes_read) {
        args.emplace_back(&buffer[start], bytes_read - start);
    }
#endif
    return args;
}

cmdline::option* cmdline::find_option_long(const string& name) {
    const auto it = options_long_.find(name);
    return it == options_long_.end() ? nullptr : it->second;
}

cmdline::option* cmdline::find_option_short(char name) {
    const auto it = options_short_.find(name);
    return it == options_short_.end() ? nullptr : it->second;
}

void cmdline::parse_long_option(const string& arg, const _MSTL vector<string>& args, size_t& i) {
    const size_t eq_pos = arg.find('=');
    const string name = arg.substr(2, eq_pos == string::npos ? string::npos : eq_pos - 2);

    option* opt = find_option_long(name);
    if (!opt) {
        throw_exception(cmdline_exception("Unknown option: " + arg));
    }

    if (opt->requires_value) {
        string value;
        if (eq_pos != string::npos) {
            value = arg.substr(eq_pos + 1);
        }
        else {
            if (i + 1 >= args.size()) {
                throw_exception(cmdline_exception("Option requires a value: --" + name));
            }
            value = args[++i];
        }

        if (opt->allow_multiple) {
            opt->values.push_back(_MSTL move(value));
        }
        else {
            opt->values.clear();
            opt->values.push_back(_MSTL move(value));
        }
    }
    else {
        if (opt->allow_multiple) {
            opt->values.push_back("1");
        }
        else {
            opt->values.clear();
            opt->values.push_back("1");
        }
    }
}

void cmdline::parse_short_options(const string& arg, const _MSTL vector<string>& args, size_t& i) {
    for (size_t j = 1; j < arg.size(); ++j) {
        const char short_name = arg[j];
        option* opt = find_option_short(short_name);

        if (!opt) {
            throw_exception(cmdline_exception(string("Unknown short option: -") + short_name));
        }

        if (opt->requires_value) {
            if (j == arg.size() - 1) {
                if (i + 1 >= args.size()) {
                    throw_exception(cmdline_exception(string("Option requires a value: -") + short_name));
                }
                const string value = args[++i];
                if (opt->allow_multiple) {
                    opt->values.push_back(_MSTL move(value));
                }
                else {
                    opt->values.clear();
                    opt->values.push_back(_MSTL move(value));
                }
            }
            else {
                const string value = arg.substr(j + 1);
                if (opt->allow_multiple) {
                    opt->values.push_back(_MSTL move(value));
                }
                else {
                    opt->values.clear();
                    opt->values.push_back(_MSTL move(value));
                }
                break;
            }
        }
        else {
            if (opt->allow_multiple) {
                opt->values.push_back("1");
            }
            else {
                opt->values.clear();
                opt->values.push_back("1");
            }
        }
    }
}

MSTL_END_NAMESPACE__
