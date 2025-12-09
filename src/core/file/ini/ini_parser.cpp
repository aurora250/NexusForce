#include <MSTL/core/file/ini/ini_parser.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

bool ini_parser::is_comment(const string& line) {
    string trimmed = line;
    trimmed.trim();
    return trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#';
}

bool ini_parser::is_section(const string& line, string& section_name) {
    string trimmed = line;
    trimmed.trim();
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
        section_name = trimmed.substr(1, trimmed.size() - 2).trim();
        return true;
    }
    return false;
}

bool ini_parser::parse_key_value(const string& line, string& key, string& value) {
    const size_t pos = line.find('=');
    if (pos == string::npos) return false;
    key = line.substr(0, pos).trim();
    value = line.substr(pos + 1).trim();

    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
        (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
    }
    return !key.empty();
}

bool ini_parser::try_parse(const string& content) {
    string line;
    string current_section;
    size_t pos = 0;
    while (getline(content, pos, line)) {
        if (is_comment(line)) continue;

        string section_name;
        if (is_section(line, section_name)) {
            current_section = section_name;
            continue;
        }

        string key, value;
        if (parse_key_value(line, key, value)) {
            data_[current_section][key] = value;
        }
    }
    return true;
}

string ini_parser::get_string(const string& section, const string& key, const string& defaultv) const {
    const auto s_it = data_.find(section);
    if (s_it != data_.end()) {
        const auto k_it = s_it->second.find(key);
        if (k_it != s_it->second.end()) {
            return k_it->second;
        }
    }
    return defaultv;
}

int ini_parser::get_int(const string& section, const string& key, const int defaultv) const {
    const string v = get_string(section, key);
    if (v.empty()) return defaultv;
    try {
        return integer32::parse(v.view());
    } catch (...) {
        return defaultv;
    }
}

double ini_parser::get_double(const string& section, const string& key, const double defaultv) const {
    const string v = get_string(section, key);
    if (v.empty()) return defaultv;
    try {
        return float64::parse(v.view());
    } catch (...) {
        return defaultv;
    }
}

bool ini_parser::get_bool(const string& section, const string& key, const bool defaultv) const {
    const string v = get_string(section, key);
    if (v.empty()) return defaultv;
    try {
        return boolean::parse(v.view());
    } catch (...) {
        return defaultv;
    }
}

void ini_parser::set_value(const string& section, const string& key, const string& value) {
    data_[section][key] = value;
}

bool ini_parser::has_section(const string& section) const {
    return data_.find(section) != data_.end();
}

bool ini_parser::has_key(const string& section, const string& key) const {
    const auto s_it = data_.find(section);
    if (s_it != data_.end()) {
        return s_it->second.find(key) != s_it->second.end();
    }
    return false;
}

string ini_parser::to_string() const {
    string result;
    for (const auto& section : data_) {
        if (!section.first.empty()) {
            result += "[" + section.first + "]\n";
        }
        for (const auto& kv : section.second) {
            result += "  " + kv.first +" = " + kv.second + '\n';
        }
        result += '\n';
    }
    return result;
}

MSTL_END_NAMESPACE__
