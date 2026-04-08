#include <NeForce/core/system/cmdline.hpp>
#include <NeForce/core/system/console.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/string/to_string.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <processthreadsapi.h>
#    include <shellapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/file/file.hpp>
#endif
NEFORCE_BEGIN_NAMESPACE__

cmdline::option::option(string lname, const char sname, string desc, const bool req_val, const bool allow_multi,
                        string def_val) :
long_name(move(lname)),
short_name(sname),
description(move(desc)),
requires_value(req_val),
allow_multiple(allow_multi),
default_value(move(def_val)) {}

void cmdline::add_option(const string& long_name, const char short_name, const string& description,
                         const bool requires_value, const bool allow_multiple, const string& default_value) {
    if (long_name.empty() && short_name == 0) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception("Option must have at least one name"));
    }
    if (!long_name.empty() && options_long_.count(long_name) != 0U) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Duplicate long option: " + long_name).data()));
    }
    if (short_name != 0 && options_short_.count(short_name) != 0U) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Duplicate short option: "_s + short_name).data()));
    }

    const option opt(long_name, short_name, description, requires_value, allow_multiple, default_value);
    options_.push_back(move(opt));

    if (!long_name.empty()) {
        options_long_[long_name] = &options_.back();
    }
    if (short_name != 0) {
        options_short_[short_name] = &options_.back();
    }
}

void cmdline::parse_os_args() {
    const auto args = get_os_argv();
    parse(args);
}

void cmdline::parse(const int argc, char* argv[]) {
    vector<string> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    parse(args);
}

void cmdline::parse(const vector<string>& args) {
    if (args.empty()) {
        return;
    }

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
        } else if (arg.starts_with("--")) {
            parse_long_option(arg, args, i);
        } else if (arg.size() > 1 && arg[0] == '-') {
            parse_short_options(arg, args, i);
        } else {
            positional_.push_back(arg);
        }
    }
}

string cmdline::get(const string& long_name, const size_t index) const {
    const auto it = options_long_.find(long_name);
    if (it == options_long_.end() || index >= it->second->values.size()) {
        if (!it->second->default_value.empty()) {
            return it->second->default_value;
        }
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Option not found or no value: " + long_name).data()));
    }
    return it->second->values[index];
}

bool cmdline::has(const string& name) const {
    const auto it = options_long_.find(name);
    if (it == options_long_.end()) {
        return false;
    }
    return !it->second->values.empty();
}

size_t cmdline::count(const string& name) const {
    const auto it = options_long_.find(name);
    if (it == options_long_.end()) {
        return 0;
    }
    return it->second->values.size();
}

void cmdline::print_help() const {
    println("Usage: ", program_name_, " [options] [positional...]\n\nOptions:");

    for (const auto& opt: options_) {
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

        string opt_str = move(str);
        print(opt_str);

        if (opt_str.length() < 30) {
            print(string(30 - opt_str.length(), ' '));
        } else {
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

vector<string> cmdline::get_os_argv() {
    vector<string> args;

#ifdef NEFORCE_PLATFORM_WINDOWS
    int argc = 0;
    ::LPWSTR* argv_wide = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv_wide == nullptr) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception("CommandLineToArgvW failed"));
    }
    for (int i = 0; i < argc; ++i) {
        args.push_back(_NEFORCE to_string(argv_wide[i]));
    }
    ::LocalFree(static_cast<void*>(argv_wide));
#else
    file cmdline_file(path{"/proc/self/cmdline"});
    if (!cmdline_file.is_opened()) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception("Failed to open /proc/self/cmdline"));
    }

    string buffer(MEMORY_BIG_ALLOC_THRESHHOLD, '\0');
    const size_t bytes_read = cmdline_file.read_binary(buffer, buffer.size());
    if (bytes_read == 0) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception("No data read from /proc/self/cmdline"));
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

cmdline::option* cmdline::find_option_short(const char name) {
    const auto it = options_short_.find(name);
    return it == options_short_.end() ? nullptr : it->second;
}

void cmdline::parse_long_option(const string& arg, const vector<string>& args, size_t& index) {
    const size_t eq_pos = arg.find('=');
    const string name = arg.substr(2, eq_pos == string::npos ? string::npos : eq_pos - 2);

    option* opt = find_option_long(name);
    if (opt == nullptr) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Unknown option: " + arg).data()));
    }

    if (opt->requires_value) {
        string value;
        if (eq_pos != string::npos) {
            value = arg.substr(eq_pos + 1);
        } else {
            if (index + 1 >= args.size()) {
                NEFORCE_THROW_EXCEPTION(cmdline_exception(("Option requires a value: --" + name).data()));
            }
            value = args[++index];
        }

        if (opt->allow_multiple) {
            opt->values.push_back(move(value));
        } else {
            opt->values.clear();
            opt->values.push_back(move(value));
        }
    } else {
        if (opt->allow_multiple) {
            opt->values.push_back("1");
        } else {
            opt->values.clear();
            opt->values.push_back("1");
        }
    }
}

void cmdline::parse_short_options(const string& arg, const vector<string>& args, size_t& index) {
    for (size_t j = 1; j < arg.size(); ++j) {
        const char short_name = arg[j];
        option* opt = find_option_short(short_name);

        if (opt == nullptr) {
            NEFORCE_THROW_EXCEPTION(cmdline_exception(("Unknown short option: -"_s + short_name).data()));
        }

        if (opt->requires_value) {
            if (j == arg.size() - 1) {
                if (index + 1 >= args.size()) {
                    NEFORCE_THROW_EXCEPTION(cmdline_exception(("Option requires a value: -"_s + short_name).data()));
                }
                string value = args[++index];
                if (opt->allow_multiple) {
                    opt->values.push_back(move(value));
                } else {
                    opt->values.clear();
                    opt->values.push_back(move(value));
                }
            } else {
                const string value = arg.substr(j + 1);
                if (opt->allow_multiple) {
                    opt->values.push_back(move(value));
                } else {
                    opt->values.clear();
                    opt->values.push_back(move(value));
                }
                break;
            }
        } else {
            if (opt->allow_multiple) {
                opt->values.push_back("1");
            } else {
                opt->values.clear();
                opt->values.push_back("1");
            }
        }
    }
}

NEFORCE_END_NAMESPACE__
