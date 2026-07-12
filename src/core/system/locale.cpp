#include <NeForce/core/system/locale.hpp>
#include <NeForce/core/time/datetime.hpp>
#include <unicode/uloc.h>
#include <unicode/uchar.h>
#include <unicode/ucol.h>
#include <unicode/ucnv.h>
#include <unicode/unum.h>
#include <unicode/udat.h>
#include <unicode/ulocdata.h>
#include <unicode/ucal.h>
#include <unicode/ustring.h>
#include <unicode/uenum.h>
#include <unicode/putil.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    void* open_converter(const char* locale_id) {
        ::UErrorCode status = ::U_ZERO_ERROR;
        ::UConverter* cnv = ::ucnv_open(locale_id, &status);
        if (::U_FAILURE(status) != 0) {
            status = ::U_ZERO_ERROR;
            cnv = ::ucnv_open("UTF-8", &status);
        }
        return cnv;
    }

    ::UDate datetime_to_udate(const datetime& dt) {
        const datetime utc_dt = dt.has_timezone() ? dt.to_UTC() : dt;
        const timestamp ts(utc_dt);
        return static_cast<UDate>(ts.value()) * 1000.0;
    }

    ::UDateFormatStyle to_udate_style(const locale::date_style ds) {
        switch (ds) {
            case locale::date_style::full:
                return ::UDAT_FULL;
            case locale::date_style::long_fmt:
                return ::UDAT_LONG;
            case locale::date_style::medium:
                return ::UDAT_MEDIUM;
            case locale::date_style::short_fmt:
                return ::UDAT_SHORT;
            case locale::date_style::none:
                return ::UDAT_NONE;
            case locale::date_style::relative:
                return ::UDAT_FULL;
        }
        return ::UDAT_MEDIUM;
    }

    ::UDateFormatStyle to_utime_style(const locale::time_style ts) {
        switch (ts) {
            case locale::time_style::full:
                return ::UDAT_FULL;
            case locale::time_style::long_fmt:
                return ::UDAT_LONG;
            case locale::time_style::medium:
                return ::UDAT_MEDIUM;
            case locale::time_style::short_fmt:
                return ::UDAT_SHORT;
            case locale::time_style::none:
                return ::UDAT_NONE;
            case locale::time_style::relative:
                return ::UDAT_FULL;
        }
        return ::UDAT_MEDIUM;
    }

    ::UColAttributeValue to_ucol_strength(const locale::collate_strength s) {
        switch (s) {
            case locale::collate_strength::primary:
                return ::UCOL_PRIMARY;
            case locale::collate_strength::secondary:
                return ::UCOL_SECONDARY;
            case locale::collate_strength::tertiary:
                return ::UCOL_TERTIARY;
            case locale::collate_strength::quaternary:
                return ::UCOL_QUATERNARY;
            case locale::collate_strength::identical:
                return ::UCOL_IDENTICAL;
        }
        return ::UCOL_TERTIARY;
    }

    string icu_to_string(const char* s) { return s != nullptr ? string(s) : string{}; }

    string icu_to_string(const ::UChar* s, const int32_t len) {
        if (s == nullptr) {
            return {};
        }
        const int32_t actual_len = (len > 0) ? len : ::u_strlen(s);
        if (actual_len <= 0) {
            return {};
        }
        string result(static_cast<size_t>(actual_len * 3), '\0');
        ::UErrorCode status = ::U_ZERO_ERROR;
        int32_t out_len = 0;
        ::u_strToUTF8(result.data(), static_cast<int32_t>(result.size()), &out_len, s, actual_len, &status);
        if ((::U_FAILURE(status) != 0) && status != ::U_BUFFER_OVERFLOW_ERROR) {
            return {};
        }
        result.resize(static_cast<size_t>(out_len));
        return result;
    }

    vector<string> enum_to_vector(::UEnumeration* en) {
        vector<string> result;
        if (en == nullptr) {
            return result;
        }
        ::UErrorCode status = ::U_ZERO_ERROR;
        const int32_t count = ::uenum_count(en, &status);
        if ((::U_SUCCESS(status) != 0) && count > 0) {
            result.reserve(static_cast<size_t>(count));
        }
        const char* item = nullptr;
        int32_t item_len = 0;
        while ((item = ::uenum_next(en, &item_len, &status)) != nullptr && (::U_SUCCESS(status) != 0)) {
            result.emplace_back(item, static_cast<size_t>(item_len));
        }
        ::uenum_close(en);
        return result;
    }

    void* open_number_formatter(const char* locale_id) {
        ::UErrorCode status = ::U_ZERO_ERROR;
        ::UNumberFormat* fmt = ::unum_open(::UNUM_DEFAULT, nullptr, 0, locale_id, nullptr, &status);
        if (::U_FAILURE(status) != 0) {
            return nullptr;
        }
        // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
        return static_cast<void*>(fmt);
    }

    void* open_currency_formatter(const char* locale_id, const char* iso_code) {
        ::UErrorCode status = ::U_ZERO_ERROR;
        ::UNumberFormat* fmt = ::unum_open(::UNUM_CURRENCY, nullptr, 0, locale_id, nullptr, &status);
        if (::U_FAILURE(status) != 0) {
            return nullptr;
        }
        ::UChar iso_uchar[4] = {};
        ::u_charsToUChars(iso_code, iso_uchar, 3);
        ::unum_setTextAttribute(fmt, ::UNUM_CURRENCY_CODE, iso_uchar, 3, &status);
        // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
        return static_cast<void*>(fmt);
    }

    void* open_date_pattern_formatter(const char* locale_id, const char* pattern) {
        ::UErrorCode status = U_ZERO_ERROR;
        const auto pattern_len = static_cast<int32_t>(string_length(pattern));
        vector<::UChar> upattern(static_cast<size_t>(pattern_len) + 1);
        int32_t actual_len = 0;
        ::UErrorCode cnv_status = U_ZERO_ERROR;
        ::u_strFromUTF8(upattern.data(), static_cast<int32_t>(upattern.size()), &actual_len, pattern, pattern_len,
                        &cnv_status);
        if (::U_FAILURE(cnv_status) != 0) {
            return nullptr;
        }

        ::UDateFormat* fmt =
                ::udat_open(::UDAT_IGNORE, ::UDAT_IGNORE, locale_id, nullptr, 0, upattern.data(), actual_len, &status);
        if (::U_FAILURE(status) != 0) {
            return nullptr;
        }
        // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
        return static_cast<void*>(fmt);
    }
} // anonymous namespace


