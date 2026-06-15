#include <NeForce/core/system/cmdline.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/file/env/env_parser.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/ini/ini_parser.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/file/toml/toml_parser.hpp>
#include <NeForce/core/file/yaml/yaml_parser.hpp>
#include <NeForce/core/string/regex.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/string/to_string.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <processthreadsapi.h>
#    include <shellapi.h>
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
    string actual_long_name = long_name;
    if (actual_long_name.empty() && short_name != 0) {
        actual_long_name = string(1, short_name);
    }

    if (actual_long_name.empty() && short_name == 0) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception("Option must have at least one name"));
    }
    if (!actual_long_name.empty() && options_long_.count(actual_long_name) != 0U) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Duplicate long option: " + actual_long_name).data()));
    }
    if (short_name != 0 && options_short_.count(short_name) != 0U) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Duplicate short option: "_s + short_name).data()));
    }

    const option opt(actual_long_name, short_name, description, requires_value, allow_multiple, default_value);
    options_.push_back(move(opt));
    const size_t index = options_.size() - 1;

    options_long_[actual_long_name] = index;
    if (short_name != 0) {
        options_short_[short_name] = index;
    }
}

void cmdline::parse_os_args() {
    const auto args = get_os_argv();
    parse(args);
}

void cmdline::parse(const int argc, const char* argv[]) {
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

    validate_values();
    validate_constraints();
}

string cmdline::get(const string& long_name, const size_t index) const {
    const auto it = options_long_.find(long_name);

    if (it == options_long_.end()) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Option not found: " + long_name).data()));
    }

    const option* opt = &options_[it->second];

    if (index >= opt->values.size()) {
        if (!opt->default_value.empty()) {
            return opt->default_value;
        }
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("No value for option: " + long_name).data()));
    }

    return opt->values[index];
}

bool cmdline::has(const string& name) const {
    const auto it = options_long_.find(name);
    if (it == options_long_.end()) {
        return false;
    }
    return !options_[it->second].values.empty();
}

size_t cmdline::count(const string& name) const {
    const auto it = options_long_.find(name);
    if (it == options_long_.end()) {
        return 0;
    }
    return options_[it->second].values.size();
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
        args.push_back(to_string(argv_wide[i]));
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
    if (it == options_long_.end()) {
        return nullptr;
    }
    return &options_[it->second];
}

