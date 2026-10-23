#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/dom/latex.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr string_view LAYOUT_MARKER_START = "\xF3\xB0\x80\x80";
    constexpr string_view LAYOUT_MARKER_END = "\xF3\xB0\x80\x81";
    constexpr string_view PROTECTED_SPACE = "\xF3\xB0\x80\x82";
    constexpr string_view NAMED_OPERATOR_START = "\xF3\xB0\x80\x84";
    constexpr string_view NAMED_OPERATOR_END = "\xF3\xB0\x80\x85";
    constexpr size_t MARKER_BYTES = 4;
    constexpr char NEGATIVE_SPACE = '\0';

    constexpr uint32_t CP_LAYOUT_MARKER_START = 0xF0000;
    constexpr uint32_t CP_LAYOUT_MARKER_END = 0xF0001;
    constexpr uint32_t CP_NAMED_OPERATOR_START = 0xF0004;

    struct layout_node {
        enum class kind {
            fraction,
            op,
            script,
            matrix
        };

        kind type = kind::fraction;
        string numerator;
        string denominator;
        string op;
        optional<string> lower;
        optional<string> upper;
        vector<string> lines;
        size_t baseline = 0;
    };

    struct layout {
        vector<string> lines;
        int width = 0;
        int baseline = 0;

        layout() = default;
        layout(vector<string> value, const int w, const int b) :
        lines(move(value)),
        width(w),
        baseline(b) {}
    };

    bool is_word(const char character) noexcept { return is_alpha_or_digit(character) || character == '_'; }

    size_t codepoint_bytes(const char lead) noexcept {
        const auto value = static_cast<unsigned char>(lead);
        if ((value & 0x80U) == 0) {
            return 1;
        }
        if ((value & 0xE0U) == 0xC0U) {
            return 2;
        }
        if ((value & 0xF0U) == 0xE0U) {
            return 3;
        }
        if ((value & 0xF8U) == 0xF0U) {
            return 4;
        }
        return 1;
    }

    size_t next_codepoint(const string_view text, const size_t position) noexcept {
        if (position >= text.size()) {
            return text.size();
        }
        return min(text.size(), position + codepoint_bytes(text[position]));
    }

    uint32_t decode_codepoint(const string_view text, const size_t position) noexcept {
        size_t offset = position;
        const codepoint value =
                codepoint::decode_utf8(reinterpret_cast<const byte_t*>(text.data()), offset, text.size());
        return value.value();
    }

    bool is_js_space(const uint32_t value) noexcept {
        if (value == 0x20U || (value >= 0x09U && value <= 0x0DU)) {
            return true;
        }
        if (value == 0xA0U || value == 0x1680U || value == 0x2028U || value == 0x2029U || value == 0x202FU ||
            value == 0x205FU || value == 0x3000U || value == 0xFEFFU) {
            return true;
        }
        return value >= 0x2000U && value <= 0x200AU;
    }

    bool is_space_at(const string_view text, const size_t position) noexcept {
        return position < text.size() && is_js_space(decode_codepoint(text, position));
    }

    size_t skip_space(const string_view text, size_t position) noexcept {
        while (position < text.size() && is_space_at(text, position)) {
            position = next_codepoint(text, position);
        }
        return position;
    }

    size_t skip_horizontal_space(const string_view text, size_t position) noexcept {
        while (position < text.size() && (text[position] == ' ' || text[position] == '\t')) {
            ++position;
        }
        return position;
    }

    size_t trim_end_index(const string_view text) noexcept {
        size_t end = 0;
        size_t position = 0;
        while (position < text.size()) {
            const uint32_t value = decode_codepoint(text, position);
            position = next_codepoint(text, position);
            if (!is_js_space(value)) {
                end = position;
            }
        }
        return end;
    }

    size_t trim_start_index(const string_view text) noexcept { return skip_space(text, 0); }

    string trim_end(const string_view text) { return string(text.substr(0, trim_end_index(text))); }

    string trim_start(const string_view text) { return string(text.substr(trim_start_index(text))); }

    string trim(const string_view text) {
        const size_t begin = trim_start_index(text);
        return trim_end(text.substr(begin));
    }

    void trim_end_in_place(string& value) {
        const size_t end = trim_end_index(value.view());
        if (end < value.size()) {
            value.erase(end);
        }
    }

    int visible_width(const string_view text) noexcept {
        int width = 0;
        const auto* data = reinterpret_cast<const byte_t*>(text.data());
        for (utf8_iterator iter(data, text.size()); iter != utf8_iterator(); ++iter) {
            width += iter->display_width();
        }
        return width;
    }

    size_t codepoint_count(const string_view text) noexcept {
        size_t count = 0;
        const auto* data = reinterpret_cast<const byte_t*>(text.data());
        for (utf8_iterator iter(data, text.size()); iter != utf8_iterator(); ++iter) {
            ++count;
        }
        return count;
    }

    string first_codepoint(const string_view text) {
        if (text.empty()) {
            return {};
        }
        return string(text.substr(0, codepoint_bytes(text[0])));
    }

    string spaces(const int count) { return count > 0 ? string(static_cast<size_t>(count), ' ') : string(); }

    string repeat_text(const string_view unit, const size_t count) {
        string result;
        if (count == 0 || unit.empty()) {
            return result;
        }
        result.reserve(unit.size() * count);
        for (size_t i = 0; i < count; ++i) {
            result += unit;
        }
        return result;
    }

    // TODO: replace the approximate ranges below with full Unicode general category tables once the core library exposes them.
    bool is_unicode_letter(const uint32_t value) noexcept {
        if (value < 0x80U) {
            return is_alpha(static_cast<char>(value));
        }
        if (value == 0x00AAU || value == 0x00B5U || value == 0x00BAU) {
            return true;
        }
        if ((value >= 0x00C0U && value <= 0x00D6U) || (value >= 0x00D8U && value <= 0x00F6U) ||
            (value >= 0x00F8U && value <= 0x02AFU)) {
            return true;
        }
        if ((value >= 0x0370U && value <= 0x0373U) || (value >= 0x0376U && value <= 0x0377U) ||
            (value >= 0x037AU && value <= 0x037DU) || value == 0x037FU || value == 0x0386U ||
            (value >= 0x0388U && value <= 0x038AU) || value == 0x038CU || (value >= 0x038EU && value <= 0x03A1U) ||
            (value >= 0x03A3U && value <= 0x03F5U) || (value >= 0x03F7U && value <= 0x03FFU)) {
            return true;
        }
        if (value >= 0x0400U && value <= 0x052FU) {
            return true;
        }
        if ((value >= 0x0531U && value <= 0x0556U) || (value >= 0x0561U && value <= 0x0587U)) {
            return true;
        }
        if ((value >= 0x05D0U && value <= 0x05EAU) || (value >= 0x05EFU && value <= 0x05F2U)) {
            return true;
        }
        if ((value >= 0x0620U && value <= 0x064AU) || (value >= 0x066EU && value <= 0x06D3U) || value == 0x06D5U ||
            (value >= 0x06E5U && value <= 0x06E6U) || (value >= 0x06EEU && value <= 0x06EFU) ||
            (value >= 0x06FAU && value <= 0x06FCU) || value == 0x06FFU) {
            return true;
        }
        if ((value >= 0x0710U && value <= 0x072FU) || (value >= 0x074DU && value <= 0x07A5U)) {
            return true;
        }
        if ((value >= 0x0904U && value <= 0x0939U) || value == 0x093DU || value == 0x0950U ||
            (value >= 0x0958U && value <= 0x0961U) || (value >= 0x0971U && value <= 0x0980U)) {
            return true;
        }
        if ((value >= 0x0985U && value <= 0x09B9U) || (value >= 0x0A05U && value <= 0x0A39U) ||
            (value >= 0x0A85U && value <= 0x0AB9U) || (value >= 0x0B05U && value <= 0x0B39U) ||
            (value >= 0x0B85U && value <= 0x0BB9U) || (value >= 0x0C05U && value <= 0x0C39U) ||
            (value >= 0x0C85U && value <= 0x0CB9U) || (value >= 0x0D05U && value <= 0x0D39U)) {
            return true;
        }
        if ((value >= 0x0E01U && value <= 0x0E30U) || (value >= 0x0E32U && value <= 0x0E33U) ||
            (value >= 0x0E40U && value <= 0x0E46U) || (value >= 0x0E81U && value <= 0x0EB0U) ||
            (value >= 0x0EB2U && value <= 0x0EB3U) || (value >= 0x0EC0U && value <= 0x0EC6U)) {
            return true;
        }
        if ((value >= 0x10A0U && value <= 0x10FAU) || (value >= 0x10FCU && value <= 0x10FFU)) {
            return true;
        }
        if (value >= 0x1100U && value <= 0x11FFU) {
            return true;
        }
        if ((value >= 0x1200U && value <= 0x137FU) || (value >= 0x13A0U && value <= 0x13FDU) ||
            (value >= 0x1401U && value <= 0x166CU) || (value >= 0x1681U && value <= 0x169AU)) {
            return true;
        }
        if ((value >= 0x1700U && value <= 0x170CU) || (value >= 0x1720U && value <= 0x1734U) ||
            (value >= 0x1740U && value <= 0x1753U) || (value >= 0x1760U && value <= 0x1770U) ||
            (value >= 0x1780U && value <= 0x17B3U)) {
            return true;
        }
        if ((value >= 0x1E00U && value <= 0x1FBCU) || value == 0x1FBEU || (value >= 0x1FC2U && value <= 0x1FCCU) ||
            (value >= 0x1FD0U && value <= 0x1FDBU) || (value >= 0x1FE0U && value <= 0x1FECU) ||
            (value >= 0x1FF2U && value <= 0x1FFCU)) {
            return true;
        }
        if ((value >= 0x02B0U && value <= 0x02C1U) || (value >= 0x02C6U && value <= 0x02D1U) ||
            (value >= 0x02E0U && value <= 0x02E4U) || value == 0x02ECU || value == 0x02EEU) {
            return true;
        }
        if (value >= 0x1D2CU && value <= 0x1DBFU) {
            return true;
        }
        if ((value >= 0x1D400U && value <= 0x1D6A3U) || (value >= 0x1D6A8U && value <= 0x1D7CBU)) {
            return true;
        }
        if (value == 0x2071U || value == 0x207FU || (value >= 0x2090U && value <= 0x209CU)) {
            return true;
        }
        if (value == 0x2102U || value == 0x2107U || (value >= 0x210AU && value <= 0x2113U) || value == 0x2115U ||
            (value >= 0x2119U && value <= 0x211DU) || value == 0x2124U || value == 0x2126U || value == 0x2128U ||
            (value >= 0x212AU && value <= 0x212DU) || (value >= 0x212FU && value <= 0x2139U)) {
            return true;
        }
        if ((value >= 0x2C00U && value <= 0x2C7FU) || (value >= 0x2C80U && value <= 0x2CE4U) ||
            (value >= 0x2D00U && value <= 0x2D2DU) || (value >= 0x2D30U && value <= 0x2D67U)) {
            return true;
        }
        if ((value >= 0x3005U && value <= 0x3006U) || (value >= 0x3031U && value <= 0x3035U) ||
            (value >= 0x3041U && value <= 0x3096U) || (value >= 0x309BU && value <= 0x30FFU) ||
            (value >= 0x3105U && value <= 0x312FU) || (value >= 0x3131U && value <= 0x318EU) ||
            (value >= 0x31A0U && value <= 0x31BFU)) {
            return true;
        }
        if ((value >= 0x3400U && value <= 0x4DBFU) || (value >= 0x4E00U && value <= 0x9FFFU) ||
            (value >= 0xA000U && value <= 0xA48CU) || (value >= 0xA4D0U && value <= 0xA4FDU) ||
            (value >= 0xA500U && value <= 0xA60CU) || (value >= 0xA640U && value <= 0xA66EU) ||
            (value >= 0xA680U && value <= 0xA69BU) || (value >= 0xA722U && value <= 0xA788U)) {
            return true;
        }
        if ((value >= 0xAC00U && value <= 0xD7A3U) || (value >= 0xF900U && value <= 0xFA6DU) ||
            (value >= 0xFA70U && value <= 0xFAD9U) || (value >= 0xFB00U && value <= 0xFB06U) ||
            (value >= 0xFB13U && value <= 0xFB17U) || (value >= 0xFB1DU && value <= 0xFB4FU) ||
            (value >= 0xFB50U && value <= 0xFBC1U) || (value >= 0xFBD3U && value <= 0xFD3DU) ||
            (value >= 0xFD50U && value <= 0xFDFCU) || (value >= 0xFE70U && value <= 0xFEFCU)) {
            return true;
        }
        if ((value >= 0xFF21U && value <= 0xFF3AU) || (value >= 0xFF41U && value <= 0xFF5AU) ||
            (value >= 0xFF66U && value <= 0xFFBEU) || (value >= 0xFFC2U && value <= 0xFFC7U) ||
            (value >= 0xFFCAU && value <= 0xFFCFU) || (value >= 0xFFD2U && value <= 0xFFD7U) ||
            (value >= 0xFFDAU && value <= 0xFFDCU)) {
            return true;
        }
        return (value >= 0x20000U && value <= 0x2FFFDU) || (value >= 0x30000U && value <= 0x3FFFDU);
    }

    bool is_unicode_number(const uint32_t value) noexcept {
        if (value < 0x80U) {
            return is_digit(static_cast<char>(value));
        }
        if (value == 0x00B2U || value == 0x00B3U || value == 0x00B9U || (value >= 0x00BCU && value <= 0x00BEU)) {
            return true;
        }
        if ((value >= 0x0660U && value <= 0x0669U) || (value >= 0x06F0U && value <= 0x06F9U) ||
            (value >= 0x0966U && value <= 0x096FU) || (value >= 0x09E6U && value <= 0x09EFU) ||
            (value >= 0x0A66U && value <= 0x0A6FU) || (value >= 0x0AE6U && value <= 0x0AEFU) ||
            (value >= 0x0B66U && value <= 0x0B6FU) || (value >= 0x0BE6U && value <= 0x0BEFU) ||
            (value >= 0x0C66U && value <= 0x0C6FU) || (value >= 0x0CE6U && value <= 0x0CEFU) ||
            (value >= 0x0D66U && value <= 0x0D6FU) || (value >= 0x0E50U && value <= 0x0E59U) ||
            (value >= 0x0ED0U && value <= 0x0ED9U) || (value >= 0x0F20U && value <= 0x0F33U)) {
            return true;
        }
        if ((value >= 0x1040U && value <= 0x1049U) || (value >= 0x1090U && value <= 0x1099U) ||
            (value >= 0x17E0U && value <= 0x17E9U) || (value >= 0x1810U && value <= 0x1819U)) {
            return true;
        }
        if ((value >= 0x2070U && value <= 0x2079U) || (value >= 0x2080U && value <= 0x2089U) ||
            (value >= 0x2150U && value <= 0x2182U) || (value >= 0x2460U && value <= 0x24FFU) ||
            (value >= 0x2776U && value <= 0x2793U)) {
            return true;
        }
        if (value == 0x3007U || (value >= 0x3021U && value <= 0x3029U) || (value >= 0x3038U && value <= 0x303AU) ||
            (value >= 0x3192U && value <= 0x3195U) || (value >= 0x3220U && value <= 0x3229U) ||
            (value >= 0x3251U && value <= 0x325FU) || (value >= 0x3280U && value <= 0x3289U) ||
            (value >= 0x32B1U && value <= 0x32BFU)) {
            return true;
        }
        if ((value >= 0xFF10U && value <= 0xFF19U) || (value >= 0x1D7CEU && value <= 0x1D7FFU)) {
            return true;
        }
        return false;
    }

    bool is_letter_number_or_dot(const string_view text) noexcept {
        if (text.empty()) {
            return false;
        }
        size_t position = 0;
        while (position < text.size()) {
            const uint32_t value = decode_codepoint(text, position);
            if (!is_unicode_letter(value) && !is_unicode_number(value) && value != '.') {
                return false;
            }
            position = next_codepoint(text, position);
        }
        return true;
    }

    bool is_number_or_dot(const string_view text) noexcept {
        if (text.empty()) {
            return false;
        }
        size_t position = 0;
        while (position < text.size()) {
            const uint32_t value = decode_codepoint(text, position);
            if (!is_unicode_number(value) && value != '.') {
                return false;
            }
            position = next_codepoint(text, position);
        }
        return true;
    }

    bool is_all_ascii_letters(const string_view text) noexcept {
        if (text.empty()) {
            return false;
        }
        for (const char character: text) {
            if (!is_alpha(character)) {
                return false;
            }
        }
        return true;
    }

    bool contains_upper_or_star(const string_view text) noexcept {
        size_t position = 0;
        while (position < text.size()) {
            const uint32_t value = decode_codepoint(text, position);
            if ((value >= 'A' && value <= 'Z') || value == '*' || value == 0x2217U) {
                return true;
            }
            position = next_codepoint(text, position);
        }
        return false;
    }

    void remove_ascii_spaces(string& value) {
        if (!value.contains(' ')) {
            return;
        }
        string result;
        result.reserve(value.size());
        for (const char character: value) {
            if (character != ' ') {
                result += character;
            }
        }
        value = move(result);
    }

    struct symbol_entry {
        string_view name;
        string_view value;
    };

    struct char_entry {
        char key;
        string_view value;
    };

    constexpr symbol_entry SYMBOLS[] = {
            {"alpha", "α"},
            {"beta", "β"},
            {"gamma", "γ"},
            {"delta", "δ"},
            {"epsilon", "ϵ"},
            {"varepsilon", "ε"},
            {"zeta", "ζ"},
            {"eta", "η"},
            {"theta", "θ"},
            {"vartheta", "ϑ"},
            {"iota", "ι"},
            {"kappa", "κ"},
            {"varkappa", "ϰ"},
            {"lambda", "λ"},
            {"mu", "μ"},
            {"nu", "ν"},
            {"xi", "ξ"},
            {"pi", "π"},
            {"varpi", "ϖ"},
            {"rho", "ρ"},
            {"varrho", "ϱ"},
            {"sigma", "σ"},
            {"varsigma", "ς"},
            {"tau", "τ"},
            {"upsilon", "υ"},
            {"phi", "ϕ"},
            {"varphi", "φ"},
            {"chi", "χ"},
            {"psi", "ψ"},
            {"omega", "ω"},
            {"Gamma", "Γ"},
            {"Delta", "Δ"},
            {"Theta", "Θ"},
            {"Lambda", "Λ"},
            {"Xi", "Ξ"},
            {"Pi", "Π"},
            {"Sigma", "Σ"},
            {"Upsilon", "Υ"},
            {"Phi", "Φ"},
            {"Psi", "Ψ"},
            {"Omega", "Ω"},
            {"pm", "±"},
            {"mp", "∓"},
            {"times", "×"},
            {"div", "÷"},
            {"cdot", "·"},
            {"ast", "∗"},
            {"star", "⋆"},
            {"circ", "∘"},
            {"bullet", "•"},
            {"oplus", "⊕"},
            {"ominus", "⊖"},
            {"otimes", "⊗"},
            {"oslash", "⊘"},
            {"odot", "⊙"},
            {"bigcirc", "○"},
            {"dagger", "†"},
            {"ddagger", "‡"},
            {"amalg", "⨿"},
            {"uplus", "⊎"},
            {"sqcap", "⊓"},
            {"sqcup", "⊔"},
            {"bowtie", "⋈"},
            {"Join", "⋈"},
            {"ltimes", "⋉"},
            {"rtimes", "⋊"},
            {"leftouterjoin", "⟕"},
            {"rightouterjoin", "⟖"},
            {"fullouterjoin", "⟗"},
            {"triangleleft", "◁"},
            {"triangleright", "▷"},
            {"wr", "≀"},
            {"cap", "∩"},
            {"cup", "∪"},
            {"bigcap", "⋂"},
            {"bigcup", "⋃"},
            {"bigwedge", "⋀"},
            {"bigvee", "⋁"},
            {"bigsqcup", "⨆"},
            {"biguplus", "⨄"},
            {"bigoplus", "⨁"},
            {"bigotimes", "⨂"},
            {"bigodot", "⨀"},
            {"setminus", "∖"},
            {"in", "∈"},
            {"notin", "∉"},
            {"ni", "∋"},
            {"subset", "⊂"},
            {"supset", "⊃"},
            {"subseteq", "⊆"},
            {"supseteq", "⊇"},
            {"sqsubset", "⊏"},
            {"sqsupset", "⊐"},
            {"sqsubseteq", "⊑"},
            {"sqsupseteq", "⊒"},
            {"prec", "≺"},
            {"preceq", "≼"},
            {"succ", "≻"},
            {"succeq", "≽"},
            {"ll", "≪"},
            {"gg", "≫"},
            {"le", "≤"},
            {"leq", "≤"},
            {"leqslant", "≤"},
            {"ge", "≥"},
            {"geq", "≥"},
            {"geqslant", "≥"},
            {"ne", "≠"},
            {"neq", "≠"},
            {"equiv", "≡"},
            {"approx", "≈"},
            {"sim", "∼"},
            {"simeq", "≃"},
            {"cong", "≅"},
            {"asymp", "≍"},
            {"doteq", "≐"},
            {"propto", "∝"},
            {"parallel", "∥"},
            {"perp", "⊥"},
            {"mid", "∣"},
            {"vdash", "⊢"},
            {"dashv", "⊣"},
            {"models", "⊨"},
            {"Vdash", "⊩"},
            {"Vvdash", "⊪"},
            {"nvdash", "⊬"},
            {"nvDash", "⊭"},
            {"forall", "∀"},
            {"exists", "∃"},
            {"nexists", "∄"},
            {"neg", "¬"},
            {"land", "∧"},
            {"wedge", "∧"},
            {"lor", "∨"},
            {"vee", "∨"},
            {"to", "→"},
            {"rightarrow", "→"},
            {"longrightarrow", "→"},
            {"leftarrow", "←"},
            {"longleftarrow", "←"},
            {"gets", "←"},
            {"leftrightarrow", "↔"},
            {"longleftrightarrow", "↔"},
            {"hookleftarrow", "↩"},
            {"hookrightarrow", "↪"},
            {"twoheadleftarrow", "↞"},
            {"twoheadrightarrow", "↠"},
            {"leftharpoonup", "↼"},
            {"leftharpoondown", "↽"},
            {"rightharpoonup", "⇀"},
            {"rightharpoondown", "⇁"},
            {"rightleftharpoons", "⇌"},
            {"leftrightharpoons", "⇋"},
            {"nearrow", "↗"},
            {"searrow", "↘"},
            {"swarrow", "↙"},
            {"nwarrow", "↖"},
            {"rightsquigarrow", "⇝"},
            {"leadsto", "⇝"},
            {"Rightarrow", "⇒"},
            {"Longrightarrow", "⇒"},
            {"Leftarrow", "⇐"},
            {"Longleftarrow", "⇐"},
            {"Leftrightarrow", "⇔"},
            {"Longleftrightarrow", "⇔"},
            {"implies", "⇒"},
            {"iff", "⇔"},
            {"mapsto", "↦"},
            {"longmapsto", "↦"},
            {"uparrow", "↑"},
            {"downarrow", "↓"},
            {"partial", "∂"},
            {"nabla", "∇"},
            {"int", "∫"},
            {"iint", "∬"},
            {"iiint", "∭"},
            {"oint", "∮"},
            {"sum", "∑"},
            {"prod", "∏"},
            {"coprod", "∐"},
            {"infty", "∞"},
            {"emptyset", "∅"},
            {"varnothing", "∅"},
            {"angle", "∠"},
            {"therefore", "∴"},
            {"because", "∵"},
            {"aleph", "ℵ"},
            {"beth", "ℶ"},
            {"gimel", "ℷ"},
            {"daleth", "ℸ"},
            {"top", "⊤"},
            {"bot", "⊥"},
            {"triangle", "△"},
            {"square", "□"},
            {"lozenge", "◊"},
            {"checkmark", "✓"},
            {"complement", "∁"},
            {"wp", "℘"},
            {"prime", "′"},
            {"ldots", "…"},
            {"dots", "…"},
            {"cdots", "⋯"},
            {"vdots", "⋮"},
            {"ddots", "⋱"},
            {"ell", "ℓ"},
            {"hbar", "ℏ"},
            {"Im", "ℑ"},
            {"Re", "ℜ"},
            {"langle", "⟨"},
            {"rangle", "⟩"},
            {"vert", "|"},
            {"lvert", "|"},
            {"rvert", "|"},
            {"Vert", "‖"},
            {"lVert", "‖"},
            {"rVert", "‖"},
            {"lbrace", "{"},
            {"rbrace", "}"},
            {"backslash", "\\"},
            {"lfloor", "⌊"},
            {"rfloor", "⌋"},
            {"lceil", "⌈"},
            {"rceil", "⌉"},
            {"colon", ":"},
    };

    constexpr symbol_entry NEGATED_SYMBOLS[] = {
            {"<", "≮"}, {">", "≯"}, {"=", "≠"}, {"∈", "∉"}, {"∋", "∌"}, {"∣", "∤"}, {"∥", "∦"}, {"∼", "≁"},
            {"≃", "≄"}, {"≅", "≇"}, {"≈", "≉"}, {"≡", "≢"}, {"≤", "≰"}, {"≥", "≱"}, {"≺", "⊀"}, {"≻", "⊁"},
            {"⊂", "⊄"}, {"⊃", "⊅"}, {"⊆", "⊈"}, {"⊇", "⊉"}, {"⊢", "⊬"}, {"⊨", "⊭"}, {"↔", "↮"}, {"←", "↚"},
            {"→", "↛"}, {"⇒", "⇏"}, {"⇐", "⇍"}, {"⇔", "⇎"}, {"≼", "⋠"}, {"≽", "⋡"},
    };

    constexpr char_entry BLACKBOARD[] = {
            {'C', "ℂ"}, {'H', "ℍ"}, {'N', "ℕ"}, {'P', "ℙ"}, {'Q', "ℚ"}, {'R', "ℝ"}, {'Z', "ℤ"},
    };

    constexpr char_entry SUPERSCRIPTS[] = {
            {'0', "⁰"}, {'1', "¹"}, {'2', "²"}, {'3', "³"}, {'4', "⁴"}, {'5', "⁵"}, {'6', "⁶"}, {'7', "⁷"},
            {'8', "⁸"}, {'9', "⁹"}, {'+', "⁺"}, {'-', "⁻"}, {'=', "⁼"}, {'(', "⁽"}, {')', "⁾"}, {'a', "ᵃ"},
            {'b', "ᵇ"}, {'c', "ᶜ"}, {'d', "ᵈ"}, {'e', "ᵉ"}, {'f', "ᶠ"}, {'g', "ᵍ"}, {'h', "ʰ"}, {'i', "ⁱ"},
            {'j', "ʲ"}, {'k', "ᵏ"}, {'l', "ˡ"}, {'m', "ᵐ"}, {'n', "ⁿ"}, {'o', "ᵒ"}, {'p', "ᵖ"}, {'r', "ʳ"},
            {'s', "ˢ"}, {'t', "ᵗ"}, {'u', "ᵘ"}, {'v', "ᵛ"}, {'w', "ʷ"}, {'x', "ˣ"}, {'y', "ʸ"}, {'z', "ᶻ"},
    };

    constexpr char_entry SUBSCRIPTS[] = {
            {'0', "₀"}, {'1', "₁"}, {'2', "₂"}, {'3', "₃"}, {'4', "₄"}, {'5', "₅"}, {'6', "₆"}, {'7', "₇"},
            {'8', "₈"}, {'9', "₉"}, {'+', "₊"}, {'-', "₋"}, {'=', "₌"}, {'(', "₍"}, {')', "₎"}, {'a', "ₐ"},
            {'e', "ₑ"}, {'h', "ₕ"}, {'i', "ᵢ"}, {'j', "ⱼ"}, {'k', "ₖ"}, {'l', "ₗ"}, {'m', "ₘ"}, {'n', "ₙ"},
            {'o', "ₒ"}, {'p', "ₚ"}, {'r', "ᵣ"}, {'s', "ₛ"}, {'t', "ₜ"}, {'u', "ᵤ"}, {'v', "ᵥ"}, {'x', "ₓ"},
    };

    constexpr string_view NAMED_OPERATORS[] = {
            "arccos", "arcsin", "arctan", "arg", "cos", "cosh", "cot",  "coth", "csc",    "deg",    "det",
            "dim",    "exp",    "gcd",    "hom", "inf", "ker",  "lg",   "lim",  "liminf", "limsup", "ln",
            "log",    "max",    "min",    "Pr",  "sec", "sin",  "sinh", "sup",  "tan",    "tanh",
    };

    constexpr string_view LIMIT_OPERATORS[] = {
            "argmax", "argmin", "inf", "injlim", "lim", "liminf", "limsup", "max", "min", "projlim", "sup",
    };

    constexpr string_view DISPLAY_LIMIT_SYMBOLS[] = {
            "bigcap",   "bigcup", "bigodot", "bigoplus", "bigotimes", "bigsqcup", "biguplus", "bigvee",
            "bigwedge", "coprod", "int",     "iint",     "iiint",     "oint",     "prod",     "sum",
    };

    constexpr string_view RELATION_COMMANDS[] = {
            "Leftarrow",
            "Leftrightarrow",
            "Longleftarrow",
            "Longleftrightarrow",
            "Longrightarrow",
            "Rightarrow",
            "Join",
            "Vdash",
            "Vvdash",
            "approx",
            "asymp",
            "bowtie",
            "cong",
            "dashv",
            "fullouterjoin",
            "doteq",
            "downarrow",
            "equiv",
            "ge",
            "geq",
            "geqslant",
            "gets",
            "gg",
            "hookleftarrow",
            "hookrightarrow",
            "iff",
            "implies",
            "in",
            "leadsto",
            "le",
            "leftarrow",
            "leftharpoondown",
            "leftharpoonup",
            "leftrightarrow",
            "leftrightharpoons",
            "leftouterjoin",
            "leq",
            "leqslant",
            "ll",
            "longleftarrow",
            "longleftrightarrow",
            "longmapsto",
            "longrightarrow",
            "ltimes",
            "mapsto",
            "mid",
            "models",
            "ne",
            "nearrow",
            "neq",
            "ni",
            "notin",
            "nvdash",
            "nvDash",
            "nwarrow",
            "parallel",
            "perp",
            "prec",
            "preceq",
            "propto",
            "rightharpoondown",
            "rightharpoonup",
            "rightleftharpoons",
            "rightouterjoin",
            "rightarrow",
            "rightsquigarrow",
            "rtimes",
            "searrow",
            "sim",
            "simeq",
            "sqsubset",
            "sqsubseteq",
            "sqsupset",
            "sqsupseteq",
            "subset",
            "subseteq",
            "succ",
            "succeq",
            "supset",
            "supseteq",
            "swarrow",
            "to",
            "triangleleft",
            "triangleright",
            "twoheadleftarrow",
            "twoheadrightarrow",
            "uparrow",
            "vdash",
    };

    constexpr string_view SPACING_COMMANDS[] = {
            ",", ":", ";", " ", ">", "enspace", "enskip", "medspace", "quad", "qquad", "thickspace", "thinspace",
    };

    constexpr string_view NEGATIVE_SPACING_COMMANDS[] = {
            "!",
            "negmedspace",
            "negthickspace",
            "negthinspace",
    };

    constexpr string_view FONT_SWITCH_COMMANDS[] = {
            "bf", "cal", "it", "rm", "sf", "sl", "tt",
    };

    constexpr string_view IGNORED_COMMANDS[] = {
            "displaystyle", "limits", "nolimits", "scriptstyle", "scriptscriptstyle", "textstyle",
    };

    constexpr string_view SIZE_COMMANDS[] = {
            "big", "Big", "bigg", "Bigg", "bigl", "Bigl", "biggl", "Biggl", "bigr", "Bigr", "biggr", "Biggr",
    };

    constexpr string_view PLAIN_WRAPPERS[] = {
            "emph",   "mathcal", "mathbf", "mathfrak",   "mathit",     "mathrm",     "mathnormal", "mathscr",
            "mathsf", "mathtt",  "mathup", "mbox",       "overbrace",  "pmb",        "smash",      "substack",
            "text",   "textbf",  "textit", "textmd",     "textnormal", "textrm",     "textsc",     "textsf",
            "textsl", "texttt",  "textup", "underbrace", "bm",         "boldsymbol",
    };

    const symbol_entry ACCENTS[] = {
            {"acute", "\xCC\x81"},
            {"bar", "\xCC\x85"},
            {"breve", "\xCC\x86"},
            {"check", "\xCC\x8C"},
            {"ddot", "\xCC\x88"},
            {"dot", "\xCC\x87"},
            {"grave", "\xCC\x80"},
            {"hat", "\xCC\x82"},
            {"mathring", "\xCC\x8A"},
            {"overleftarrow", "\xE2\x83\x96"},
            {"overleftrightarrow", "\xE2\x83\xA1"},
            {"overline", "\xCC\x85"},
            {"overrightarrow", "\xE2\x83\x97"},
            {"tilde", "\xCC\x83"},
            {"underline", "\xCC\xB2"},
            {"vec", "\xE2\x83\x97"},
            {"widehat", "\xCC\x82"},
            {"widetilde", "\xCC\x83"},
    };

    constexpr string_view PMATRIX_FENCE[] = {"\xE2\x8E\x9B", "\xE2\x8E\x9E", "\xE2\x8E\x9C",
                                             "\xE2\x8E\x9F", "\xE2\x8E\x9D", "\xE2\x8E\xA0"};
    constexpr string_view BMATRIX_FENCE[] = {"\xE2\x8E\xA1", "\xE2\x8E\xA4", "\xE2\x8E\xA2",
                                             "\xE2\x8E\xA5", "\xE2\x8E\xA3", "\xE2\x8E\xA6"};
    constexpr string_view BMATRIX_CURLY_FENCE[] = {"\xE2\x8E\xA7", "\xE2\x8E\xAB", "\xE2\x8E\xA8",
                                                   "\xE2\x8E\xAC", "\xE2\x8E\xA9", "\xE2\x8E\xAD"};
    constexpr string_view VMATRIX_FENCE[] = {"\xE2\x94\x82", "\xE2\x94\x82", "\xE2\x94\x82",
                                             "\xE2\x94\x82", "\xE2\x94\x82", "\xE2\x94\x82"};
    constexpr string_view VVMATRIX_FENCE[] = {"\xE2\x95\x91", "\xE2\x95\x91", "\xE2\x95\x91",
                                              "\xE2\x95\x91", "\xE2\x95\x91", "\xE2\x95\x91"};

    template <size_t N>
    string_view lookup_symbol(const symbol_entry (&table)[N], const string_view name) noexcept {
        for (size_t i = 0; i < N; ++i) {
            if (name == table[i].name) {
                return table[i].value;
            }
        }
        return nullptr;
    }

    template <size_t N>
    string_view lookup_character(const char_entry (&table)[N], const char key) noexcept {
        for (size_t i = 0; i < N; ++i) {
            if (table[i].key == key) {
                return table[i].value;
            }
        }
        return nullptr;
    }

    template <size_t N>
    bool contains_name(const string_view (&names)[N], const string_view name) noexcept {
        for (size_t i = 0; i < N; ++i) {
            if (name == names[i]) {
                return true;
            }
        }
        return false;
    }

    string normalize_script_value(const string_view value) {
        const string trimmed = trim(value);
        string result;
        result.reserve(trimmed.size());
        size_t position = 0;
        while (position < trimmed.size()) {
            const char character = trimmed[position];
            if (character == '=' || character == '+' || character == '-') {
                trim_end_in_place(result);
                result += character;
                ++position;
                while (position < trimmed.size() && is_space_at(trimmed.view(), position)) {
                    position = next_codepoint(trimmed.view(), position);
                }
                continue;
            }
            const size_t next = next_codepoint(trimmed.view(), position);
            result.append(trimmed.view(position, next - position));
            position = next;
        }
        return result;
    }

    optional<string> format_unicode_script(const string_view value, const bool subscript) {
        const string normalized = normalize_script_value(value);
        string result;
        result.reserve(normalized.size());
        size_t position = 0;
        while (position < normalized.size()) {
            const size_t next = next_codepoint(normalized.view(), position);
            if (next - position != 1) {
                return none;
            }
            const string_view replacement = subscript ? lookup_character(SUBSCRIPTS, normalized[position])
                                                      : lookup_character(SUPERSCRIPTS, normalized[position]);
            if (replacement.empty()) {
                return none;
            }
            result += replacement;
            position = next;
        }
        return result;
    }

    string format_script(const string_view value, const bool subscript) {
        const string normalized = normalize_script_value(value);
        const optional<string> unicode = format_unicode_script(normalized.view(), subscript);
        if (unicode.has_value()) {
            return *unicode;
        }
        string result;
        result += subscript ? '_' : '^';
        if (codepoint_count(normalized.view()) == 1 || (subscript && is_all_ascii_letters(normalized.view()))) {
            result += normalized;
        } else {
            result += '(';
            result += normalized;
            result += ')';
        }
        return result;
    }

    string format_fraction(const string_view numerator_value, const string_view denominator_value) {
        const string numerator = trim(numerator_value);
        const string denominator = trim(denominator_value);
        const bool simple_numerator = is_letter_number_or_dot(numerator.view());
        const bool simple_denominator =
                is_number_or_dot(denominator.view()) || codepoint_count(denominator.view()) == 1;
        string result;
        result += simple_numerator ? numerator : "(" + numerator + ")";
        result += '/';
        result += simple_denominator ? denominator : "(" + denominator + ")";
        return result;
    }

    string format_root(const string_view value, const string_view symbol = "\xE2\x88\x9A") {
        const string trimmed = trim(value);
        string result(symbol);
        if (is_letter_number_or_dot(trimmed.view())) {
            result += trimmed;
        } else {
            result += '(';
            result += trimmed;
            result += ')';
        }
        return result;
    }

    bool named_operator_left_context(const uint32_t value) noexcept {
        return is_unicode_letter(value) || is_unicode_number(value) || value == ')' || value == ']' || value == '}' ||
               value == CP_LAYOUT_MARKER_END;
    }

    bool named_operator_right_context(const uint32_t value) noexcept {
        return is_unicode_letter(value) || is_unicode_number(value) || value == 0x221AU ||
               value == CP_LAYOUT_MARKER_START;
    }

    string collapse_line(const string_view line) {
        string result;
        result.reserve(line.size());
        bool pending_space = false;
        for (const char character: line) {
            if (character == ' ' || character == '\t') {
                if (!result.empty()) {
                    pending_space = true;
                }
                continue;
            }
            if (pending_space) {
                result += ' ';
                pending_space = false;
            }
            result += character;
        }
        return trim(result.view());
    }

    string normalize_output(const string_view value) {
        // The reference applies the left-spacing replacement and
        // removes the named-operator start markers before looking for right-spacing,
        // so a marker removed in the first pass no longer blocks a following space.
        string marked;
        marked.reserve(value.size());
        uint32_t previous = 0;
        size_t position = 0;
        while (position < value.size()) {
            if (value.substr(position).starts_with(NAMED_OPERATOR_START)) {
                if (previous != 0 && named_operator_left_context(previous)) {
                    marked += ' ';
                }
                previous = CP_NAMED_OPERATOR_START;
                position += MARKER_BYTES;
                continue;
            }
            const size_t next = next_codepoint(value, position);
            marked.append(value.substr(position, next - position));
            previous = decode_codepoint(value, position);
            position = next;
        }

        string expanded;
        expanded.reserve(marked.size());
        position = 0;
        while (position < marked.size()) {
            if (marked.substr(position).starts_with(NAMED_OPERATOR_END)) {
                const size_t next_position = position + MARKER_BYTES;
                if (next_position < marked.size() &&
                    named_operator_right_context(decode_codepoint(marked.view(), next_position))) {
                    expanded += ' ';
                }
                position = next_position;
                continue;
            }
            const size_t next = next_codepoint(marked.view(), position);
            expanded.append(marked.view(position, next - position));
            position = next;
        }

        vector<string> lines;
        size_t start = 0;
        for (size_t i = 0; i <= expanded.size(); ++i) {
            if (i == expanded.size() || expanded[i] == '\n') {
                lines.push_back(collapse_line(expanded.view().substr(start, i - start)));
                start = i + 1;
            }
        }
        size_t begin = 0;
        size_t end = lines.size();
        while (begin < end && lines[begin].empty()) {
            ++begin;
        }
        while (end > begin && lines[end - 1].empty()) {
            --end;
        }
        string result;
        for (size_t i = begin; i < end; ++i) {
            if (i > begin) {
                result += '\n';
            }
            result += lines[i];
        }
        return trim(result.view());
    }

    string replace_protected_spaces(const string_view value) {
        string result;
        result.reserve(value.size());
        size_t position = 0;
        while (position < value.size()) {
            if (value.substr(position).starts_with(PROTECTED_SPACE)) {
                result += ' ';
                position += MARKER_BYTES;
                continue;
            }
            const size_t next = next_codepoint(value, position);
            result.append(value.substr(position, next - position));
            position = next;
        }
        return result;
    }

    vector<string> split_lines(const string_view value) {
        vector<string> lines;
        size_t start = 0;
        for (size_t i = 0; i <= value.size(); ++i) {
            if (i == value.size() || value[i] == '\n') {
                lines.push_back(string(value.substr(start, i - start)));
                start = i + 1;
            }
        }
        return lines;
    }

    string layout_marker(const size_t index) {
        string result(LAYOUT_MARKER_START);
        result += to_string(static_cast<int64_t>(index));
        result += LAYOUT_MARKER_END;
        return result;
    }

    bool find_layout_marker(const string_view line, const size_t from, size_t& marker_start, size_t& marker_end,
                            size_t& node_index) noexcept {
        size_t search = from;
        while (search < line.size()) {
            const size_t start = line.find(LAYOUT_MARKER_START, search);
            if (start == string_view::npos) {
                return false;
            }
            size_t cursor = start + MARKER_BYTES;
            const size_t digits_start = cursor;
            size_t index = 0;
            while (cursor < line.size() && is_digit(line[cursor])) {
                if (index <= (static_cast<size_t>(-1) - 9) / 10) {
                    index = index * 10 + static_cast<size_t>(line[cursor] - '0');
                }
                ++cursor;
            }
            if (cursor > digits_start && line.substr(cursor).starts_with(LAYOUT_MARKER_END)) {
                marker_start = start;
                marker_end = cursor + MARKER_BYTES;
                node_index = index;
                return true;
            }
            search = start + 1;
        }
        return false;
    }

    bool trailing_layout_marker(const string_view text, size_t& node_index) noexcept {
        if (!text.ends_with(LAYOUT_MARKER_END) || text.size() < MARKER_BYTES * 2) {
            return false;
        }
        const size_t digits_end = text.size() - MARKER_BYTES;
        size_t digits_start = digits_end;
        while (digits_start > 0 && is_digit(text[digits_start - 1])) {
            --digits_start;
        }
        if (digits_start == digits_end || digits_start < MARKER_BYTES) {
            return false;
        }
        const size_t marker_start = digits_start - MARKER_BYTES;
        if (!text.substr(marker_start).starts_with(LAYOUT_MARKER_START)) {
            return false;
        }
        size_t index = 0;
        for (size_t i = digits_start; i < digits_end; ++i) {
            if (index > (static_cast<size_t>(-1) - 9) / 10) {
                return false;
            }
            index = index * 10 + static_cast<size_t>(text[i] - '0');
        }
        node_index = index;
        return true;
    }

    string pad_layout_line(const string_view line, const int width, const bool centered = false) {
        const int padding = max(0, width - visible_width(line));
        const int left = centered ? padding / 2 : 0;
        string result;
        result += spaces(left);
        result += line;
        result += spaces(padding - left);
        return result;
    }

    layout join_layouts(const vector<layout>& layouts) {
        if (layouts.empty()) {
            vector<string> empty_line;
            empty_line.push_back(string());
            return layout(move(empty_line), 0, 0);
        }
        int baseline = 0;
        int below = 0;
        for (const auto& item: layouts) {
            baseline = max(baseline, item.baseline);
            below = max(below, static_cast<int>(item.lines.size()) - item.baseline - 1);
        }
        vector<string> lines;
        for (int row = 0; row <= baseline + below; ++row) {
            string line;
            for (const auto& item: layouts) {
                const int source_row = row - baseline + item.baseline;
                if (source_row >= 0 && compare_less(source_row, item.lines.size())) {
                    line += pad_layout_line(item.lines[static_cast<size_t>(source_row)].view(), item.width);
                } else {
                    line += spaces(item.width);
                }
            }
            line = trim_end(line.view());
            lines.push_back(move(line));
        }
        int width = 0;
        for (const auto& item: layouts) {
            width += item.width;
        }
        return layout(move(lines), width, baseline);
    }

    layout render_layout(const string_view source, const vector<layout_node>& nodes) {
        vector<string> rendered_lines;
        int first_baseline = 0;
        size_t line_start = 0;
        for (size_t line_end = 0; line_end <= source.size(); ++line_end) {
            if (line_end != source.size() && source[line_end] != '\n') {
                continue;
            }
            const string_view source_line = source.substr(line_start, line_end - line_start);
            line_start = line_end + 1;

            vector<layout> layouts;
            size_t position = 0;
            const layout_node* previous = nullptr;
            size_t search = 0;
            size_t marker_start = 0;
            size_t marker_end = 0;
            size_t node_index = 0;
            while (find_layout_marker(source_line, search, marker_start, marker_end, node_index)) {
                search = marker_end;
                if (node_index >= nodes.size()) {
                    continue;
                }
                const layout_node& node = nodes[node_index];
                if (marker_start > position) {
                    const string_view sliced = source_line.substr(position, marker_start - position);
                    string trimmed = previous != nullptr ? trim_start(sliced) : string(sliced);
                    trimmed = trim_end(trimmed.view());
                    const bool preserve_leading = previous != nullptr && previous->type == layout_node::kind::matrix &&
                                                  !sliced.empty() && is_space_at(sliced, 0);
                    const bool preserve_trailing = node.type == layout_node::kind::matrix && !sliced.empty() &&
                                                   trim_end_index(sliced) != sliced.size();
                    string text;
                    if (!trimmed.empty()) {
                        if (preserve_leading) {
                            text += ' ';
                        }
                        text += trimmed;
                        if (preserve_trailing) {
                            text += ' ';
                        }
                    } else if (preserve_leading || preserve_trailing) {
                        text = " ";
                    }
                    const int text_width = visible_width(text.view());
                    vector<string> segment;
                    segment.push_back(move(text));
                    layouts.push_back(layout(move(segment), text_width, 0));
                }
                if (node.type == layout_node::kind::fraction) {
                    const layout numerator = render_layout(node.numerator.view(), nodes);
                    const layout denominator = render_layout(node.denominator.view(), nodes);
                    const int content_width = max(max(numerator.width, denominator.width), 1);
                    const int width = content_width + 2;
                    vector<string> lines;
                    for (const auto& line: numerator.lines) {
                        lines.push_back(pad_layout_line(line.view(), width, true));
                    }
                    string bar = " ";
                    bar += repeat_text("\xE2\x94\x80", static_cast<size_t>(content_width));
                    bar += ' ';
                    lines.push_back(move(bar));
                    for (const auto& line: denominator.lines) {
                        lines.push_back(pad_layout_line(line.view(), width, true));
                    }
                    layouts.push_back(layout(move(lines), width, static_cast<int>(numerator.lines.size())));
                } else if (node.type == layout_node::kind::op) {
                    int content_width = visible_width(node.op.view());
                    if (node.lower.has_value()) {
                        content_width = max(content_width, visible_width(node.lower->view()));
                    }
                    if (node.upper.has_value()) {
                        content_width = max(content_width, visible_width(node.upper->view()));
                    }
                    vector<string> lines;
                    if (node.upper.has_value()) {
                        string line = pad_layout_line(node.upper->view(), content_width, true);
                        line += ' ';
                        lines.push_back(move(line));
                    }
                    {
                        string line = pad_layout_line(node.op.view(), content_width, true);
                        line += ' ';
                        lines.push_back(move(line));
                    }
                    if (node.lower.has_value()) {
                        string line = pad_layout_line(node.lower->view(), content_width, true);
                        line += ' ';
                        lines.push_back(move(line));
                    }
                    layouts.push_back(layout(move(lines), content_width + 1, node.upper.has_value() ? 1 : 0));
                } else if (node.type == layout_node::kind::script) {
                    optional<layout> upper_layout;
                    if (node.upper.has_value()) {
                        upper_layout = render_layout(node.upper->view(), nodes);
                    }
                    optional<layout> lower_layout;
                    if (node.lower.has_value()) {
                        lower_layout = render_layout(node.lower->view(), nodes);
                    }
                    int width = 0;
                    if (upper_layout.has_value()) {
                        width = max(width, upper_layout->width);
                    }
                    if (lower_layout.has_value()) {
                        width = max(width, lower_layout->width);
                    }
                    vector<string> lines;
                    if (upper_layout.has_value()) {
                        for (const auto& line: upper_layout->lines) {
                            lines.push_back(pad_layout_line(line.view(), width));
                        }
                    }
                    lines.push_back(spaces(width));
                    if (lower_layout.has_value()) {
                        for (const auto& line: lower_layout->lines) {
                            lines.push_back(pad_layout_line(line.view(), width));
                        }
                    }
                    const int baseline = upper_layout.has_value() ? static_cast<int>(upper_layout->lines.size()) : 0;
                    layouts.push_back(layout(move(lines), width, baseline));
                } else {
                    int width = 0;
                    for (const auto& line: node.lines) {
                        width = max(width, visible_width(line.view()));
                    }
                    vector<string> lines;
                    for (const auto& line: node.lines) {
                        lines.push_back(pad_layout_line(line.view(), width));
                    }
                    layouts.push_back(layout(move(lines), width, static_cast<int>(node.baseline)));
                }
                position = marker_end;
                previous = &node;
            }
            if (position < source_line.size()) {
                const string_view sliced = source_line.substr(position);
                string trimmed = previous != nullptr ? trim_start(sliced) : string(sliced);
                string text;
                if (previous != nullptr && previous->type == layout_node::kind::matrix && !sliced.empty() &&
                    is_space_at(sliced, 0)) {
                    text = " ";
                    text += trimmed;
                } else {
                    text = move(trimmed);
                }
                const int text_width = visible_width(text.view());
                vector<string> segment;
                segment.push_back(move(text));
                layouts.push_back(layout(move(segment), text_width, 0));
            }
            const layout line_layout = join_layouts(layouts);
            if (rendered_lines.empty()) {
                first_baseline = line_layout.baseline;
            }
            for (const auto& line: line_layout.lines) {
                rendered_lines.push_back(line);
            }
            if (line_end == source.size()) {
                break;
            }
        }
        int width = 0;
        for (const auto& line: rendered_lines) {
            width = max(width, visible_width(line.view()));
        }
        return layout(move(rendered_lines), width, first_baseline);
    }

    class latex_parser {
    public:
        latex_parser(const string_view source, vector<layout_node>& nodes, const bool display) :
        source_(source),
        nodes_(nodes),
        display_(display) {}

        optional<string> render() {
            const string rendered = parse_sequence(0);
            if (!supported_ || position_ != source_.size()) {
                return none;
            }
            return normalize_output(rendered.view());
        }

    private:
        static bool is_negative_space(const string& value) noexcept {
            return value.size() == 1 && value[0] == NEGATIVE_SPACE;
        }

        string parse_sequence(const char end_character = 0) {
            string result;
            while (position_ < source_.size()) {
                const char character = source_[position_];
                if (end_character != 0 && character == end_character) {
                    ++position_;
                    return result;
                }
                if (character == '}') {
                    supported_ = false;
                    return result;
                }
                if (character == '{') {
                    ++position_;
                    result += parse_sequence('}');
                    continue;
                }
                if (character == '\\') {
                    const string command = parse_command();
                    if (is_negative_space(command)) {
                        trim_end_in_place(result);
                        if (result.ends_with(NAMED_OPERATOR_END)) {
                            result.erase(result.size() - MARKER_BYTES);
                        }
                    } else {
                        result += command;
                    }
                    continue;
                }
                if (character == '^' || character == '_') {
                    ++position_;
                    trim_end_in_place(result);
                    const string script = parse_scripts(character);
                    if (result.ends_with(NAMED_OPERATOR_END)) {
                        result.erase(result.size() - MARKER_BYTES);
                        result += script;
                        result += NAMED_OPERATOR_END;
                    } else {
                        result += script;
                    }
                    continue;
                }
                if (is_space_at(source_.view(), position_)) {
                    result += parse_whitespace();
                    continue;
                }
                if (character == '=' || character == '<' || character == '>') {
                    trim_end_in_place(result);
                    result += ' ';
                    result += character;
                    result += ' ';
                    ++position_;
                    continue;
                }
                if (character == '&') {
                    ++position_;
                    continue;
                }
                if (character == '~') {
                    ++position_;
                    result += ' ';
                    continue;
                }
                if (character == '.') {
                    size_t index = 0;
                    if (trailing_layout_marker(result.view(), index) && index < nodes_.size() &&
                        nodes_[index].type == layout_node::kind::matrix && !nodes_[index].lines.empty()) {
                        nodes_[index].lines.back() += '.';
                        ++position_;
                        continue;
                    }
                }
                result += character;
                ++position_;
            }
            if (end_character != 0) {
                supported_ = false;
            }
            return result;
        }

        string parse_scripts(const char initial_marker) {
            optional<string> sub;
            optional<string> upper;
            vector<char> order;
            auto parse_one = [this, &sub, &upper, &order](const char marker) {
                ++script_depth_;
                const string value = parse_required_argument(false);
                --script_depth_;
                if (marker == '_') {
                    sub = value;
                } else {
                    upper = value;
                }
                order.push_back(marker);
            };

            parse_one(initial_marker);
            const size_t next_position = skip_space(source_.view(), position_);
            if (next_position < source_.size()) {
                const char next_marker = source_[next_position];
                if ((next_marker == '^' || next_marker == '_') && next_marker != initial_marker) {
                    position_ = next_position + 1;
                    parse_one(next_marker);
                }
            }

            optional<string> sub_unicode;
            if (sub.has_value()) {
                sub_unicode = format_unicode_script(sub->view(), true);
            }
            optional<string> upper_unicode;
            if (upper.has_value()) {
                upper_unicode = format_unicode_script(upper->view(), false);
            }

            bool can_use_layout = true;
            if (sub.has_value() && !script_layout_allowed(sub->view())) {
                can_use_layout = false;
            }
            if (upper.has_value() && !script_layout_allowed(upper->view())) {
                can_use_layout = false;
            }
            const bool needs_layout = display_ && can_use_layout &&
                                      (script_depth_ > 0 || (sub.has_value() && !sub_unicode.has_value()) ||
                                       (upper.has_value() && !upper_unicode.has_value()));
            if (!needs_layout) {
                string output;
                for (const char marker: order) {
                    if (marker == '_') {
                        if (sub_unicode.has_value()) {
                            output += *sub_unicode;
                        } else {
                            output += format_script(sub.has_value() ? sub->view() : string_view(), true);
                        }
                    } else {
                        if (upper_unicode.has_value()) {
                            output += *upper_unicode;
                        } else {
                            output += format_script(upper.has_value() ? upper->view() : string_view(), false);
                        }
                    }
                }
                return output;
            }

            layout_node node;
            node.type = layout_node::kind::script;
            if (sub.has_value()) {
                node.lower = normalize_output(sub->view());
            }
            if (upper.has_value()) {
                node.upper = normalize_output(upper->view());
            }
            nodes_.push_back(move(node));
            return layout_marker(nodes_.size() - 1);
        }

        static bool script_layout_allowed(const string_view value) noexcept {
            if (value.contains('/')) {
                return false;
            }
            if (!value.contains(LAYOUT_MARKER_START) && codepoint_count(value) > 1 && !contains_upper_or_star(value)) {
                return false;
            }
            return true;
        }

        string parse_whitespace() {
            position_ = skip_space(source_.view(), position_);
            return " ";
        }

        string parse_command() {
            ++position_;
            if (position_ >= source_.size()) {
                supported_ = false;
                return {};
            }

            string command;
            const char first = source_[position_];
            if (first == '\n' || first == '\r') {
                ++position_;
                if (first == '\r' && position_ < source_.size() && source_[position_] == '\n') {
                    ++position_;
                }
                return " ";
            }
            if (is_alpha(first)) {
                const size_t start = position_;
                while (position_ < source_.size() && is_alpha(source_[position_])) {
                    ++position_;
                }
                command = source_.substr(start, position_ - start);
            } else {
                const size_t next = next_codepoint(source_.view(), position_);
                command = source_.substr(position_, next - position_);
                position_ = next;
            }

            if (command == "\\") {
                return "\n";
            }
            if (contains_name(SPACING_COMMANDS, command.view())) {
                return " ";
            }
            if (contains_name(NEGATIVE_SPACING_COMMANDS, command.view())) {
                return string(1, NEGATIVE_SPACE);
            }
            if (contains_name(FONT_SWITCH_COMMANDS, command.view())) {
                position_ = skip_space(source_.view(), position_);
                return {};
            }
            if (contains_name(IGNORED_COMMANDS, command.view())) {
                return {};
            }
            if (command == "{" || command == "}" || command == "$" || command == "%" || command == "#" ||
                command == "_" || command == "&") {
                return command;
            }
            if (command == "|") {
                return "\xE2\x80\x96";
            }
            if (command == "not") {
                const string value = trim(parse_required_argument(false).view());
                const string_view negated = lookup_symbol(NEGATED_SYMBOLS, value.view());
                if (!negated.empty()) {
                    string result = " ";
                    result += negated;
                    result += ' ';
                    return result;
                }
                if (value.empty()) {
                    supported_ = false;
                    return {};
                }
                const string leading_character = first_codepoint(value.view());
                string result = " ";
                result += leading_character;
                result += "\xCC\xB8";
                result += value.substr(leading_character.size());
                result += ' ';
                return result;
            }
            if (contains_name(LIMIT_OPERATORS, command.view())) {
                return parse_operator(command.view(), true, true, true);
            }

            const string_view symbol = lookup_symbol(SYMBOLS, command.view());
            if (!symbol.empty()) {
                if (contains_name(DISPLAY_LIMIT_SYMBOLS, command.view())) {
                    return parse_operator(symbol, false, true);
                }
                if (command == "cdot" || command == "times" || contains_name(RELATION_COMMANDS, command.view())) {
                    string result = " ";
                    result += symbol;
                    result += ' ';
                    return result;
                }
                return symbol;
            }
            if (contains_name(NAMED_OPERATORS, command.view())) {
                string result(NAMED_OPERATOR_START);
                result += command;
                result += NAMED_OPERATOR_END;
                return result;
            }
            if (contains_name(SIZE_COMMANDS, command.view())) {
                return {};
            }
            if (command == "left" || command == "middle" || command == "right") {
                if (position_ < source_.size() && source_[position_] == '.') {
                    ++position_;
                }
                return {};
            }
            if (command == "frac" || command == "dfrac" || command == "tfrac") {
                const bool should_stack = display_ && stack_fractions_ && command != "tfrac";
                const string numerator = parse_required_argument(!should_stack);
                const string denominator = parse_required_argument(!should_stack);
                if (should_stack) {
                    layout_node node;
                    node.type = layout_node::kind::fraction;
                    node.numerator = normalize_output(numerator.view());
                    node.denominator = normalize_output(denominator.view());
                    nodes_.push_back(move(node));
                    return layout_marker(nodes_.size() - 1);
                }
                return format_fraction(numerator.view(), denominator.view());
            }
            if (command == "sqrt") {
                const optional<string> degree = parse_optional_argument();
                const string value = parse_required_argument();
                if (!degree.has_value()) {
                    return format_root(value.view());
                }
                const string trimmed_degree = trim(degree->view());
                if (trimmed_degree == "2") {
                    return format_root(value.view());
                }
                if (trimmed_degree == "3") {
                    return format_root(value.view(), "\xE2\x88\x9B");
                }
                if (trimmed_degree == "4") {
                    return format_root(value.view(), "\xE2\x88\x9C");
                }
                string result = format_script(trimmed_degree.view(), false);
                result += format_root(value.view());
                return result;
            }
            if (command == "boxed" || command == "fbox") {
                string result = "[";
                result += trim(parse_required_argument().view());
                result += ']';
                return result;
            }
            if (command == "binom" || command == "dbinom" || command == "tbinom") {
                const string upper_value = parse_required_argument();
                const string lower_value = parse_required_argument();
                string result = "(";
                result += upper_value;
                result += " choose ";
                result += lower_value;
                result += ')';
                return result;
            }

            const string_view accent = lookup_symbol(ACCENTS, command.view());
            if (!accent.empty()) {
                string value = parse_required_argument();
                if (codepoint_count(value.view()) == 1) {
                    string result = move(value);
                    result += accent;
                    return result;
                }
                string result = command;
                result += '(';
                result += move(value);
                result += ')';
                return result;
            }
            if (command == "mathbb") {
                const string value = parse_required_argument();
                string result;
                result.reserve(value.size());
                size_t value_position = 0;
                while (value_position < value.size()) {
                    const size_t next = next_codepoint(value.view(), value_position);
                    if (next - value_position == 1) {
                        const string_view mapped = lookup_character(BLACKBOARD, value[value_position]);
                        if (!mapped.empty()) {
                            result += mapped;
                            value_position = next;
                            continue;
                        }
                    }
                    result.append(value, value_position, next - value_position);
                    value_position = next;
                }
                return result;
            }
            if (command == "operatorname") {
                const bool starred = position_ < source_.size() && source_[position_] == '*';
                if (starred) {
                    ++position_;
                }
                const string op = trim(normalize_output(parse_required_argument().view()).view());
                return parse_operator(op.view(), true, starred, true);
            }
            if (command == "mod" || command == "bmod") {
                return " mod ";
            }
            if (command == "pmod" || command == "pod") {
                const string value = trim(parse_required_argument().view());
                string result = command == "pmod" ? " (mod " : " (";
                result += value;
                result += ')';
                return result;
            }
            if (command == "overset" || command == "stackrel") {
                const string upper_value = parse_required_argument();
                const string value = trim(parse_required_argument().view());
                return value + format_script(upper_value.view(), false);
            }
            if (command == "underset") {
                const string lower_value = parse_required_argument();
                const string value = trim(parse_required_argument().view());
                return value + format_script(lower_value.view(), true);
            }
            if (contains_name(PLAIN_WRAPPERS, command.view())) {
                string value = parse_required_argument();
                if (command.starts_with("text") || command == "mbox") {
                    return move(value);
                }
                return trim(value.view());
            }
            if (command == "begin") {
                return parse_environment();
            }
            if (command == "end") {
                supported_ = false;
                return {};
            }

            supported_ = false;
            return "\\" + command;
        }

        string parse_operator(const string_view op, const bool bracket_style, const bool display_limits,
                              const bool spaced = false) {
            bool use_display_limits = display_limits;
            size_t modifier_position = position_;
            while (modifier_position < source_.size() &&
                   (source_[modifier_position] == ' ' || source_[modifier_position] == '\t')) {
                ++modifier_position;
            }
            const string_view tail = source_.view().substr(modifier_position);
            bool matched = false;
            bool limits_modifier = false;
            size_t modifier_length = 0;
            if (tail.starts_with("\\limits")) {
                matched = true;
                limits_modifier = true;
                modifier_length = 7;
            } else if (tail.starts_with("\\nolimits")) {
                matched = true;
                modifier_length = 9;
            }
            if (matched) {
                const size_t after = modifier_position + modifier_length;
                if (after >= source_.size() || !is_alpha(source_[after])) {
                    use_display_limits = limits_modifier;
                    position_ = after;
                }
            }

            optional<string> lower;
            optional<string> upper;
            while (true) {
                size_t script_position = position_;
                while (script_position < source_.size() &&
                       (source_[script_position] == ' ' || source_[script_position] == '\t')) {
                    ++script_position;
                }
                if (script_position >= source_.size()) {
                    break;
                }
                const char kind = source_[script_position];
                if (kind != '_' && kind != '^') {
                    break;
                }
                position_ = script_position + 1;
                string value = normalize_output(parse_required_argument(false).view());
                remove_ascii_spaces(value);
                if (kind == '_') {
                    if (lower.has_value()) {
                        supported_ = false;
                    }
                    lower = move(value);
                } else {
                    if (upper.has_value()) {
                        supported_ = false;
                    }
                    upper = move(value);
                }
            }

            if (display_ && use_display_limits && (lower.has_value() || upper.has_value())) {
                layout_node node;
                node.type = layout_node::kind::op;
                node.op = string(op);
                node.lower = move(lower);
                node.upper = move(upper);
                nodes_.push_back(move(node));
                return layout_marker(nodes_.size() - 1);
            }

            string rendered(op);
            if (lower.has_value()) {
                if (bracket_style) {
                    rendered += '[';
                    rendered += *lower;
                    rendered += ']';
                } else {
                    rendered += format_script(lower->view(), true);
                }
            }
            if (upper.has_value()) {
                rendered += format_script(upper->view(), false);
            }
            if (!spaced) {
                return rendered;
            }
            string result = " ";
            result += rendered;
            result += ' ';
            return result;
        }

        string parse_required_argument(const bool stack_fractions = true) {
            const bool previous = stack_fractions_;
            stack_fractions_ = previous && stack_fractions;
            const string value = parse_required_argument_value();
            stack_fractions_ = previous;
            return value;
        }

        string parse_required_argument_value() {
            position_ = skip_space(source_.view(), position_);
            if (position_ >= source_.size()) {
                supported_ = false;
                return {};
            }
            if (source_[position_] == '{') {
                ++position_;
                return parse_sequence('}');
            }
            if (source_[position_] == '\\') {
                return parse_command();
            }
            const size_t next = next_codepoint(source_.view(), position_);
            string value = source_.substr(position_, next - position_);
            position_ = next;
            return value;
        }

        optional<string> parse_optional_argument() {
            position_ = skip_horizontal_space(source_.view(), position_);
            if (position_ >= source_.size() || source_[position_] != '[') {
                return none;
            }
            const size_t end = source_.find(']', position_ + 1);
            if (end == string::npos) {
                supported_ = false;
                return none;
            }
            const string value = source_.substr(position_ + 1, end - position_ - 1);
            position_ = end + 1;
            return render_nested(value.view());
        }

        optional<string> read_raw_group() {
            position_ = skip_horizontal_space(source_.view(), position_);
            if (position_ >= source_.size() || source_[position_] != '{') {
                supported_ = false;
                return none;
            }
            const size_t start = ++position_;
            size_t depth = 1;
            while (position_ < source_.size()) {
                const char character = source_[position_];
                if (character == '\\') {
                    ++position_;
                    if (position_ < source_.size()) {
                        position_ = next_codepoint(source_.view(), position_);
                    }
                    continue;
                }
                if (character == '{') {
                    ++depth;
                }
                if (character == '}') {
                    --depth;
                }
                if (depth == 0) {
                    string value = source_.substr(start, position_ - start);
                    ++position_;
                    return move(value);
                }
                ++position_;
            }
            supported_ = false;
            return none;
        }

        static vector<string> split_environment_rows(const string_view body) {
            vector<string> rows;
            size_t start = 0;
            size_t position = 0;
            while (position + 1 < body.size()) {
                if (body[position] != '\\' || body[position + 1] != '\\') {
                    ++position;
                    continue;
                }
                size_t separator_end = position + 2;
                if (separator_end < body.size() && body[separator_end] == '[') {
                    size_t cursor = separator_end + 1;
                    bool closed = false;
                    while (cursor < body.size()) {
                        if (body[cursor] == ']') {
                            closed = true;
                            break;
                        }
                        if (body[cursor] == '\n') {
                            break;
                        }
                        ++cursor;
                    }
                    if (closed) {
                        separator_end = cursor + 1;
                    }
                }
                rows.push_back(string(body.substr(start, position - start)));
                start = separator_end;
                position = separator_end;
            }
            rows.push_back(string(body.substr(start)));
            return rows;
        }

        static vector<string> split_cells(const string_view row) {
            vector<string> cells;
            size_t start = 0;
            for (size_t i = 0; i <= row.size(); ++i) {
                if (i == row.size() || row[i] == '&') {
                    cells.push_back(string(row.substr(start, i - start)));
                    start = i + 1;
                }
            }
            return cells;
        }

        static string strip_column_spec(const string_view body) {
            const size_t begin = skip_space(body, 0);
            if (begin < body.size() && body[begin] == '{') {
                const size_t end = body.find('}', begin + 1);
                if (end != string_view::npos) {
                    return string(body.substr(end + 1));
                }
            }
            return string(body);
        }

        static string remove_trailing_comma(const string_view value) {
            size_t end = value.size();
            while (end > 0) {
                size_t start = end - 1;
                while (start > 0 && (static_cast<unsigned char>(value[start]) & 0xC0U) == 0x80U) {
                    --start;
                }
                if (!is_js_space(decode_codepoint(value, start))) {
                    break;
                }
                end = start;
            }
            if (end > 0 && value[end - 1] == ',') {
                return string(value.substr(0, end - 1));
            }
            return string(value);
        }

        static bool starts_with_condition_word(const string_view condition) noexcept {
            constexpr string_view words[] = {"if", "when", "for", "otherwise"};

            for (const string_view word: words) {
                if (condition.size() < word.size()) {
                    continue;
                }
                bool equal = true;
                for (size_t i = 0; i < word.size(); ++i) {
                    char character = condition[i];
                    if (character >= 'A' && character <= 'Z') {
                        character = static_cast<char>(character - 'A' + 'a');
                    }
                    if (character != word[i]) {
                        equal = false;
                        break;
                    }
                }
                if (!equal) {
                    continue;
                }
                if (condition.size() == word.size() || !is_word(condition[word.size()])) {
                    return true;
                }
            }
            return false;
        }

        static bool is_alignment_environment(const string_view environment) noexcept {
            return environment == "aligned" || environment == "align" || environment == "align*" ||
                   environment == "alignedat" || environment == "alignat" || environment == "alignat*" ||
                   environment == "gather" || environment == "gathered" || environment == "multline" ||
                   environment == "multline*" || environment == "split";
        }

        static bool is_matrix_environment(const string_view environment) noexcept {
            return environment == "array" || environment == "matrix" || environment == "smallmatrix" ||
                   environment == "pmatrix" || environment == "bmatrix" || environment == "Bmatrix" ||
                   environment == "vmatrix" || environment == "Vmatrix";
        }

        string parse_environment() {
            const optional<string> environment_value = read_raw_group();
            if (!environment_value.has_value() || environment_value->empty()) {
                return {};
            }
            const string& environment = *environment_value;
            const string end_marker = "\\end{" + environment + "}";
            const size_t end = source_.find(end_marker, position_);
            if (end == string::npos) {
                supported_ = false;
                return {};
            }
            const string body = source_.substr(position_, end - position_);
            position_ = end + end_marker.size();

            if (environment == "equation" || environment == "equation*" || environment == "displaymath") {
                return trim(render_nested(body.view()).view());
            }

            if (is_alignment_environment(environment.view())) {
                const bool aligned_at =
                        environment == "alignedat" || environment == "alignat" || environment == "alignat*";
                const string aligned_body = aligned_at ? strip_column_spec(body.view()) : body;
                const vector<string> rows = split_environment_rows(aligned_body.view());
                string result;
                bool first = true;
                for (const auto& row: rows) {
                    const vector<string> cells = split_cells(row.view());
                    string source;
                    if (aligned_at) {
                        for (size_t i = 0; i < cells.size(); i += 2) {
                            if (i > 0) {
                                source += ' ';
                            }
                            source += cells[i];
                            if (i + 1 < cells.size()) {
                                source += cells[i + 1];
                            }
                        }
                    } else {
                        for (const auto& cell: cells) {
                            source += cell;
                        }
                    }
                    const string rendered = trim(render_nested(source.view()).view());
                    if (rendered.empty()) {
                        continue;
                    }
                    if (!first) {
                        result += '\n';
                    }
                    result += rendered;
                    first = false;
                }
                return result;
            }

            if (environment == "cases" || environment == "cases*") {
                return render_cases(body.view());
            }

            if (is_matrix_environment(environment.view())) {
                const string matrix_body = environment == "array" ? strip_column_spec(body.view()) : body;
                return render_matrix(environment.view(), matrix_body.view());
            }

            supported_ = false;
            return move(body);
        }

        string render_cases(const string_view body) {
            vector<vector<string>> rows;
            for (const auto& row: split_environment_rows(body)) {
                const vector<string> cells = split_cells(row.view());
                vector<string> rendered;
                bool any = false;
                for (const auto& cell: cells) {
                    string value = trim(render_nested(cell.view(), false).view());
                    if (!value.empty()) {
                        any = true;
                    }
                    rendered.push_back(move(value));
                }
                if (any) {
                    rows.push_back(move(rendered));
                }
            }

            int value_width = 0;
            for (const auto& row: rows) {
                const string value = row.empty() ? string() : remove_trailing_comma(row[0].view());
                value_width = max(value_width, visible_width(value.view()));
            }
            vector<string> contents;
            for (const auto& row: rows) {
                const string value = row.empty() ? string() : remove_trailing_comma(row[0].view());
                const string condition = row.size() > 1 ? row[1] : string();
                if (condition.empty()) {
                    contents.push_back(value);
                    continue;
                }
                string content = value;
                content += repeat_text(PROTECTED_SPACE, static_cast<size_t>(value_width - visible_width(value.view())));
                content += starts_with_condition_word(condition.view()) ? " " : " if ";
                content += condition;
                contents.push_back(move(content));
            }

            if (contents.empty()) {
                return {};
            }
            if (contents.size() == 1) {
                string result = "\xE2\x8E\xA7 ";
                result += contents[0];
                return result;
            }

            const size_t middle = contents.size() / 2;
            vector<optional<string>> visual_rows;
            visual_rows.reserve(contents.size() + 1);
            for (const auto& content: contents) {
                visual_rows.push_back(optional<string>(content));
            }
            if (contents.size() % 2 == 0) {
                visual_rows.insert(visual_rows.begin() + static_cast<ptrdiff_t>(middle), optional<string>());
            }
            vector<string> lines;
            for (size_t i = 0; i < visual_rows.size(); ++i) {
                const string_view delimiter =
                        i == 0 ? "\xE2\x8E\xA7" : (i == visual_rows.size() - 1 ? "\xE2\x8E\xA9" : "\xE2\x8E\xA8");
                if (!visual_rows[i].has_value()) {
                    lines.push_back(delimiter);
                } else {
                    string line(delimiter);
                    line += ' ';
                    line += *visual_rows[i];
                    lines.push_back(move(line));
                }
            }
            layout_node node;
            node.type = layout_node::kind::matrix;
            node.lines = move(lines);
            node.baseline = middle;
            nodes_.push_back(move(node));
            return layout_marker(nodes_.size() - 1);
        }

        string render_matrix(const string_view environment, const string_view body) {
            vector<vector<string>> matrix;
            for (const auto& row: split_environment_rows(body)) {
                const vector<string> cells = split_cells(row.view());
                vector<string> rendered;
                bool any = false;
                for (const auto& cell: cells) {
                    string value = trim(render_nested(cell.view(), false).view());
                    if (!value.empty()) {
                        any = true;
                    }
                    rendered.push_back(move(value));
                }
                if (any) {
                    matrix.push_back(move(rendered));
                }
            }

            size_t column_count = 0;
            for (const auto& row: matrix) {
                column_count = max(column_count, row.size());
            }
            vector<int> column_widths(column_count, 0);
            for (const auto& row: matrix) {
                for (size_t column = 0; column < row.size(); ++column) {
                    column_widths[column] = max(column_widths[column], visible_width(row[column].view()));
                }
            }
            vector<string> rows;
            for (const auto& row: matrix) {
                string line;
                for (size_t column = 0; column < column_count; ++column) {
                    if (column > 0) {
                        line += " \xE2\x94\x82 ";
                    }
                    const string& cell = column < row.size() ? row[column] : string();
                    line += cell;
                    line += repeat_text(
                            PROTECTED_SPACE,
                            static_cast<size_t>(max(0, column_widths[column] - visible_width(cell.view()))));
                }
                rows.push_back(move(line));
            }

            vector<string> lines;
            if (environment == "array" || environment == "matrix" || environment == "smallmatrix") {
                lines = move(rows);
            } else {
                const string_view* fence = nullptr;
                if (environment == "pmatrix") {
                    fence = PMATRIX_FENCE;
                } else if (environment == "bmatrix") {
                    fence = BMATRIX_FENCE;
                } else if (environment == "Bmatrix") {
                    fence = BMATRIX_CURLY_FENCE;
                } else if (environment == "vmatrix") {
                    fence = VMATRIX_FENCE;
                } else if (environment == "Vmatrix") {
                    fence = VVMATRIX_FENCE;
                } else {
                    supported_ = false;
                    return join_strings(rows);
                }
                for (size_t i = 0; i < rows.size(); ++i) {
                    string_view left = i == 0 ? fence[0] : (i == rows.size() - 1 ? fence[4] : fence[2]);
                    string_view right = i == 0 ? fence[1] : (i == rows.size() - 1 ? fence[5] : fence[3]);
                    string line(left);
                    line += ' ';
                    line += rows[i];
                    line += ' ';
                    line += right;
                    lines.push_back(move(line));
                }
            }

            if (lines.size() <= 1) {
                return lines.empty() ? string() : lines[0];
            }
            layout_node node;
            node.type = layout_node::kind::matrix;
            node.lines = move(lines);
            node.baseline = 0;
            nodes_.push_back(move(node));
            return layout_marker(nodes_.size() - 1);
        }

        static string join_strings(const vector<string>& values) {
            string result;
            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0) {
                    result += '\n';
                }
                result += values[i];
            }
            return result;
        }

        string render_nested(const string_view source, const bool stack_fractions = true) {
            latex_parser nested(source, nodes_, display_ && stack_fractions);
            const optional<string> rendered = nested.render();
            if (!rendered.has_value()) {
                supported_ = false;
                return string(source);
            }
            return *rendered;
        }

        string source_;
        vector<layout_node>& nodes_;
        bool display_ = false;
        size_t position_ = 0;
        bool supported_ = true;
        bool stack_fractions_ = true;
        int script_depth_ = 0;
    };
} // namespace