void locale::load(const string& bcp47) {
    cleanup();

    const char* input_name = bcp47.data();
    if (bcp47.empty() || bcp47 == "C" || bcp47 == "POSIX") {
        input_name = "en-US-POSIX";
    }

    ::UErrorCode status = ::U_ZERO_ERROR;
    char canonical[ULOC_FULLNAME_CAPACITY] = {};
    ::uloc_canonicalize(input_name, canonical, sizeof(canonical), &status);
    if (::U_FAILURE(status) != 0) {
        throw locale_exception(string("locale: invalid locale name '"_s + bcp47 + "'").data());
    }

    icu_name_ = canonical;

    status = ::U_ZERO_ERROR;
    char _bcp47[ULOC_FULLNAME_CAPACITY] = {};
    ::uloc_toLanguageTag(canonical, _bcp47, sizeof(_bcp47), 0, &status);
    name_ = (::U_SUCCESS(status) != 0 && _bcp47[0] != '\0') ? _bcp47 : canonical;

    char buf[ULOC_FULLNAME_CAPACITY] = {};

    status = ::U_ZERO_ERROR;
    ::uloc_getLanguage(canonical, buf, sizeof(buf), &status);
    language_code_ = (::U_SUCCESS(status) != 0) ? buf : "";
    if (language_code_.empty()) {
        throw locale_exception(string("locale: invalid locale name '"_s + _bcp47 + "'").data());
    }

    status = ::U_ZERO_ERROR;
    ::uloc_getScript(canonical, buf, sizeof(buf), &status);
    script_code_ = (::U_SUCCESS(status) != 0) ? buf : "";

    status = ::U_ZERO_ERROR;
    ::uloc_getCountry(canonical, buf, sizeof(buf), &status);
    country_code_ = (::U_SUCCESS(status) != 0) ? buf : "";

    status = ::U_ZERO_ERROR;
    ::uloc_getVariant(canonical, buf, sizeof(buf), &status);
    variant_code_ = (::U_SUCCESS(status) != 0) ? buf : "";

    status = ::U_ZERO_ERROR;
    ::UCollator* col = ::ucol_open(canonical, &status);
    if (::U_SUCCESS(status) != 0) {
        collator_ = static_cast<void*>(col);
    }

    converter_ = open_converter(canonical);
    num_fmt_ = open_number_formatter(canonical);
}

