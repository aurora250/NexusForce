#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_TYPES_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_TYPES_SRC__
#include <NeForce/core/algorithm/sort.hpp>
#include <NeForce/core/string/string.hpp>
#include "engine_common.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct code_range {
    uint32_t first;
    uint32_t last;
};

struct char_class {
    bool negate = false;
    bool caseless = false;
    vector<code_range> ranges;

    void add(const uint32_t first, const uint32_t last) { ranges.push_back(code_range{first, last}); }

    void add_ascii_class(char kind);
    void add_categories(uint32_t mask);
    void add_script(int script);
    void normalize();
    NEFORCE_NODISCARD bool contains(uint32_t code_point) const noexcept;
};

struct node {
    enum class kind : uint8_t {
        empty,
        literal,
        any,
        char_class,
        concat,
        alternate,
        repeat,
        capture,
        group,
        look,
        atomic,
        keep,
        begin_search,
        conditional,
        backref,
        begin_line,
        end_line,
        begin_subject,
        end_subject,
        end_subject_final,
        word_boundary,
        non_word_boundary,
        cluster,
        call,
    };

    enum class repeat_mode : uint8_t {
        greedy = 0,
        lazy = 1,
        possessive = 2,
    };

    kind type = kind::empty;
    uint32_t code_point = 0;
    int32_t class_index = -1;
    int32_t capture = -1;
    int32_t minimum = 0;
    int32_t maximum = -1;
    repeat_mode mode = repeat_mode::greedy;
    bool caseless = false;
    bool dotall = false;
    bool multiline = false;
    bool ucp = false;
    int32_t look_kind = 0;
    int32_t group = 0;
    size_t offset = 0;
    vector<int32_t> children;
};

enum class op : uint8_t {
    fail = 0,
    match,
    accept,
    char_match,
    any,
    char_class,
    split,
    jump,
    save,
    begin_line,
    end_line,
    begin_subject,
    end_subject,
    end_subject_final,
    word_boundary,
    non_word_boundary,
    backref,
    look,
    atomic,
    call,
    cluster,
    keep,
    begin_search,
    branch_group,
    loop_enter,
    loop_check,
};

struct instruction {
    op code = op::fail;
    int32_t a = 0;
    int32_t b = 0;
    int32_t c = 0;
    int32_t d = 0;
    int32_t e = 0;
};

inline void char_class::normalize() {
    if (ranges.empty()) {
        return;
    }

    sort(ranges.begin(), ranges.end(),
         [](const code_range& left, const code_range& right) { return left.first < right.first; });

    size_t merged = 1;
    for (size_t index = 1; index < ranges.size(); ++index) {
        if (ranges[index].first <= ranges[merged - 1].last + 1) {
            ranges[merged - 1].last = max(ranges[index].last, ranges[merged - 1].last);
        } else {
            ranges[merged++] = ranges[index];
        }
    }

    ranges.resize(merged);
}

NEFORCE_NODISCARD inline bool char_class::contains(const uint32_t code_point) const noexcept {
    const auto search = [this](const uint32_t value) noexcept -> bool {
        size_t low = 0;
        size_t high = ranges.size();

        while (low < high) {
            const size_t middle = low + (high - low) / 2;

            if (value < ranges[middle].first) {
                high = middle;
            } else if (value > ranges[middle].last) {
                low = middle + 1;
            } else {
                return true;
            }
        }

        return false;
    };

    bool hit = search(code_point);

    if (!hit && caseless) {
        hit = search(to_lower(code_point)) || search(to_upper(code_point));
    }

    return hit != negate;
}

inline void char_class::add_ascii_class(const char kind) {
    switch (kind) {
        case 'd':
            add('0', '9');
            break;
        case 'w':
            add('a', 'z');
            add('A', 'Z');
            add('0', '9');
            add('_', '_');
            break;
        default:
            add(0x09, 0x0D);
            add(' ', ' ');
            break;
    }
}

inline void char_class::add_categories(const uint32_t mask) {
    for (const auto& entry: regex_ucd::ranges) {
        if ((mask & (1U << (entry.category - 1U))) != 0U) {
            add(entry.start, entry.end);
        }
    }
}

inline void char_class::add_script(const int script) {
    switch (script) {
        case 1: // Han
            add(0x2E80, 0x2EF3);
            add(0x2F00, 0x2FD5);
            add(0x3005, 0x3005);
            add(0x3007, 0x3007);
            add(0x3021, 0x3029);
            add(0x3038, 0x303B);
            add(0x3400, 0x4DBF);
            add(0x4E00, 0x9FFF);
            add(0xF900, 0xFAFF);
            add(0x20000, 0x2A6DF);
            add(0x2A700, 0x2EBEF);
            add(0x2F800, 0x2FA1F);
            add(0x30000, 0x3134F);
            break;
        case 2: // Hiragana
            add(0x3041, 0x309F);
            add(0x1B001, 0x1B11F);
            break;
        case 3: // Katakana
            add(0x30A1, 0x30FF);
            add(0x31F0, 0x31FF);
            add(0xFF66, 0xFF9D);
            add(0x1B000, 0x1B000);
            break;
        case 4: // Hangul
            add(0x1100, 0x11FF);
            add(0x3131, 0x318E);
            add(0xA960, 0xA97F);
            add(0xAC00, 0xD7A3);
            add(0xD7B0, 0xD7FF);
            break;
        default:
            break;
    }
}

enum : uint32_t {
    cat_lu = 1U << 0U,
    cat_ll = 1U << 1U,
    cat_lt = 1U << 2U,
    cat_lm = 1U << 3U,
    cat_lo = 1U << 4U,
    cat_mn = 1U << 5U,
    cat_mc = 1U << 6U,
    cat_me = 1U << 7U,
    cat_nd = 1U << 8U,
    cat_nl = 1U << 9U,
    cat_no = 1U << 10U,
    cat_pc = 1U << 11U,
    cat_pd = 1U << 12U,
    cat_ps = 1U << 13U,
    cat_pe = 1U << 14U,
    cat_pi = 1U << 15U,
    cat_pf = 1U << 16U,
    cat_po = 1U << 17U,
    cat_sm = 1U << 18U,
    cat_sc = 1U << 19U,
    cat_sk = 1U << 20U,
    cat_so = 1U << 21U,
    cat_zs = 1U << 22U,
    cat_zl = 1U << 23U,
    cat_zp = 1U << 24U,
    cat_cc = 1U << 25U,
    cat_cf = 1U << 26U,
    cat_co = 1U << 27U,
    cat_cs = 1U << 28U,
    cat_cn = 1U << 29U,
};

constexpr uint32_t cat_letters = cat_lu | cat_ll | cat_lt | cat_lm | cat_lo;
constexpr uint32_t cat_numbers = cat_nd | cat_nl | cat_no;
constexpr uint32_t cat_marks = cat_mn | cat_mc | cat_me;
constexpr uint32_t cat_punctuation = cat_pc | cat_pd | cat_ps | cat_pe | cat_pi | cat_pf | cat_po;
constexpr uint32_t cat_symbols = cat_sm | cat_sc | cat_sk | cat_so;
constexpr uint32_t cat_separators = cat_zs | cat_zl | cat_zp;
constexpr uint32_t cat_others = cat_cc | cat_cf | cat_co | cat_cs | cat_cn;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_TYPES_SRC__
