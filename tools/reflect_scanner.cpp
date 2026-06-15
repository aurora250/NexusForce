/**
 * @file reflect_scanner.cpp
 * @brief NeForce Reflection Scanner (MOC-like pre-compile code generator)
 *
 * Scans C++ headers for NEFORCE_REFLECT_OBJ markers and generates
 * _neforce_reflect_gen.cpp containing complete type registration code.
 *
 * Usage:
 *   reflect_scanner <input_dir> -o <output_file>
 *
 *   <input_dir>  Root directory to scan for .hpp files
 *   -o           Output path for generated C++ registration file
 */

#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/path_tree.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/system/cmdline.hpp>
#include <NeForce/core/system/console.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    struct prop_info {
        string type;
        string name;
        string attrs;
    };

    struct func_info {
        string ret;
        string name;
        string args;
        vector<string> arg_types;
    };

    struct base_info {
        string access;
        string name;
    };

    struct enum_info {
        string name;
        string underlying;
        vector<string> values;
    };

    struct signal_info {
        string name;
        string signature;
    };

    struct class_info {
        string name;
        vector<base_info> bases;
        vector<prop_info> props;
        vector<func_info> funcs;
        vector<signal_info> signals;
    };

    size_t find_matching_brace(const string& text, const size_t open_pos) {
        if (open_pos >= text.size() || text[open_pos] != '{') {
            return string::npos;
        }
        int depth = 1;
        size_t i = open_pos + 1;
        while (i < text.size() && depth > 0) {
            if (text[i] == '{') {
                ++depth;
            } else if (text[i] == '}') {
                --depth;
            }
            ++i;
        }
        return depth == 0 ? i - 1 : string::npos;
    }

    vector<base_info> parse_bases(const string& base_clause) {
        vector<base_info> bases;
        if (base_clause.empty()) {
            return bases;
        }

        const string& trimmed = base_clause;

        size_t pos = 0;
        while (pos < trimmed.size()) {
            const size_t next = trimmed.find(',', pos);
            string part = trimmed.substr(pos, next == string::npos ? string::npos : next - pos);
            pos = next == string::npos ? trimmed.size() : next + 1;

            part.trim();

            if (part.empty()) {
                continue;
            }

            const auto space_pos = part.find(' ');
            if (space_pos != string::npos) {
                base_info bi;
                bi.access = part.substr(0, space_pos);
                bi.name = part.substr(space_pos + 1);
                bases.push_back(bi);
            } else {
                base_info bi;
                bi.access = "public";
                bi.name = part;
                bases.push_back(bi);
            }
        }

        return bases;
    }

    vector<class_info> extract_classes(const string& content) {
        vector<class_info> classes;

        regex class_re(R"((?:class|struct)\s+(\w+)\s*(?::\s*([^{]+?))?\s*\{)");
        regex obj_marker(R"(NEFORCE_REFLECT_OBJ\s*\(\s*(\w+)\s*\))");
        regex prop_marker(R"(NEFORCE_REFLECT_PROP\s*\(\s*([\w:<>]+)\s*,\s*(\w+)\s*\))");
        regex prop_attr_marker(R"(NEFORCE_REFLECT_PROP_ATTR\s*\(\s*([\w:<>]+)\s*,\s*(\w+)\s*,\s*([^)]+)\s*\))");
        regex func_marker(R"(NEFORCE_REFLECT_FUNC\s*\(\s*([\w:<>]+)\s*,\s*(\w+)\s*(?:,\s*(.+))?\s*\))");

        auto class_matches = class_re.find_all(content);
        for (const auto& cm: class_matches) {
            size_t class_start = cm.position();

            size_t pattern_len = cm.length();
            size_t brace_start = class_start + pattern_len - 1;

            size_t brace_end = find_matching_brace(content, brace_start);
            if (brace_end == string::npos) {
                continue;
            }

            string class_body = content.substr(class_start, brace_end - class_start + 1);

            auto obj_match = obj_marker.search(class_body);
            if (!obj_match.matched()) {
                continue;
            }

            class_info info;
            info.name = string(cm[1]);

            auto base_clause = string(cm[2]);
            info.bases = parse_bases(base_clause);

            for (const auto& pm: prop_marker.find_all(class_body)) {
                prop_info pi;
                pi.type = string(pm[1]);
                pi.name = string(pm[2]);
                info.props.push_back(pi);
            }

            for (const auto& pm: prop_attr_marker.find_all(class_body)) {
                prop_info pi;
                pi.type = string(pm[1]);
                pi.name = string(pm[2]);
                pi.attrs = string(pm[3]);
                info.props.push_back(pi);
            }

            for (const auto& fm: func_marker.find_all(class_body)) {
                func_info fi;
                fi.ret = string(fm[1]);
                fi.name = string(fm[2]);
                fi.args = string(fm[3]);

                if (!fi.args.empty()) {
                    size_t ap = 0;
                    while (ap < fi.args.size()) {
                        const size_t comma = fi.args.find(',', ap);
                        string arg = fi.args.substr(ap, comma == string::npos ? string::npos : comma - ap);
                        arg.trim();
                        if (!arg.empty()) {
                            fi.arg_types.push_back(arg);
                        }
                        ap = comma == string::npos ? fi.args.size() : comma + 1;
                    }
                }
                info.funcs.push_back(fi);
            }

            regex signal_marker(R"(NEFORCE_REFLECT_SIGNAL\s*\(\s*([\w:<>]+)\s*,\s*(\w+)\s*(?:,\s*(.+))?\s*\))");
            for (const auto& sm: signal_marker.find_all(class_body)) {
                signal_info si;
                si.name = string(sm[2]);
                si.signature = string(sm[1]);
                {
                    const string arg_str{sm[3]};
                    if (!arg_str.empty()) {
                        si.signature += "," + arg_str;
                    }
                }
                info.signals.push_back(si);
            }

            classes.push_back(info);
        }

        return classes;
    }

    vector<enum_info> extract_enums(const string& content) {
        vector<enum_info> enums;
        unordered_map<string, size_t> name_index;

        const regex enum_marker(R"(NEFORCE_REFLECT_ENUM\s*\(\s*(\w+)\s*,\s*(\w+)\s*\))");
        const regex eval_marker(R"(NEFORCE_REFLECT_ENUM_VAL\s*\(\s*(\w+)\s*,\s*(\w+)\s*\))");

        for (const auto& em: enum_marker.find_all(content)) {
            enum_info ei;
            ei.name = string(em[1]);
            ei.underlying = string(em[2]);
            name_index[ei.name] = enums.size();
            enums.push_back(ei);
        }

        for (const auto& vm: eval_marker.find_all(content)) {
            auto enum_name = string(vm[1]);
            auto it = name_index.find(enum_name);
            if (it != name_index.end()) {
                enums[it->second].values.push_back(string(vm[2]));
            }
        }

        return enums;
    }

    string resolve_attr(const string& raw_attrs) {
        string result;
        size_t pos = 0;
        while (pos < raw_attrs.size()) {
            const size_t pipe = raw_attrs.find('|', pos);
            string part = raw_attrs.substr(pos, pipe == string::npos ? string::npos : pipe - pos);
            part.trim();
            if (!part.empty()) {
                if (!result.empty()) {
                    result += " | ";
                }
                result += "neforce::reflect::" + part;
            }
            pos = pipe == string::npos ? raw_attrs.size() : pipe + 1;
        }
        return result;
    }

    string generate_code(const vector<string>& headers, const vector<class_info>& classes,
                         const vector<enum_info>& enums) {
        string out;
        out += "// AUTO-GENERATED by reflect_scanner — DO NOT EDIT\n";
        out += format("// Generated from {} header(s)\n", headers.size());
        out += "\n";
        out += "#include <NeForce/core/reflect/reflect.hpp>\n";
        for (const auto& hdr: headers) {
            out += format("#include \"{}\"\n", hdr);
        }
        out += "\n";

        for (const auto& en: enums) {
            out += format("// === Enum registration: {} ===\n", en.name);
            out += "namespace {\n";
            out += format("static auto _neforce_enum_{} = []() {{\n", en.name);
            out += format(
                    "    auto& _neforce_meta = neforce::reflect::registry::instance().register_type<{}>(\"{}\");\n",
                    en.name, en.name);
            out += format("    auto ei = neforce::make_unique<neforce::reflect::meta_enum>(\"{}\", "
                          "neforce::reflect::type_id_for<{}>());\n",
                          en.name, en.underlying);
            for (const auto& val: en.values) {
                out += format("    ei->add_entry(\"{}\", static_cast<int64_t>({}::{}));\n", val, en.name, val);
            }
            out += "    _neforce_meta.enum_info(neforce::move(ei));\n";
            out += "    return 0;\n";
            out += "}();\n";
            out += "}  // anonymous namespace\n";
            out += "\n";
        }

        for (const auto& cls: classes) {
            out += format("// === Registration for class: {} ===\n", cls.name);
            out += "namespace {\n";
            out += format("static auto _neforce_reg_{} = []() {{\n", cls.name);
            out += format("    auto builder = neforce::reflect::reflect<{}>(\"{}\");\n", cls.name, cls.name);

            for (const auto& base: cls.bases) {
                out += format("    builder.base(\"{}\");\n", base.name);
            }

            for (const auto& prop: cls.props) {
                if (!prop.attrs.empty()) {
                    out += format("    builder.property(\"{}\", &{}::{}, {});\n", prop.name, cls.name, prop.name,
                                  resolve_attr(prop.attrs));
                } else {
                    out += format("    builder.property(\"{}\", &{}::{});\n", prop.name, cls.name, prop.name);
                }
            }

            for (const auto& func: cls.funcs) {
                out += format("    builder.function(\"{}\", &{}::{});\n", func.name, cls.name, func.name);
            }

            for (const auto& sig: cls.signals) {
                out += format("    builder.signal(\"{}\");\n", sig.name);
            }

            out += "    builder.constructor();\n";
            out += "    return builder;\n";
            out += "}();\n";
            out += "}  // anonymous namespace\n";
            out += "\n";
        }

        if (!classes.empty() || !enums.empty()) {
            out += "static auto _neforce_resolve_bases = []() {\n";
            out += "    NEFORCE_REFLECT_RESOLVE_BASES();\n";
            out += "    return true;\n";
            out += "}();\n";
            out += "\n";
        }
        return out;
    }

    void print_usage(const cmdline& cmd) {
        eprint("Usage: reflect_scanner <input_dir> -o <output_file> [--exclude <path> ...]\n");
        eprint("\n");
        eprint("Scans C++ headers for NEFORCE_REFLECT_OBJ markers and generates\n");
        eprint("a complete type registration file.\n");
    }
} // namespace