void locale::cleanup() noexcept {
    if (collator_ != nullptr) {
        ::ucol_close(static_cast<::UCollator*>(collator_));
        collator_ = nullptr;
    }
    if (converter_ != nullptr) {
        ::ucnv_close(static_cast<::UConverter*>(converter_));
        converter_ = nullptr;
    }
    if (num_fmt_ != nullptr) {
        ::unum_close(static_cast<::UNumberFormat*>(num_fmt_));
        num_fmt_ = nullptr;
    }
    if (curr_fmt_ != nullptr) {
        ::unum_close(static_cast<::UNumberFormat*>(curr_fmt_));
        curr_fmt_ = nullptr;
    }
    if (date_fmt_ != nullptr) {
        ::udat_close(static_cast<::UDateFormat*>(date_fmt_));
        date_fmt_ = nullptr;
    }
}

locale::locale() { load("en-US-POSIX"); }

locale::locale(const string& name) { load(name); }

locale::~locale() { cleanup(); }

locale::locale(const locale& other) { load(other.name_); }

locale& locale::operator=(const locale& other) {
    if (addressof(other) == this) {
        return *this;
    }
    load(other.name_);
    return *this;
}

locale::locale(locale&& other) noexcept :
name_(move(other.name_)),
icu_name_(move(other.icu_name_)),
collator_(other.collator_),
converter_(other.converter_),
num_fmt_(other.num_fmt_),
curr_fmt_(other.curr_fmt_),
date_fmt_(other.date_fmt_),
language_code_(move(other.language_code_)),
script_code_(move(other.script_code_)),
country_code_(move(other.country_code_)),
variant_code_(move(other.variant_code_)) {
    other.collator_ = nullptr;
    other.converter_ = nullptr;
    other.num_fmt_ = nullptr;
    other.curr_fmt_ = nullptr;
    other.date_fmt_ = nullptr;
}

locale& locale::operator=(locale&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    cleanup();
    name_ = move(other.name_);
    icu_name_ = move(other.icu_name_);
    collator_ = other.collator_;
    converter_ = other.converter_;
    num_fmt_ = other.num_fmt_;
    curr_fmt_ = other.curr_fmt_;
    date_fmt_ = other.date_fmt_;
    language_code_ = move(other.language_code_);
    script_code_ = move(other.script_code_);
    country_code_ = move(other.country_code_);
    variant_code_ = move(other.variant_code_);
    other.collator_ = nullptr;
    other.converter_ = nullptr;
    other.num_fmt_ = nullptr;
    other.curr_fmt_ = nullptr;
    other.date_fmt_ = nullptr;
    return *this;
}

locale locale::classic() { return locale("en-US-POSIX"); }

locale locale::system() { return locale(icu_to_string(::uloc_getDefault())); }

locale locale::from_name(const string& name) { return locale(name); }

string locale::display_name(const locale& display_locale) const {
    ::UErrorCode status = ::U_ZERO_ERROR;
    ::UChar ubuf[256] = {};
    const int32_t result_len = ::uloc_getDisplayName(name_.data(), display_locale.name_.data(), ubuf, 256, &status);
    if (::U_SUCCESS(status) != 0) {
        return icu_to_string(ubuf, result_len);
    }
    return name_;
}

string locale::native_language_name() const { return display_name(*this); }

string locale::native_country_name() const {
    if (country_code_.empty()) {
        return {};
    }
    return display_name(*this);
}

locale::text_direction locale::direction() const {
    return (::uloc_isRightToLeft(name_.data()) != 0) ? text_direction::RTL : text_direction::LTR;
}

locale::measurement_system locale::measurement() const {
    ::UErrorCode status = ::U_ZERO_ERROR;
    const ::UMeasurementSystem ms = ::ulocdata_getMeasurementSystem(name_.data(), &status);
    if (::U_FAILURE(status) != 0) {
        return measurement_system::SI;
    }
    switch (ms) {
        case ::UMS_SI:
            return measurement_system::SI;
        case ::UMS_US:
            return measurement_system::US;
        case ::UMS_UK:
            return measurement_system::UK;
        default:
            return measurement_system::SI;
    }
}

int32_t locale::first_day_of_week() const {
    ::UErrorCode status = ::U_ZERO_ERROR;
    ::UCalendar* cal = ::ucal_open(nullptr, 0, name_.data(), ::UCAL_DEFAULT, &status);
    if (::U_FAILURE(status) != 0) {
        return 1;
    }
    const int32_t fdow = ::ucal_getAttribute(cal, ::UCAL_FIRST_DAY_OF_WEEK);
    ::ucal_close(cal);
    return fdow;
}