optional<latex_layout> render_latex(const string_view tex, const bool display) {
    vector<layout_node> nodes;
    latex_parser parser(tex, nodes, display);
    const optional<string> rendered = parser.render();
    if (!rendered.has_value()) {
        return none;
    }

    vector<string> lines;
    if (nodes.empty()) {
        lines = split_lines(replace_protected_spaces(rendered->view()).view());
    } else {
        const layout root = render_layout(rendered->view(), nodes);
        size_t indentation = string::npos;
        for (const auto& line: root.lines) {
            if (trim(line.view()).empty()) {
                continue;
            }
            indentation = min(indentation, trim_start_index(line.view()));
        }
        string joined;
        for (size_t i = 0; i < root.lines.size(); ++i) {
            if (i > 0) {
                joined += '\n';
            }
            const string& line = root.lines[i];
            string sliced = line.size() > indentation ? line.substr(indentation) : string();
            trim_end_in_place(sliced);
            joined += sliced;
        }
        trim_end_in_place(joined);
        lines = split_lines(replace_protected_spaces(joined.view()).view());
        // The reference trims before its protected spaces become ordinary blanks,
        // which can leave trailing blanks on matrix rows.
        // Trim again so the documented "no trailing whitespace" invariant holds.
        for (auto& line: lines) {
            trim_end_in_place(line);
        }
    }

    // Inline math is inserted into wrapped prose,
    // so a multi-line layout has no valid single-line form;
    // report failure and let the caller fall back.
    if (!display && lines.size() > 1) {
        return none;
    }

    latex_layout result;
    result.lines = move(lines);
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