NEFORCE_END_NAMESPACE__


int main(int argc, const char* argv[]) {
    using namespace neforce;

    cmdline cmd;
    cmd.add_option("output", 'o', "Output file path for generated code", true, false, "");
    cmd.add_option("exclude", 'e', "Paths to exclude from scanning", true, true, "");
    cmd.add_option("help", 'h', "Print help information", false, false, "");

    try {
        cmd.parse(argc, argv);
    } catch (const exception& e) {
        eprintfln("Error: {}", e.what());
        return 1;
    }

    if (cmd.has("help")) {
        print_usage(cmd);
        return 0;
    }

    if (!cmd.has("output")) {
        eprintln("Error: -o <output_file> is required");
        return 1;
    }

    const auto& positional = cmd.positional_args();
    if (positional.empty()) {
        eprintln("Error: <input_dir> is required");
        return 1;
    }

    string input_dir = positional[0];
    string output_path = cmd.get("output");

    vector<string> excludes;
    if (cmd.has("exclude")) {
        const size_t count = cmd.count("exclude");
        for (size_t i = 0; i < count; ++i) {
            excludes.push_back(cmd.get("exclude", i));
        }
    }

    eprintfln("Scanning: {}", input_dir);

    vector<string> headers;
    try {
        path_tree::scan_options opts;
        opts.extensions = {"hpp", "h"};
        opts.files_only = true;
        opts.include_hidden = false;

        const auto tree = path_tree::scan(path(input_dir), opts);
        tree.traverse_files([&](const path_tree::node& node) {
            const string file_path = node.get_path().to_string();

            bool excluded = false;
            for (const auto& ex: excludes) {
                if (file_path.find(ex) != string::npos) {
                    excluded = true;
                    break;
                }
            }

            if (!excluded) {
                headers.push_back(file_path);
            }
            return path_tree::visit_result::proceed;
        });
    } catch (const exception& e) {
        eprintfln("Error scanning directory: {}", e.what());
        return 1;
    }

    eprintfln("Found {} header file(s)", headers.size());

    string cache_path = output_path + ".cache";
    unordered_map<string, string> cache;
    try {
        file cache_file{path{cache_path}};
        string cache_content = cache_file.read();
        size_t pos = 0;
        while (pos < cache_content.size()) {
            const size_t nl = cache_content.find('\n', pos);
            string line = cache_content.substr(pos, nl == string::npos ? string::npos : nl - pos);
            pos = nl == string::npos ? cache_content.size() : nl + 1;
            line.trim();
            if (line.empty()) {
                continue;
            }
            const size_t sep = line.find('|');
            if (sep != string::npos) {
                cache[line.substr(sep + 1)] = line.substr(0, sep);
            }
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }

    bool output_exists = false;
    try {
        file test{path{output_path}};
        output_exists = true;
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }

    bool need_regenerate = !output_exists || headers.empty();
    if (!need_regenerate && headers.size() != cache.size()) {
        need_regenerate = true;
    }
    if (!need_regenerate) {
        for (const auto& hdr: headers) {
            try {
                file f{path{hdr}};
                const string ts = f.info().last_write_time().to_string();
                const auto it = cache.find(hdr);
                if (it == cache.end() || it->second != ts) {
                    need_regenerate = true;
                    break;
                }
            } catch (...) {
                need_regenerate = true;
                break;
            }
        }
    }

    if (!need_regenerate) {
        eprintfln("All {} header(s) unchanged, skipping generation.", headers.size());
        return 0;
    }

    unordered_map<string, string> new_cache;
    vector<class_info> all_classes;
    vector<enum_info> all_enums;
    for (const auto& hdr: headers) {
        try {
            file f{path{hdr}};
            new_cache[hdr] = f.info().last_write_time().to_string();
            string content = f.read();
            auto classes = extract_classes(content);
            for (auto& cls: classes) {
                all_classes.push_back(move(cls));
            }
            auto enums = extract_enums(content);
            for (auto& en: enums) {
                all_enums.push_back(move(en));
            }
        } catch (const exception& e) {
            eprintfln("Warning: Could not read {}: {}", hdr, e.what());
        }
    }

    string generated = generate_code(headers, all_classes, all_enums);

    try {
        file out_file(path(output_path), false, file_access::WRITE, file_shared::SHARE_READ,
                      file_creation::CREATE_FORCE, file_attri::NORMAL);
        out_file.write(generated.data(), static_cast<file::size_type>(generated.size()));
    } catch (const exception& e) {
        eprintfln("Error writing output: {}", e.what());
        return 1;
    }

    try {
        string cache_data;
        for (const auto& kv: new_cache) {
            cache_data += kv.second + "|" + kv.first + "\n";
        }
        file cache_file(path(cache_path), false, file_access::WRITE, file_shared::SHARE_READ,
                        file_creation::CREATE_FORCE, file_attri::NORMAL);
        cache_file.write(cache_data.data(), static_cast<file::size_type>(cache_data.size()));
    } catch (const exception& e) {
        eprintfln("Warning: Could not write cache file: {}", e.what());
    }

    eprintfln("Generated {}: {} class(es), {} enum(s)", output_path, all_classes.size(), all_enums.size());

    if (all_classes.empty() && all_enums.empty()) {
        eprintln("Warning: No reflect-registered types found in scanned headers.");
    }

    return 0;
}