cmdline::option* cmdline::find_option_short(const char name) {
    const auto it = options_short_.find(name);
    if (it == options_short_.end()) {
        return nullptr;
    }
    return &options_[it->second];
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
            value = arg.tail(eq_pos + 1);
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

void cmdline::add_dependency(const string& present, const string& required) {
    dependencies_.push_back({present, required});
}

void cmdline::add_conflict(const string& option_a, const string& option_b) {
    conflicts_.push_back({option_a, option_b});
}

void cmdline::validate_values() {
    for (const auto& opt: options_) {
        if (opt.values.empty()) {
            continue;
        }
        for (const auto& v: validators_) {
            for (const auto& val: opt.values) {
                const string error = v(opt.long_name, val);
                if (!error.empty()) {
                    NEFORCE_THROW_EXCEPTION(cmdline_exception(error.data()));
                }
            }
        }
    }
}

void cmdline::validate_constraints() {
    for (const auto& dep: dependencies_) {
        if (has(dep.present) && !has(dep.required)) {
            const string msg = "Option '" + dep.present + "' requires option '" + dep.required + "'";
            switch (conflict_behavior_) {
                case conflict_behavior::error:
                    NEFORCE_THROW_EXCEPTION(cmdline_exception(msg.data()));
                case conflict_behavior::warning:
                    println("[WARNING] ", msg);
                    break;
                case conflict_behavior::ignore:
                    break;
            }
        }
    }

    for (const auto& conf: conflicts_) {
        if (has(conf.option_a) && has(conf.option_b)) {
            const string msg = "Conflicting options: '" + conf.option_a + "' and '" + conf.option_b + "'";
            switch (conflict_behavior_) {
                case conflict_behavior::error:
                    NEFORCE_THROW_EXCEPTION(cmdline_exception(msg.data()));
                case conflict_behavior::warning:
                    println("[WARNING] ", msg);
                    break;
                case conflict_behavior::ignore:
                    break;
            }
        }
    }
}

void cmdline::load_config(const string& config_path, const string& section) {
    file config_file(path{config_path});
    if (!config_file.is_opened()) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Cannot open config file: " + config_path).data()));
    }
    const string content = config_file.read();

    unordered_map<string, string> config_values;
    const string ext = path{config_path}.extension();

    if (ext == ".ini") {
        ini_parser parser(content);
        auto doc = parser.parse();
        const ini_section* sec = section.empty() ? doc->get_global_section() : doc->get_section(section);
        if (sec != nullptr) {
            for (const auto& entry: sec->get_properties()) {
                config_values[entry.first] = entry.second->get_value();
            }
        }
    } else if (ext == ".json") {
        json_parser parser(content);
        auto root = parser.parse();
        const json_value* current = root.get();
        if (!section.empty()) {
            const auto* root_obj = dynamic_cast<const json_object*>(current);
            if (root_obj != nullptr) {
                for (const auto& member: root_obj->get_members()) {
                    if (member.first == section) {
                        current = member.second.get();
                        break;
                    }
                }
            }
        }
        const auto* obj = dynamic_cast<const json_object*>(current);
        if (obj != nullptr) {
            for (const auto& member: obj->get_members()) {
                const string val_str = member.second->to_string();
                if (val_str.size() >= 2 && val_str.front() == '"' && val_str.back() == '"') {
                    config_values[member.first] = val_str.substr(1, val_str.size() - 2);
                } else {
                    config_values[member.first] = val_str;
                }
            }
        }
    } else if (ext == ".toml") {
        toml_parser parser(content);
        auto root = parser.parse();
        const toml_value* current = root.get();
        if (!section.empty()) {
            const auto* root_tbl = dynamic_cast<const toml_table*>(current);
            if (root_tbl != nullptr) {
                for (const auto& member: root_tbl->get_members()) {
                    if (member.first == section) {
                        current = member.second.get();
                        break;
                    }
                }
            }
        }
        const auto* tbl = dynamic_cast<const toml_table*>(current);
        if (tbl != nullptr) {
            for (const auto& member: tbl->get_members()) {
                const string val_str = member.second->to_string();
                if (val_str.size() >= 2 && val_str.front() == '"' && val_str.back() == '"') {
                    config_values[member.first] = val_str.substr(1, val_str.size() - 2);
                } else {
                    config_values[member.first] = val_str;
                }
            }
        }
    } else if (ext == ".yaml" || ext == ".yml") {
        yaml_parser parser(content);
        auto root = parser.parse();
        const yaml_value* current = root.get();
        if (!section.empty()) {
            const auto* root_map = dynamic_cast<const yaml_mapping*>(current);
            if (root_map != nullptr) {
                for (const auto& member: root_map->get_members()) {
                    if (member.first == section) {
                        current = member.second.get();
                        break;
                    }
                }
            }
        }
        const auto* map_val = dynamic_cast<const yaml_mapping*>(current);
        if (map_val != nullptr) {
            for (const auto& member: map_val->get_members()) {
                const string val_str = member.second->to_string();
                if (val_str.size() >= 2 && val_str.front() == '"' && val_str.back() == '"') {
                    config_values[member.first] = val_str.substr(1, val_str.size() - 2);
                } else {
                    config_values[member.first] = val_str;
                }
            }
        }
    } else if (ext == ".env") {
        env_parser parser(content);
        auto doc = parser.parse();
        for (const auto& entry: doc->get_variables()) {
            config_values[entry.first] = entry.second->get_value();
        }
    } else {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Unsupported config file format: " + ext).data()));
    }

    for (auto& opt: options_) {
        if (opt.values.empty()) {
            const auto it = config_values.find(opt.long_name);
            if (it != config_values.end()) {
                opt.values.push_back(it->second);
            }
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
                const string value = arg.tail(j + 1);
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

void cmdline::add_validator(validator v) { validators_.push_back(move(v)); }

void cmdline::add_range_validator(string long_name, const int64_t min_val, const int64_t max_val) {
    if (options_long_.find(long_name) == options_long_.end()) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Option not found: " + long_name).data()));
    }
    validators_.push_back(
            [long_name = move(long_name), min_val, max_val](const string& name, const string& value) -> string {
                try {
                    if (name != long_name) {
                        return "";
                    }
                    const int64_t num = to_int64(value.view());
                    if (num < min_val || num > max_val) {
                        return "Value for --" + long_name + " must be in range [" + to_string(min_val) + ", " +
                               to_string(max_val) + "]";
                    }
                    return "";
                } catch (...) {
                    return "Value for --" + long_name + " is not a valid integer";
                }
            });
}

void cmdline::add_regex_validator(string long_name, const string& pattern) {
    if (options_long_.find(long_name) == options_long_.end()) {
        NEFORCE_THROW_EXCEPTION(cmdline_exception(("Option not found: " + long_name).data()));
    }
    validators_.push_back(
            [long_name = move(long_name), re = regex(pattern)](const string& name, const string& value) -> string {
                try {
                    if (name != long_name) {
                        return "";
                    }
                    if (!re.match(value)) {
                        return "Value for --" + long_name + " does not match required pattern";
                    }
                    return "";
                } catch (...) {
                    return "Value for --" + long_name + " does not match required pattern";
                }
            });
}

NEFORCE_END_NAMESPACE__