vector<string> locale::ui_languages() const {
    vector<string> result;
    if (name_.empty()) {
        return result;
    }

    result.push_back(name_);

    char parent[ULOC_FULLNAME_CAPACITY] = {};
    string current = icu_name_;
    while (true) {
        ::UErrorCode status = ::U_ZERO_ERROR;
        constexpr int32_t parent_len = sizeof(parent);
        ::uloc_getParent(current.data(), parent, parent_len, &status);
        if ((::U_FAILURE(status) != 0) || parent[0] == '\0' || string_compare(parent, "root") == 0) {
            break;
        }
        char bcp47[ULOC_FULLNAME_CAPACITY] = {};
        status = ::U_ZERO_ERROR;
        ::uloc_toLanguageTag(parent, bcp47, sizeof(bcp47), 0, &status);
        string parent_bcp47 = (::U_SUCCESS(status) != 0 && bcp47[0] != '\0') ? bcp47 : parent;
        if (find(result.begin(), result.end(), parent_bcp47) == result.end()) {
            result.push_back(move(parent_bcp47));
        }
        current = parent;
    }

    return result;
}

bool locale::is_alpha(const char32_t cp) noexcept { return ::u_isalpha(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_digit(const char32_t cp) noexcept { return ::u_isdigit(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_alnum(const char32_t cp) noexcept { return ::u_isalnum(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_space(const char32_t cp) noexcept { return ::u_isspace(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_upper(const char32_t cp) noexcept { return ::u_isupper(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_lower(const char32_t cp) noexcept { return ::u_islower(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_punct(const char32_t cp) noexcept { return ::u_ispunct(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_print(const char32_t cp) noexcept { return ::u_isprint(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_titlecase(const char32_t cp) noexcept { return ::u_istitle(static_cast<::UChar32>(cp)) != 0; }
bool locale::is_white_space(const char32_t cp) noexcept { return ::u_isUWhiteSpace(static_cast<::UChar32>(cp)) != 0; }

char32_t locale::to_upper(const char32_t cp) noexcept {
    return static_cast<char32_t>(::u_toupper(static_cast<::UChar32>(cp)));
}
char32_t locale::to_lower(const char32_t cp) noexcept {
    return static_cast<char32_t>(::u_tolower(static_cast<::UChar32>(cp)));
}
char32_t locale::to_titlecase(const char32_t cp) noexcept {
    return static_cast<char32_t>(::u_totitle(static_cast<::UChar32>(cp)));
}

int locale::compare(const string& a, const string& b, const collate_strength strength) const {
    const auto* col = static_cast<::UCollator*>(collator_);
    if (col == nullptr) {
        const int cmp = string_compare(a.data(), b.data());
        return (cmp < 0) ? -1 : (cmp > 0) ? 1 : 0;
    }

    ::UErrorCode status = ::U_ZERO_ERROR;
#if U_ICU_VERSION_MAJOR_NUM >= 71
    ::UCollator* cloned = ::ucol_clone(col, &status);
#else
    ::UCollator* cloned = ::ucol_safeClone(col, nullptr, nullptr, &status);
#endif
    if ((::U_FAILURE(status) != 0) || cloned == nullptr) {
        const int cmp = string_compare(a.data(), b.data());
        return (cmp < 0) ? -1 : (cmp > 0) ? 1 : 0;
    }

    ::ucol_setAttribute(cloned, ::UCOL_STRENGTH, to_ucol_strength(strength), &status);

    status = ::U_ZERO_ERROR;
    const ::UCollationResult result = ::ucol_strcollUTF8(cloned, a.data(), static_cast<int32_t>(a.size()), b.data(),
                                                         static_cast<int32_t>(b.size()), &status);
    ::ucol_close(cloned);

    if (::U_FAILURE(status) != 0) {
        const int cmp = string_compare(a.data(), b.data());
        return (cmp < 0) ? -1 : (cmp > 0) ? 1 : 0;
    }

    switch (result) {
        case ::UCOL_LESS:
            return -1;
        case ::UCOL_EQUAL:
            return 0;
        case ::UCOL_GREATER:
            return 1;
    }
    return 0;
}

string locale::collation_key(const string& s) const {
    const auto* col = static_cast<::UCollator*>(collator_);
    if (col == nullptr) {
        return s;
    }

    vector<::UChar> ustr(s.size() + 1);
    ::UErrorCode cnv_status = ::U_ZERO_ERROR;
    int32_t ustr_len = 0;
    ::u_strFromUTF8(ustr.data(), static_cast<int32_t>(ustr.size()), &ustr_len, s.data(), static_cast<int32_t>(s.size()),
                    &cnv_status);
    if (::U_FAILURE(cnv_status) != 0) {
        return {};
    }

    int32_t key_len = ::ucol_getSortKey(col, ustr.data(), ustr_len, nullptr, 0);
    if (key_len <= 0) {
        return {};
    }

    string key(static_cast<size_t>(key_len), '\0');
    key_len = ::ucol_getSortKey(col, ustr.data(), ustr_len, reinterpret_cast<uint8_t*>(key.data()), key_len);
    if (key_len <= 0) {
        return {};
    }
    key.resize(static_cast<size_t>(key_len));
    return key;
}

string locale::to_multibyte(const u32string& ucs4) const {
    auto* cnv = static_cast<::UConverter*>(converter_);
    if (cnv == nullptr) {
        string result;
        for (const char32_t cp: ucs4) {
            if (cp < 0x80) {
                result += static_cast<char>(cp);
            } else if (cp < 0x800) {
                result += static_cast<char>(0xC0 | (cp >> 6));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                result += static_cast<char>(0xE0 | (cp >> 12));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (cp >> 18));
                result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        return result;
    }

    vector<::UChar> utf16(ucs4.size() * 2 + 1);
    int32_t u16_len = 0;
    ::UErrorCode status = ::U_ZERO_ERROR;
    ::u_strFromUTF32(utf16.data(), static_cast<int32_t>(utf16.size()), &u16_len,
                     reinterpret_cast<const ::UChar32*>(ucs4.data()), static_cast<int32_t>(ucs4.size()), &status);
    if ((::U_FAILURE(status) != 0) && status != ::U_BUFFER_OVERFLOW_ERROR) {
        return {};
    }

    string result(u16_len * 4 + 4, '\0');
    status = ::U_ZERO_ERROR;
    const int32_t out_len =
            ::ucnv_fromUChars(cnv, result.data(), static_cast<int32_t>(result.size()), utf16.data(), u16_len, &status);
    if (U_FAILURE(status) != 0 && status != ::U_BUFFER_OVERFLOW_ERROR) {
        throw locale_exception("to_multibyte: conversion failed");
    }
    result.resize(static_cast<size_t>(out_len));
    return result;
}

u32string locale::to_ucs4(const string& mb) const {
    auto* cnv = static_cast<::UConverter*>(converter_);
    if (cnv == nullptr) {
        u32string result;
        size_t i = 0;
        while (i < mb.size()) {
            char32_t cp = 0;
            const auto c = static_cast<byte_t>(mb[i]);
            if (c < 0x80) {
                cp = c;
                i += 1;
            } else if (c < 0xE0) {
                cp = static_cast<char32_t>(c & 0x1F) << 6;
                if (i + 1 < mb.size()) {
                    cp |= static_cast<unsigned char>(mb[i + 1]) & 0x3F;
                }
                i += 2;
            } else if (c < 0xF0) {
                cp = static_cast<char32_t>(c & 0x0F) << 12;
                if (i + 1 < mb.size()) {
                    cp |= (static_cast<unsigned char>(mb[i + 1]) & 0x3F) << 6;
                }
                if (i + 2 < mb.size()) {
                    cp |= static_cast<unsigned char>(mb[i + 2]) & 0x3F;
                }
                i += 3;
            } else {
                cp = static_cast<char32_t>(c & 0x07) << 18;
                if (i + 1 < mb.size()) {
                    cp |= (static_cast<unsigned char>(mb[i + 1]) & 0x3F) << 12;
                }
                if (i + 2 < mb.size()) {
                    cp |= (static_cast<unsigned char>(mb[i + 2]) & 0x3F) << 6;
                }
                if (i + 3 < mb.size()) {
                    cp |= static_cast<unsigned char>(mb[i + 3]) & 0x3F;
                }
                i += 4;
            }
            result += cp;
        }
        return result;
    }

    vector<::UChar> utf16(mb.size() * 2 + 1);
    ::UErrorCode status = ::U_ZERO_ERROR;
    const int32_t u16_len = ::ucnv_toUChars(cnv, utf16.data(), static_cast<int32_t>(utf16.size()), mb.data(),
                                            static_cast<int32_t>(mb.size()), &status);
    if ((::U_FAILURE(status) != 0) && status != ::U_BUFFER_OVERFLOW_ERROR) {
        throw locale_exception("to_ucs4: conversion failed");
    }

    u32string result(u16_len + 1, U'\0');
    status = U_ZERO_ERROR;
    int32_t u32_len = 0;
    ::u_strToUTF32(reinterpret_cast<::UChar32*>(result.data()), static_cast<int32_t>(result.size()), &u32_len,
                   utf16.data(), u16_len, &status);
    if ((::U_FAILURE(status) != 0) && status != ::U_BUFFER_OVERFLOW_ERROR) {
        throw locale_exception("to_ucs4: conversion failed");
    }
    result.resize(static_cast<size_t>(u32_len));
    return result;
}

string locale::format_number(const int64_t value) const {
    const auto* fmt = static_cast<::UNumberFormat*>(num_fmt_);
    if (fmt == nullptr) {
        return to_string(value);
    }

    ::UErrorCode status = ::U_ZERO_ERROR;
    const int32_t needed = ::unum_formatInt64(fmt, value, nullptr, 0, nullptr, &status);
    if (status == ::U_BUFFER_OVERFLOW_ERROR && needed > 0) {
        string result(static_cast<size_t>(needed) + 1, '\0');
        status = ::U_ZERO_ERROR;
        vector<::UChar> ubuf(static_cast<size_t>(needed) + 1);
        unum_formatInt64(fmt, value, ubuf.data(), static_cast<int32_t>(ubuf.size()), nullptr, &status);
        if (::U_SUCCESS(status) != 0) {
            return icu_to_string(ubuf.data(), needed);
        }
    }
    return to_string(value);
}

string locale::format_number(const double value, const int32_t fraction_digits) const {
    auto* fmt = static_cast<::UNumberFormat*>(num_fmt_);
    if (fmt == nullptr) {
        return to_string(value);
    }

    ::UErrorCode status = ::U_ZERO_ERROR;

    if (fraction_digits >= 0) {
        ::unum_setAttribute(fmt, ::UNUM_MIN_FRACTION_DIGITS, fraction_digits);
        ::unum_setAttribute(fmt, ::UNUM_MAX_FRACTION_DIGITS, fraction_digits);
    } else {
        ::unum_setAttribute(fmt, ::UNUM_MIN_FRACTION_DIGITS, 0);
        ::unum_setAttribute(fmt, ::UNUM_MAX_FRACTION_DIGITS, 6);
    }

    const int32_t needed = ::unum_formatDouble(fmt, value, nullptr, 0, nullptr, &status);
    if (status == ::U_BUFFER_OVERFLOW_ERROR && needed > 0) {
        vector<::UChar> ubuf(static_cast<size_t>(needed) + 1);
        status = ::U_ZERO_ERROR;
        ::unum_formatDouble(fmt, value, ubuf.data(), static_cast<int32_t>(ubuf.size()), nullptr, &status);
        if (::U_SUCCESS(status) != 0) {
            return icu_to_string(ubuf.data(), needed);
        }
    }
    return to_string(value);
}

string locale::format_currency(const double value, const string& iso_4217_code) const {
    UErrorCode status = U_ZERO_ERROR;

    void* c_fmt = open_currency_formatter(icu_name_.data(), iso_4217_code.data());
    auto* fmt = static_cast<::UNumberFormat*>(c_fmt);
    if (fmt == nullptr) {
        return to_string(value) + " " + iso_4217_code;
    }

    const int32_t needed = ::unum_formatDouble(fmt, value, nullptr, 0, nullptr, &status);
    string result;
    if (status == ::U_BUFFER_OVERFLOW_ERROR && needed > 0) {
        vector<::UChar> ubuf(static_cast<size_t>(needed) + 1);
        status = ::U_ZERO_ERROR;
        ::unum_formatDouble(fmt, value, ubuf.data(), static_cast<int32_t>(ubuf.size()), nullptr, &status);
        if (::U_SUCCESS(status) != 0) {
            result = icu_to_string(ubuf.data(), needed);
        }
    }

    ::unum_close(fmt);
    return result;
}

string locale::format_datetime(const datetime& dt, const date_style ds, const time_style ts) const {
    const ::UDate udate = datetime_to_udate(dt);
    ::UErrorCode status = ::U_ZERO_ERROR;
    ::UDateFormat* fmt =
            udat_open(to_udate_style(ds), to_utime_style(ts), name_.data(), nullptr, 0, nullptr, 0, &status);
    if (::U_FAILURE(status) != 0) {
        return dt.to_string();
    }

    const int32_t needed = ::udat_format(fmt, udate, nullptr, 0, nullptr, &status);
    string result;
    if (status == ::U_BUFFER_OVERFLOW_ERROR && needed > 0) {
        vector<::UChar> ubuf(static_cast<size_t>(needed) + 1);
        status = U_ZERO_ERROR;
        ::udat_format(fmt, udate, ubuf.data(), static_cast<int32_t>(ubuf.size()), nullptr, &status);
        if (::U_SUCCESS(status) != 0) {
            result = icu_to_string(ubuf.data(), needed);
        }
    }

    ::udat_close(fmt);
    return result.empty() ? dt.to_string() : result;
}

string locale::format_datetime(const datetime& dt, const string& pattern) const {
    const ::UDate udate = datetime_to_udate(dt);
    void* pat_fmt = open_date_pattern_formatter(name_.data(), pattern.data());
    auto* fmt = static_cast<::UDateFormat*>(pat_fmt);
    if (fmt == nullptr) {
        return dt.to_string();
    }

    ::UErrorCode status = ::U_ZERO_ERROR;
    const int32_t needed = ::udat_format(fmt, udate, nullptr, 0, nullptr, &status);
    string result;
    if (status == ::U_BUFFER_OVERFLOW_ERROR && needed > 0) {
        vector<::UChar> ubuf(static_cast<size_t>(needed) + 1);
        status = ::U_ZERO_ERROR;
        ::udat_format(fmt, udate, ubuf.data(), static_cast<int32_t>(ubuf.size()), nullptr, &status);
        if (::U_SUCCESS(status) != 0) {
            result = icu_to_string(ubuf.data(), needed);
        }
    }

    ::udat_close(fmt);
    return result.empty() ? dt.to_string() : result;
}

string locale::format_datetime() const {
    const datetime now = datetime::now();
    return format_datetime(now, date_style::medium, time_style::medium);
}

locale::numeric_info locale::numeric() const {
    numeric_info info;
    ::UErrorCode status = ::U_ZERO_ERROR;

    const auto* fmt = static_cast<::UNumberFormat*>(num_fmt_);

    if (fmt == nullptr) {
        info.decimal_point = ".";
        info.thousands_sep = ",";
        info.grouping = "";
        info.percent_sign = "%";
        info.minus_sign = "-";
        info.plus_sign = "+";
        info.exponential = "E";
        info.nan_symbol = "NaN";
        info.infinity_symbol = "∞";
        return info;
    }

    ::UChar ubuf[16] = {};
    ::unum_getSymbol(fmt, ::UNUM_DECIMAL_SEPARATOR_SYMBOL, ubuf, 16, &status);
    info.decimal_point = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_GROUPING_SEPARATOR_SYMBOL, ubuf, 16, &status);
    info.thousands_sep = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_PERCENT_SYMBOL, ubuf, 16, &status);
    info.percent_sign = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_MINUS_SIGN_SYMBOL, ubuf, 16, &status);
    info.minus_sign = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_PLUS_SIGN_SYMBOL, ubuf, 16, &status);
    info.plus_sign = icu_to_string(ubuf, -1);

    info.grouping = "";

    return info;
}

locale::monetary_info locale::monetary() const {
    monetary_info info{};
    ::UErrorCode status = ::U_ZERO_ERROR;

    auto* fmt = static_cast<::UNumberFormat*>(open_currency_formatter(icu_name_.data(), "USD"));
    if (fmt == nullptr) {
        return info;
    }

    ::UChar ubuf[16] = {};

    ::unum_getSymbol(fmt, ::UNUM_CURRENCY_SYMBOL, ubuf, 16, &status);
    info.currency_symbol = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_INTL_CURRENCY_SYMBOL, ubuf, 16, &status);
    info.int_curr_symbol = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_MONETARY_SEPARATOR_SYMBOL, ubuf, 16, &status);
    info.mon_decimal_point = icu_to_string(ubuf, -1);

    status = ::U_ZERO_ERROR;
    ::unum_getSymbol(fmt, ::UNUM_MONETARY_GROUPING_SEPARATOR_SYMBOL, ubuf, 16, &status);
    info.mon_thousands_sep = icu_to_string(ubuf, -1);

    info.frac_digits = ::unum_getAttribute(fmt, ::UNUM_MIN_FRACTION_DIGITS);

    ::unum_close(fmt);
    return info;
}

locale::time_info locale::time() const {
    time_info info;
    ::UErrorCode status = ::U_ZERO_ERROR;

    ::UDateFormat* dfmt = ::udat_open(::UDAT_MEDIUM, ::UDAT_NONE, name_.data(), nullptr, 0, nullptr, 0, &status);
    if (::U_SUCCESS(status) != 0) {
        ::UChar ubuf[128] = {};
        const int32_t len = ::udat_toPattern(dfmt, 0, ubuf, 128, &status);
        if ((::U_SUCCESS(status) != 0) && len > 0) {
            info.date_fmt = icu_to_string(ubuf, len);
        }
        ::udat_close(dfmt);
    }

    status = ::U_ZERO_ERROR;
    ::UDateFormat* tfmt = ::udat_open(::UDAT_NONE, ::UDAT_MEDIUM, name_.data(), nullptr, 0, nullptr, 0, &status);
    if (::U_SUCCESS(status) != 0) {
        UChar ubuf[128] = {};
        const int32_t len = ::udat_toPattern(tfmt, 0, ubuf, 128, &status);
        if ((::U_SUCCESS(status) != 0) && len > 0) {
            info.time_fmt = icu_to_string(ubuf, len);
        }
        ::udat_close(tfmt);
    }

    status = ::U_ZERO_ERROR;
    ::UDateFormat* dtfmt = ::udat_open(::UDAT_MEDIUM, ::UDAT_MEDIUM, name_.data(), nullptr, 0, nullptr, 0, &status);
    if (::U_SUCCESS(status) != 0) {
        ::UChar ubuf[128] = {};
        const int32_t len = ::udat_toPattern(dtfmt, 0, ubuf, 128, &status);
        if ((::U_SUCCESS(status) != 0) && len > 0) {
            info.datetime_fmt = icu_to_string(ubuf, len);
        }
        ::udat_close(dtfmt);
    }

    status = ::U_ZERO_ERROR;
    ::UDateFormat* sym_fmt = ::udat_open(::UDAT_FULL, ::UDAT_NONE, name_.data(), nullptr, 0, nullptr, 0, &status);
    if (::U_SUCCESS(status) != 0) {
        ::UChar ubuf[64] = {};
        for (int i = 0; i < 7; ++i) {
            status = ::U_ZERO_ERROR;
            const int32_t len = ::udat_getSymbols(sym_fmt, ::UDAT_STANDALONE_WEEKDAYS, i, ubuf, 64, &status);
            if ((::U_SUCCESS(status) != 0) && len > 0) {
                info.day_names.push_back(icu_to_string(ubuf, len));
            }
        }
        for (int i = 0; i < 7; ++i) {
            status = ::U_ZERO_ERROR;
            const int32_t len = ::udat_getSymbols(sym_fmt, ::UDAT_STANDALONE_SHORT_WEEKDAYS, i, ubuf, 64, &status);
            if ((::U_SUCCESS(status) != 0) && len > 0) {
                info.abbr_day_names.push_back(icu_to_string(ubuf, len));
            }
        }
        for (int i = 0; i < 12; ++i) {
            status = ::U_ZERO_ERROR;
            const int32_t len = ::udat_getSymbols(sym_fmt, ::UDAT_STANDALONE_MONTHS, i, ubuf, 64, &status);
            if ((::U_SUCCESS(status) != 0) && len > 0) {
                info.month_names.push_back(icu_to_string(ubuf, len));
            }
        }
        for (int i = 0; i < 12; ++i) {
            status = ::U_ZERO_ERROR;
            const int32_t len = ::udat_getSymbols(sym_fmt, ::UDAT_STANDALONE_SHORT_MONTHS, i, ubuf, 64, &status);
            if ((::U_SUCCESS(status) != 0) && len > 0) {
                info.abbr_month_names.push_back(icu_to_string(ubuf, len));
            }
        }

        status = ::U_ZERO_ERROR;
        int32_t len = ::udat_getSymbols(sym_fmt, ::UDAT_AM_PMS, 0, ubuf, 64, &status);
        if ((::U_SUCCESS(status) != 0) && len > 0) {
            info.am_str = icu_to_string(ubuf, len);
        }
        status = ::U_ZERO_ERROR;
        len = udat_getSymbols(sym_fmt, ::UDAT_AM_PMS, 1, ubuf, 64, &status);
        if ((::U_SUCCESS(status) != 0) && len > 0) {
            info.pm_str = icu_to_string(ubuf, len);
        }

        ::udat_close(sym_fmt);
    }

    return info;
}

vector<string> locale::available_locales() {
    ::UErrorCode status = ::U_ZERO_ERROR;
    ::UEnumeration* en = ::uloc_openAvailableByType(::ULOC_AVAILABLE_DEFAULT, &status);
    return enum_to_vector(en);
}

vector<string> locale::available_countries(const string& /*language*/) {
    vector<string> result;
    const char* const* countries = ::uloc_getISOCountries();
    if (countries != nullptr) {
        for (int32_t i = 0; countries[i] != nullptr; ++i) {
            result.emplace_back(countries[i]);
        }
    }
    return result;
}

bool locale::is_valid_locale(const string& name) {
    ::UErrorCode status = ::U_ZERO_ERROR;
    char canonical[ULOC_FULLNAME_CAPACITY] = {};
    ::uloc_canonicalize(name.data(), canonical, sizeof(canonical), &status);
    if ((::U_FAILURE(status) != 0) || canonical[0] == '\0') {
        return false;
    }

    char lang[ULOC_LANG_CAPACITY] = {};
    status = ::U_ZERO_ERROR;
    ::uloc_getLanguage(canonical, lang, sizeof(lang), &status);
    return (::U_SUCCESS(status) != 0) && lang[0] != '\0';
}

NEFORCE_END_NAMESPACE__
