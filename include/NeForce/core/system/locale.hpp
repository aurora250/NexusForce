#ifndef NEFORCE_CORE_SYSTEM_LOCALE_HPP__
#define NEFORCE_CORE_SYSTEM_LOCALE_HPP__
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/container/vector.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include <locale.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

struct locale_exception final : system_exception {
    explicit locale_exception(const char* info = "Locale Operation Failed", const char* type = static_type,
                              const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit locale_exception(const exception& e) :
    system_exception(e) {}

    ~locale_exception() override = default;

    static constexpr auto static_type = "locale_exception";
};


class NEFORCE_API locale {
public:
    struct numeric_info {
        string decimal_point;
        string thousands_sep;
        string grouping;
    };

    struct monetary_info {
        string currency_symbol;
        string int_curr_symbol;
        string mon_decimal_point;
        string mon_thousands_sep;
        string mon_grouping;
        string positive_sign;
        string negative_sign;
        int frac_digits{2};
        int int_frac_digits{2};
        bool p_cs_precedes{true};
        bool n_cs_precedes{true};
    };

    struct time_info {
        string date_fmt;
        string time_fmt;
        string datetime_fmt;
        vector<string> day_names;
        vector<string> abbr_day_names;
        vector<string> month_names;
        vector<string> abbr_month_names;
        string am_str;
        string pm_str;
    };

    enum class collate_strength : int32_t {
        primary = 1,
        secondary = 2,
        tertiary = 3,
        identical = 4
    };

private:
    string name_;
    string encoding_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    string win_name_;
#else
    ::locale_t loc_;
    bool owns_;
#endif

    void load_locale(const string& name);

public:
    locale();
    explicit locale(const string& name);
    ~locale();

    locale(const locale&);
    locale& operator=(const locale&);
    locale(locale&&) noexcept;
    locale& operator=(locale&&) noexcept;

    static locale classic();
    static locale system();
    static locale from_name(const string& name);

    const string& name() const noexcept { return name_; }
    const string& encoding() const noexcept { return encoding_; }

    bool operator==(const locale& o) const noexcept { return name_ == o.name_; }
    bool operator!=(const locale& o) const noexcept { return !(*this == o); }

    numeric_info numeric() const;
    monetary_info monetary() const;
    time_info time() const;

    bool is_alpha(char32_t cp) const noexcept;
    bool is_digit(char32_t cp) const noexcept;
    bool is_alnum(char32_t cp) const noexcept;
    bool is_space(char32_t cp) const noexcept;
    bool is_upper(char32_t cp) const noexcept;
    bool is_lower(char32_t cp) const noexcept;
    bool is_punct(char32_t cp) const noexcept;
    bool is_print(char32_t cp) const noexcept;

    char32_t to_upper(char32_t cp) const noexcept;
    char32_t to_lower(char32_t cp) const noexcept;

    int compare(const string& a, const string& b, collate_strength strength = collate_strength::tertiary) const;
    string collation_key(const string& s) const;

    string to_multibyte(const u32string& ucs4) const;
    u32string to_ucs4(const string& mb) const;

    static vector<string> available_locales();
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_LOCALE_HPP__
