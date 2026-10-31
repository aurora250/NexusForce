#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/file/path_tree.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/tui/dom/theme.hpp>
#include "theme_internal.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr string_view palette_names[] = {"legacy-dark", "legacy-light", "dark",     "light",
                                             "dark-ansi",   "pink-night",   "pink-day", "pink-ansi"};

    bool equals_ignore_case(const string_view lhs, const string_view rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (size_t i = 0; i < lhs.size(); ++i) {
            char a = lhs[i];
            char b = rhs[i];
            if (a >= 'A' && a <= 'Z') {
                a = static_cast<char>(a - 'A' + 'a');
            }
            if (b >= 'A' && b <= 'Z') {
                b = static_cast<char>(b - 'A' + 'a');
            }
            if (a != b) {
                return false;
            }
        }
        return true;
    }

    string to_lower(string_view text) {
        string result;
        result.reserve(text.size());
        for (const char c: text) {
            result.push_back((c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c);
        }
        return result;
    }

    // Fills the whole role table from the legacy fields.
    // Used before the first role override so partial edits do not leave the rest of the table black.
    void materialize_legacy(theme& t) {
        for (size_t i = 0; i < theme_role_count; ++i) {
            t.roles_[i] = t.get(static_cast<theme_role>(i));
        }
        t.has_palette = true;
    }

    void sync_legacy_fields(theme& t) {
        t.bg = color{0, 0, 0, 0};
        t.fg = t.get(theme_role::text);
        t.primary = t.get(theme_role::accent);
        t.secondary = t.get(theme_role::permission);
        t.danger = t.get(theme_role::error);
        t.success = t.get(theme_role::success);
        t.warning = t.get(theme_role::warning);
        t.muted = t.get(theme_role::inactive);
        t.border = t.get(theme_role::prompt_border);
    }

    int ansi256_to_rgb(const int index, int& r, int& g, int& b) noexcept {
        if (index < 0 || index > 255) {
            return 0;
        }
        if (index < 16) {
            static constexpr int base[16][3] = {{0, 0, 0},       {128, 0, 0},   {0, 128, 0},   {128, 128, 0},
                                                {0, 0, 128},     {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
                                                {128, 128, 128}, {255, 0, 0},   {0, 255, 0},   {255, 255, 0},
                                                {0, 0, 255},     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
            r = base[index][0];
            g = base[index][1];
            b = base[index][2];
            return 1;
        }
        if (index < 232) {
            static constexpr int steps[6] = {0, 95, 135, 175, 215, 255};
            const int v = index - 16;
            r = steps[v / 36];
            g = steps[(v % 36) / 6];
            b = steps[v % 6];
            return 1;
        }
        const int level = 8 + (index - 232) * 10;
        r = level;
        g = level;
        b = level;
        return 1;
    }

    bool parse_uint(string_view text, int& value) noexcept {
        if (text.empty()) {
            return false;
        }
        int result = 0;
        for (const char c: text) {
            if (c < '0' || c > '9') {
                return false;
            }
            result = result * 10 + (c - '0');
            if (result > 1000000) {
                return false;
            }
        }
        value = result;
        return true;
    }

    bool parse_hex_digit(const char c, int& value) noexcept {
        if (c >= '0' && c <= '9') {
            value = c - '0';
            return true;
        }
        if (c >= 'a' && c <= 'f') {
            value = c - 'a' + 10;
            return true;
        }
        if (c >= 'A' && c <= 'F') {
            value = c - 'A' + 10;
            return true;
        }
        return false;
    }

    bool parse_hex_color(const string_view text, color& out) noexcept {
        string_view body = text;
        if (!body.empty() && body[0] == '#') {
            body = body.tail(1);
        }
        if (body.size() != 3 && body.size() != 6 && body.size() != 8) {
            return false;
        }
        int digits[8] = {};
        for (size_t i = 0; i < body.size(); ++i) {
            if (!parse_hex_digit(body[i], digits[i])) {
                return false;
            }
        }
        if (body.size() == 3) {
            out = color{digits[0] * 17, digits[1] * 17, digits[2] * 17};
            return true;
        }
        const int a = (body.size() == 8) ? (digits[6] * 16 + digits[7]) : 255;
        out = color{digits[0] * 16 + digits[1], digits[2] * 16 + digits[3], digits[4] * 16 + digits[5], a};
        return true;
    }

    bool parse_rgb_function(string_view text, color& out) noexcept {
        if (text.size() < 6 || !text.starts_with("rgb(")) {
            return false;
        }
        const string_view body = text.substr(4, text.size() - 5);
        int values[3] = {};
        size_t index = 0;
        size_t begin = 0;
        for (size_t i = 0; i <= body.size(); ++i) {
            if (i == body.size() || body[i] == ',') {
                if (index >= 3) {
                    return false;
                }
                string_view part = body.substr(begin, i - begin);
                while (!part.empty() && (part.front() == ' ' || part.front() == '\t')) {
                    part = part.tail(1);
                }
                while (!part.empty() && (part[part.size() - 1] == ' ' || part[part.size() - 1] == '\t')) {
                    part = part.substr(0, part.size() - 1);
                }
                if (!parse_uint(part, values[index])) {
                    return false;
                }
                ++index;
                begin = i + 1;
            }
        }
        if (index != 3) {
            return false;
        }
        out = color{values[0], values[1], values[2]};
        return true;
    }

    bool parse_ansi_named(string_view text, color& out) noexcept {
        static constexpr const char* names[16] = {"black",       "red",           "green",       "yellow",
                                                  "blue",        "magenta",       "cyan",        "white",
                                                  "blackBright", "redBright",     "greenBright", "yellowBright",
                                                  "blueBright",  "magentaBright", "cyanBright",  "whiteBright"};
        static constexpr int rgb[16][3] = {{0, 0, 0},       {128, 0, 0},   {0, 128, 0},   {128, 128, 0},
                                           {0, 0, 128},     {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
                                           {128, 128, 128}, {255, 0, 0},   {0, 255, 0},   {255, 255, 0},
                                           {0, 0, 255},     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
        if (text.size() < 5 || !text.starts_with("ansi:")) {
            return false;
        }
        const string_view name = text.tail(5);
        for (int i = 0; i < 16; ++i) {
            if (equals_ignore_case(name, string_view{names[i]})) {
                out = color{rgb[i][0], rgb[i][1], rgb[i][2]};
                return true;
            }
        }
        return false;
    }

    bool parse_ansi256_function(string_view text, color& out) noexcept {
        if (text.size() < 8 || !text.starts_with("ansi256(")) {
            return false;
        }
        int index = 0;
        if (!parse_uint(text.substr(8, text.size() - 9), index)) {
            return false;
        }
        int r = 0;
        int g = 0;
        int b = 0;
        if (ansi256_to_rgb(index, r, g, b) == 0) {
            return false;
        }
        out = color{r, g, b};
        return true;
    }

    vector<string> list_theme_files(const string_view directory) {
        path_tree::scan_options options;
        options.max_depth = 1;
        options.include_hidden = true;
        options.files_only = true;
        options.extensions.push_back("json");
        const path_tree tree = path_tree::scan(path(directory), options);
        vector<string> files;
        for (auto& location: tree.all_file_paths()) {
            files.push_back(move(location.release()));
        }
        return files;
    }

    const unsigned int* palette_row(const theme_palette_id id) noexcept {
        if (id == theme_palette_id::legacy_dark || id == theme_palette_id::legacy_light) {
            return nullptr;
        }
        return palette_colors[static_cast<size_t>(id)];
    }
} // namespace


color theme::get(const theme_role role) const noexcept {
    const auto index = static_cast<size_t>(role);
    if (index >= theme_role_count) {
        return border;
    }
    if (has_palette) {
        return roles_[index];
    }
    // Legacy themes only carry the nine historical fields;
    // map the roles the old components actually used and fall back to the muted colour.
    switch (role) {
        case theme_role::accent:
        case theme_role::activity:
            return primary;
        case theme_role::text:
        case theme_role::inverse_text:
            return fg;
        case theme_role::success:
            return success;
        case theme_role::error:
            return danger;
        case theme_role::warning:
            return warning;
        case theme_role::inactive:
        case theme_role::subtle:
            return muted;
        case theme_role::prompt_border:
            return border;
        case theme_role::permission:
        case theme_role::suggestion:
            return secondary;
        default:
            return muted;
    }
}

theme& theme::set(const theme_role role, const color value) noexcept {
    const auto index = static_cast<size_t>(role);
    if (index >= theme_role_count) {
        return *this;
    }
    if (!has_palette) {
        materialize_legacy(*this);
    }
    roles_[index] = value;
    sync_legacy_fields(*this);
    return *this;
}

style theme::button_style(const style::variant v) const {
    style s;
    s.fg = fg;
    s.padding = {0, 2, 0, 2};
    switch (v) {
        case style::variant::primary:
            s.bg = primary;
            s.bold = true;
            break;
        case style::variant::secondary:
            s.bg = secondary;
            break;
        case style::variant::danger:
            s.fg = danger;
            s.bold = true;
            break;
        case style::variant::success:
            s.fg = success;
            s.bold = true;
            break;
        case style::variant::default_:
        default:
            break;
    }
    return s;
}

style theme::text_style() const {
    style s;
    s.fg = fg;
    return s;
}

style theme::input_style(const bool focused) const {
    style s;
    s.fg = fg;
    s.bg = bg;
    if (focused) {
        s.border = style::border::single;
        s.borderColor = primary;
    } else {
        s.border = style::border::single;
        s.borderColor = border;
    }
    s.padding = {0, 1, 0, 1};
    return s;
}

string_view theme_role_name(const theme_role role) noexcept {
    const auto index = static_cast<size_t>(role);
    if (index >= theme_role_count) {
        return "";
    }
    return role_names[index];
}

optional<theme_role> parse_theme_role(const string_view name) noexcept {
    for (size_t i = 0; i < theme_role_count; ++i) {
        if (equals_ignore_case(name, role_names[i])) {
            return static_cast<theme_role>(i);
        }
    }
    return none;
}

string_view palette_name(const theme_palette_id id) noexcept {
    const auto index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(theme_palette_id::count_)) {
        return "";
    }
    return palette_names[index];
}

color lighten(const color c, const double amount) noexcept {
    return color::lerp(c, color{255, 255, 255, c.A()}, amount);
}

color darken(const color c, const double amount) noexcept { return color::lerp(c, color{0, 0, 0, c.A()}, amount); }

optional<color> parse_color(const string_view text) noexcept {
    string_view trimmed = text;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) {
        trimmed = trimmed.tail(1);
    }
    while (!trimmed.empty() && (trimmed[trimmed.size() - 1] == ' ' || trimmed[trimmed.size() - 1] == '\t')) {
        trimmed = trimmed.substr(0, trimmed.size() - 1);
    }
    if (trimmed.empty()) {
        return none;
    }
    color result;
    if (trimmed[0] == '#') {
        if (parse_hex_color(trimmed, result)) {
            return result;
        }
        return none;
    }
    if (trimmed.starts_with("rgb(")) {
        if (parse_rgb_function(trimmed, result)) {
            return result;
        }
        return none;
    }
    if (trimmed.starts_with("ansi256(")) {
        if (parse_ansi256_function(trimmed, result)) {
            return result;
        }
        return none;
    }
    if (trimmed.starts_with("ansi:")) {
        if (parse_ansi_named(trimmed, result)) {
            return result;
        }
        return none;
    }
    return none;
}

theme make_palette(const theme_palette_id id) {
    if (id == theme_palette_id::legacy_dark) {
        return dark_theme;
    }
    if (id == theme_palette_id::legacy_light) {
        return light_theme;
    }
    theme result{};
    result.has_palette = true;
    const unsigned int* row = palette_row(id);
    for (size_t i = 0; i < theme_role_count; ++i) {
        const unsigned int value = (row != nullptr) ? row[i] : 0x1000000U;
        if (value == 0x1000000U) {
            result.roles_[i] = color{0, 0, 0, 0};
        } else {
            result.roles_[i] = color{static_cast<int>((value >> 16) & 0xFFU), static_cast<int>((value >> 8) & 0xFFU),
                                     static_cast<int>(value & 0xFFU)};
        }
    }
    sync_legacy_fields(result);
    return result;
}

optional<theme> make_palette(const string_view name) {
    for (size_t i = 0; i < static_cast<size_t>(theme_palette_id::count_); ++i) {
        const auto id = static_cast<theme_palette_id>(i);
        if (equals_ignore_case(name, palette_name(id))) {
            return make_palette(id);
        }
    }
    return none;
}

theme_catalog& theme_catalog::instance() {
    static theme_catalog catalog;
    return catalog;
}

vector<string> theme_catalog::names() const {
    vector<string> result;
    result.push_back(string{"auto"});
    for (size_t i = 2; i < static_cast<size_t>(theme_palette_id::count_); ++i) {
        result.push_back(string{palette_name(static_cast<theme_palette_id>(i))});
    }
    for (const auto& entry: user_themes_) {
        result.push_back(entry.first);
    }
    return result;
}

optional<theme> theme_catalog::find(const string_view name) const {
    if (equals_ignore_case(name, string_view{"auto"})) {
        return make_palette(terminal_light_ ? theme_palette_id::light : theme_palette_id::dark);
    }
    if (const auto builtin = make_palette(name)) {
        return builtin;
    }
    for (const auto& entry: user_themes_) {
        if (equals_ignore_case(name, entry.first.view())) {
            return entry.second;
        }
    }
    return none;
}

theme theme_catalog::resolve(const string_view name) const {
    if (const auto found = find(name)) {
        return *found;
    }
    return make_palette(terminal_light_ ? theme_palette_id::light : theme_palette_id::dark);
}

void theme_catalog::clear_user_themes() { user_themes_.clear(); }

void theme_catalog::set_terminal_light(const bool light) noexcept { terminal_light_ = light; }

bool theme_catalog::register_theme(const string_view json) {
    auto parsed = json_parser{string{json}}.try_parse();
    if (!parsed.has_value() || parsed->get() == nullptr) {
        return false;
    }
    const auto* root = parsed->get()->as_object();
    if (root == nullptr) {
        return false;
    }
    const auto* base_value = root->get_member(string{"base"});
    if (base_value == nullptr || base_value->as_string() == nullptr) {
        return false;
    }
    const string& base_name = base_value->as_string()->get_value();
    auto palette = make_palette(string_view{base_name.data(), base_name.size()});
    if (!palette.has_value()) {
        return false;
    }

    string name;
    if (const auto* name_value = root->get_member(string{"name"});
        name_value != nullptr && name_value->as_string() != nullptr) {
        name = name_value->as_string()->get_value();
    } else if (const auto* display = root->get_member(string{"displayName"});
               display != nullptr && display->as_string() != nullptr) {
        name = to_lower(display->as_string()->get_value().view());
        for (char& c: name) {
            if (c == ' ') {
                c = '-';
            }
        }
    }
    if (name.empty() || equals_ignore_case(name.view(), string_view{"auto"})) {
        return false;
    }

    if (const auto* colors = root->get_member(string{"colors"}); colors != nullptr) {
        if (const auto* color_object = colors->as_object(); color_object != nullptr) {
            for (const auto& entry: color_object->get_members()) {
                const auto role = parse_theme_role(entry.first.view());
                if (!role.has_value() || entry.second == nullptr || entry.second->as_string() == nullptr) {
                    continue;
                }
                if (const auto parsed_color = parse_color(entry.second->as_string()->get_value().view())) {
                    palette->set(*role, *parsed_color);
                }
            }
        }
    }

    for (auto& entry: user_themes_) {
        if (equals_ignore_case(name.view(), entry.first.view())) {
            entry.second = *palette;
            return true;
        }
    }
    user_themes_.push_back(pair<string, theme>{move(name), *palette});
    return true;
}

bool theme_catalog::load_user_themes(const string_view directory) {
    bool loaded = false;
    for (const auto& theme_file: list_theme_files(directory)) {
        const path location{theme_file};
        if (!location.exists()) {
            continue;
        }
        file input{location, false, file_access::read};
        if (!input.is_opened()) {
            continue;
        }
        const string content = input.read();
        if (register_theme(content.view())) {
            loaded = true;
        }
    }
    return loaded;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
