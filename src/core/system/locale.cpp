#include <NeForce/core/system/locale.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/utility/packages.hpp>
#    include <NeForce/core/algorithm/sort.hpp>
#    include <NeForce/core/config/windef.hpp>
#    include <stringapiset.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <stdlib.h>
#    include <glob.h>
#    include <langinfo.h>
#    include <wctype.h>
#    include <wchar.h>
#    include <iconv.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    struct enum_ctx {
        vector<string> list;
    };

    BOOL CALLBACK enum_proc(LPWSTR lname, DWORD, LPARAM param) {
        auto* ctx = reinterpret_cast<enum_ctx*>(param);
        if (lname) {
            wstring wn(lname);
            replace(wn.begin(), wn.end(), L'-', L'_');
            ctx->list.push_back(wcharacter::to_string(wn.view()));
        }
        return TRUE;
    }

    /* POSIX name "en_US.UTF-8" -> BCP-47 "en-US" */
    string posix_to_win(const string& name) {
        if (name.empty() || name == "C" || name == "POSIX") {
            return {};
        }
        string base = name;
        const auto dot = base.find('.');
        if (dot != string::npos) {
            base.erase(dot);
        }
        replace(base.begin(), base.end(), '_', '-');
        return base;
    }

    wstring query_info(const LCTYPE type, const string_view win_name) {
        wstring wn_storage;
        const wchar_t* lname;
        if (win_name.empty()) {
            lname = LOCALE_NAME_USER_DEFAULT;
        } else {
            wn_storage = character::to_wstring(win_name);
            lname = wn_storage.data();
        }

        const int sz = ::GetLocaleInfoEx(lname, type, nullptr, 0);
        if (sz <= 0) {
            return {};
        }
        wstring buf(static_cast<size_t>(sz), L'\0');
        ::GetLocaleInfoEx(lname, type, &buf[0], sz);
        if (!buf.empty() && buf.back() == L'\0') {
            buf.pop_back();
        }
        return buf;
    }

    string query_info_utf8(const LCTYPE type, const string_view win_name) {
        const auto info = query_info(type, win_name);
        return wcharacter::to_string(info.view());
    }

    UINT ansi_codepage(const string_view win_name) {
        const auto s = query_info(LOCALE_IDEFAULTANSICODEPAGE, win_name);
        if (s.empty()) {
            return CP_ACP;
        }
        const auto ws = wcharacter::to_string(s.view());
        return uinteger32::parse(ws.view()).value();
    }

#else

    string nl_str(nl_item item, locale_t loc) const {
        const char* p = ::nl_langinfo_l(item, loc);
        return p ? string(p) : string{};
    }

    void free_locale(bool& owns, locale_t& loc) noexcept {
        if (owns && loc != LC_GLOBAL_LOCALE) {
            ::freelocale(loc);
            loc = LC_GLOBAL_LOCALE;
            owns = false;
        }
    }

#endif
} // namespace


void locale::load_locale(const string& name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    win_name_ = posix_to_win(name);
    name_ = name.empty() ? "C" : name;

    const auto cp = query_info(LOCALE_IDEFAULTANSICODEPAGE, win_name_.view());
    if (cp == L"65001") {
        encoding_ = "UTF-8";
    } else {
        encoding_ = "CP" + wcharacter::to_string(cp.view());
    }

#else
    free_locale(owns_, loc_);

    const char* lname = (name.empty() || name == "C" || name == "POSIX") ? "C" : name.c_str();

    locale_t loc = ::newlocale(LC_ALL_MASK, lname, static_cast<locale_t>(0));
    if (loc == static_cast<locale_t>(0)) {
        throw locale_exception(string("locale: cannot open '") + lname + "'");
    }

    loc_ = loc;
    owns_ = true;
    name_ = name.empty() ? "C" : name;
    const char* cs = ::nl_langinfo_l(CODESET, loc_);
    encoding_ = cs ? cs : "UTF-8";
#endif
}

locale::locale()
#ifdef NEFORCE_PLATFORM_LINUX
:
loc_(LC_GLOBAL_LOCALE),
owns_(false)
#endif
{
    load_locale("C");
}

locale::locale(const string& name)
#ifdef NEFORCE_PLATFORM_LINUX
:
loc_(LC_GLOBAL_LOCALE),
owns_(false)
#endif
{
    load_locale(name);
}

locale::~locale() {
#ifdef NEFORCE_PLATFORM_LINUX
    free_locale(owns_, loc_);
#endif
}

locale::locale(const locale& other)
#ifdef NEFORCE_PLATFORM_LINUX
:
loc_(LC_GLOBAL_LOCALE),
owns_(false)
#endif
{
    load_locale(other.name_);
}

locale& locale::operator=(const locale& other) {
    if (this != &other) {
        load_locale(other.name_);
    }
    return *this;
}

locale::locale(locale&& other) noexcept :
name_(move(other.name_)),
encoding_(move(other.encoding_))
#ifdef NEFORCE_PLATFORM_WINDOWS
,
win_name_(move(other.win_name_))
#else
,
loc_(other.loc_),
owns_(other.owns_)
#endif
{
#ifdef NEFORCE_PLATFORM_LINUX
    other.loc_ = LC_GLOBAL_LOCALE;
    other.owns_ = false;
#endif
}

locale& locale::operator=(locale&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

#ifdef NEFORCE_PLATFORM_LINUX
    free_locale(owns_, loc_);
#endif

    name_ = move(other.name_);
    encoding_ = move(other.encoding_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    win_name_ = move(other.win_name_);
#else
    loc_ = other.loc_;
    owns_ = other.owns_;
    other.loc_ = LC_GLOBAL_LOCALE;
    other.owns_ = false;
#endif
    return *this;
}

locale locale::classic() { return locale("C"); }
locale locale::from_name(const string& n) { return locale(n); }

locale locale::system() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    wchar_t buf[LOCALE_NAME_MAX_LENGTH]{};
    ::GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH);
    wstring wn(buf);
    replace(wn.begin(), wn.end(), L'-', L'_');
    return locale(wcharacter::to_string(wn.view()));
#else
    const char* env = ::getenv("LC_ALL");
    if (!env || !*env) {
        env = ::getenv("LC_CTYPE");
    }
    if (!env || !*env) {
        env = ::getenv("LANG");
    }
    return locale(env ? env : "C");
#endif
}

locale::numeric_info locale::numeric() const {
    numeric_info info;
#ifdef NEFORCE_PLATFORM_WINDOWS
    info.decimal_point = query_info_utf8(LOCALE_SDECIMAL, win_name_.view());
    info.thousands_sep = query_info_utf8(LOCALE_STHOUSAND, win_name_.view());
    {
        auto grp = query_info_utf8(LOCALE_SGROUPING, win_name_.view());
        string g;
        for (const char c: grp) {
            if (c >= '1' && c <= '9') {
                g += static_cast<char>(c - '0');
            } else if (c == '0') {
                g += '\0';
                break;
            }
        }
        info.grouping = g;
    }
#else
    info.decimal_point = nl_str(RADIXCHAR, loc_);
    info.thousands_sep = nl_str(THOUSEP, loc_);
    info.grouping = nl_str(GROUPING, loc_);
#endif
    return info;
}

locale::monetary_info locale::monetary() const {
    monetary_info info;

#ifdef NEFORCE_PLATFORM_WINDOWS
    info.currency_symbol = query_info_utf8(LOCALE_SCURRENCY, win_name_.view());
    info.int_curr_symbol = query_info_utf8(LOCALE_SINTLSYMBOL, win_name_.view());
    info.mon_decimal_point = query_info_utf8(LOCALE_SMONDECIMALSEP, win_name_.view());
    info.mon_thousands_sep = query_info_utf8(LOCALE_SMONTHOUSANDSEP, win_name_.view());
    info.positive_sign = query_info_utf8(LOCALE_SPOSITIVESIGN, win_name_.view());
    info.negative_sign = query_info_utf8(LOCALE_SNEGATIVESIGN, win_name_.view());
    {
        const auto s = query_info_utf8(LOCALE_ICURRDIGITS, win_name_.view());
        info.frac_digits = s.empty() ? 2 : integer32::parse(s.view()).value();
    }
    {
        const auto s = query_info_utf8(LOCALE_IINTLCURRDIGITS, win_name_.view());
        info.int_frac_digits = s.empty() ? 2 : integer32::parse(s.view()).value();
    }
    info.p_cs_precedes = (query_info_utf8(LOCALE_IPOSSYMPRECEDES, win_name_.view()) == "1");
    info.n_cs_precedes = (query_info_utf8(LOCALE_INEGSYMPRECEDES, win_name_.view()) == "1");

#else
    info.currency_symbol = nl_str(CRNCYSTR, loc_);
    if (!info.currency_symbol.empty()) {
        char ind = info.currency_symbol[0];
        info.p_cs_precedes = (ind == '-');
        info.n_cs_precedes = info.p_cs_precedes;
        info.currency_symbol.erase(0, 1);
    }
    info.mon_decimal_point = nl_str(MON_DECIMAL_POINT, loc_);
    info.mon_thousands_sep = nl_str(MON_THOUSANDS_SEP, loc_);
    info.mon_grouping = nl_str(MON_GROUPING, loc_);
    info.positive_sign = nl_str(POSITIVE_SIGN, loc_);
    info.negative_sign = nl_str(NEGATIVE_SIGN, loc_);
    {
        string s = nl_str(FRAC_DIGITS, loc_);
        info.frac_digits = s.empty() ? 2 : static_cast<int>(static_cast<unsigned char>(s[0]));
    }
    {
        string s = nl_str(INT_FRAC_DIGITS, loc_);
        info.int_frac_digits = s.empty() ? 2 : static_cast<int>(static_cast<unsigned char>(s[0]));
    }

#endif

    return info;
}

locale::time_info locale::time() const {
    time_info info;
#ifdef NEFORCE_PLATFORM_WINDOWS
    info.date_fmt = query_info_utf8(LOCALE_SSHORTDATE, win_name_.view());
    info.time_fmt = query_info_utf8(LOCALE_STIMEFORMAT, win_name_.view());
    info.datetime_fmt = info.date_fmt + " " + info.time_fmt;
    info.am_str = query_info_utf8(LOCALE_S1159, win_name_.view());
    info.pm_str = query_info_utf8(LOCALE_S2359, win_name_.view());

    static const LCTYPE kDay[7] = {LOCALE_SDAYNAME7, LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
                                   LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6};
    static const LCTYPE kAbbrDay[7] = {LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
                                       LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
                                       LOCALE_SABBREVDAYNAME6};
    for (int i = 0; i < 7; ++i) {
        info.day_names.push_back(query_info_utf8(kDay[i], win_name_.view()));
        info.abbr_day_names.push_back(query_info_utf8(kAbbrDay[i], win_name_.view()));
    }

    static const LCTYPE kMon[12] = {LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2,  LOCALE_SMONTHNAME3,  LOCALE_SMONTHNAME4,
                                    LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6,  LOCALE_SMONTHNAME7,  LOCALE_SMONTHNAME8,
                                    LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12};
    static const LCTYPE kAbbrMon[12] = {
            LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,  LOCALE_SABBREVMONTHNAME3,  LOCALE_SABBREVMONTHNAME4,
            LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,  LOCALE_SABBREVMONTHNAME7,  LOCALE_SABBREVMONTHNAME8,
            LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12};
    for (int i = 0; i < 12; ++i) {
        info.month_names.push_back(query_info_utf8(kMon[i], win_name_.view()));
        info.abbr_month_names.push_back(query_info_utf8(kAbbrMon[i], win_name_.view()));
    }
#else
    info.date_fmt = nl_str(D_FMT, loc_);
    info.time_fmt = nl_str(T_FMT, loc_);
    info.datetime_fmt = nl_str(D_T_FMT, loc_);
    info.am_str = nl_str(AM_STR, loc_);
    info.pm_str = nl_str(PM_STR, loc_);

    static const nl_item kDay[7] = {DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7};
    static const nl_item kAbbrDay[7] = {ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7};
    for (int i = 0; i < 7; ++i) {
        info.day_names.push_back(nl_str(kDay[i], loc_));
        info.abbr_day_names.push_back(nl_str(kAbbrDay[i], loc_));
    }

    static const nl_item kMon[12] = {MON_1, MON_2, MON_3, MON_4,  MON_5,  MON_6,
                                     MON_7, MON_8, MON_9, MON_10, MON_11, MON_12};
    static const nl_item kAbbrMon[12] = {ABMON_1, ABMON_2, ABMON_3, ABMON_4,  ABMON_5,  ABMON_6,
                                         ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12};
    for (int i = 0; i < 12; ++i) {
        info.month_names.push_back(nl_str(kMon[i], loc_));
        info.abbr_month_names.push_back(nl_str(kAbbrMon[i], loc_));
    }
#endif
    return info;
}

#ifdef NEFORCE_PLATFORM_WINDOWS

static WORD char_type1(char32_t cp) {
    wchar_t buf[3]{};
    int len = 0;
    if (cp < 0x10000) {
        buf[0] = static_cast<wchar_t>(cp);
        len = 1;
    } else {
        char32_t c = cp - 0x10000;
        buf[0] = static_cast<wchar_t>(0xD800 | (c >> 10));
        buf[1] = static_cast<wchar_t>(0xDC00 | (c & 0x3FF));
        len = 2;
    }
    WORD out[2]{};
    ::GetStringTypeW(CT_CTYPE1, buf, len, out);
    return out[0];
}

bool locale::is_alpha(char32_t cp) const noexcept { return (char_type1(cp) & C1_ALPHA) != 0; }
bool locale::is_digit(char32_t cp) const noexcept { return (char_type1(cp) & C1_DIGIT) != 0; }
bool locale::is_alnum(char32_t cp) const noexcept { return (char_type1(cp) & (C1_ALPHA | C1_DIGIT)) != 0; }
bool locale::is_space(char32_t cp) const noexcept { return (char_type1(cp) & C1_SPACE) != 0; }
bool locale::is_upper(char32_t cp) const noexcept { return (char_type1(cp) & C1_UPPER) != 0; }
bool locale::is_lower(char32_t cp) const noexcept { return (char_type1(cp) & C1_LOWER) != 0; }
bool locale::is_punct(char32_t cp) const noexcept { return (char_type1(cp) & C1_PUNCT) != 0; }
bool locale::is_print(char32_t cp) const noexcept {
    WORD t = char_type1(cp);
    return (t & (C1_ALPHA | C1_DIGIT | C1_PUNCT | C1_BLANK | C1_SPACE)) != 0;
}

static char32_t lcmap_case(const string& win_name, char32_t cp, DWORD flags) {
    wchar_t buf[3]{};
    int len = 0;
    if (cp < 0x10000) {
        buf[0] = static_cast<wchar_t>(cp);
        len = 1;
    } else {
        char32_t c = cp - 0x10000;
        buf[0] = static_cast<wchar_t>(0xD800 | (c >> 10));
        buf[1] = static_cast<wchar_t>(0xDC00 | (c & 0x3FF));
        len = 2;
    }

    wchar_t out[3]{};

    wstring wn_storage;
    const wchar_t* lname;
    if (win_name.empty()) {
        lname = LOCALE_NAME_USER_DEFAULT;
    } else {
        wn_storage = character::to_wstring(win_name.view());
        lname = wn_storage.data();
    }

    ::LCMapStringEx(lname, flags, buf, len, out, 3, nullptr, nullptr, 0);
    if (out[0] >= 0xD800 && out[0] <= 0xDBFF) {
        return 0x10000u + static_cast<char32_t>(((out[0] - 0xD800) << 10) | (out[1] - 0xDC00));
    }
    return static_cast<char32_t>(out[0]);
}

char32_t locale::to_upper(char32_t cp) const noexcept { return lcmap_case(win_name_, cp, LCMAP_UPPERCASE); }

char32_t locale::to_lower(char32_t cp) const noexcept { return lcmap_case(win_name_, cp, LCMAP_LOWERCASE); }

#else

bool locale::is_alpha(char32_t cp) const noexcept { return ::iswalpha_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_digit(char32_t cp) const noexcept { return ::iswdigit_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_alnum(char32_t cp) const noexcept { return ::iswalnum_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_space(char32_t cp) const noexcept { return ::iswspace_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_upper(char32_t cp) const noexcept { return ::iswupper_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_lower(char32_t cp) const noexcept { return ::iswlower_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_punct(char32_t cp) const noexcept { return ::iswpunct_l(static_cast<wint_t>(cp), loc_) != 0; }
bool locale::is_print(char32_t cp) const noexcept { return ::iswprint_l(static_cast<wint_t>(cp), loc_) != 0; }

char32_t locale::to_upper(char32_t cp) const noexcept {
    return static_cast<char32_t>(::towupper_l(static_cast<wint_t>(cp), loc_));
}
char32_t locale::to_lower(char32_t cp) const noexcept {
    return static_cast<char32_t>(::towlower_l(static_cast<wint_t>(cp), loc_));
}

#endif

int locale::compare(const string& a, const string& b, collate_strength strength) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    DWORD flags = 0;
    switch (strength) {
        case collate_strength::primary:
            flags = NORM_IGNORECASE | NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS;
            break;
        case collate_strength::secondary:
            flags = NORM_IGNORECASE | NORM_IGNORENONSPACE;
            break;
        case collate_strength::tertiary:
            flags = NORM_IGNORECASE;
            break;
        default:
            flags = 0;
            break;
    }
    wstring wa = character::to_wstring(a.view());
    wstring wb = character::to_wstring(b.view());

    wstring wn_storage;
    const wchar_t* lname;
    if (win_name_.empty()) {
        lname = LOCALE_NAME_USER_DEFAULT;
    } else {
        wn_storage = character::to_wstring(win_name_.view());
        lname = wn_storage.data();
    }

    int r = ::CompareStringEx(lname, flags, wa.data(), static_cast<int>(wa.size()), wb.data(),
                              static_cast<int>(wb.size()), nullptr, nullptr, 0);
    if (r == 0) {
        return 0;
    }
    return r - 2; // CSTR_LESS=1,CSTR_EQUAL=2,CSTR_GREATER=3
#else
    auto to_wide = [&](const string& s) -> wstring {
        wstring out(s.size() + 1, L'\0');
        locale_t saved = ::uselocale(loc_);
        size_t n = ::mbstowcs(&out[0], s.c_str(), out.size());
        ::uselocale(saved);
        if (n == static_cast<size_t>(-1)) {
            return {};
        }
        out.resize(n);
        return out;
    };
    auto wa = to_wide(a);
    auto wb = to_wide(b);
    return ::wcscoll_l(wa.c_str(), wb.c_str(), loc_);
#endif
}

string locale::collation_key(const string& s) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    wstring wn_storage;
    const wchar_t* lname;
    if (win_name_.empty()) {
        lname = LOCALE_NAME_USER_DEFAULT;
    } else {
        wn_storage = character::to_wstring(win_name_.view());
        lname = wn_storage.data();
    }

    wstring ws = character::to_wstring(s.view());
    int sz = ::LCMapStringEx(lname, LCMAP_SORTKEY, ws.data(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr,
                             0);
    if (sz <= 0) {
        return {};
    }
    string key(static_cast<size_t>(sz), '\0');
    ::LCMapStringEx(lname, LCMAP_SORTKEY, ws.data(), static_cast<int>(ws.size()), reinterpret_cast<LPWSTR>(&key[0]), sz,
                    nullptr, nullptr, 0);
    return key;
#else
    auto to_wide = [&](const string& mb) -> wstring {
        wstring out(mb.size() + 1, L'\0');
        locale_t saved = ::uselocale(loc_);
        size_t n = ::mbstowcs(&out[0], mb.c_str(), out.size());
        ::uselocale(saved);
        if (n == static_cast<size_t>(-1)) {
            return {};
        }
        out.resize(n);
        return out;
    };
    auto ws = to_wide(s);
    size_t sz = ::wcsxfrm_l(nullptr, ws.c_str(), 0, loc_);
    wstring key(sz + 1, L'\0');
    ::wcsxfrm_l(&key[0], ws.c_str(), sz + 1, loc_);
    return string(reinterpret_cast<const char*>(key.data()), sz * sizeof(wchar_t));
#endif
}

string locale::to_multibyte(const u32string& ucs4) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    // UCS-4 -> UTF-16 -> ANSI
    wstring wide;
    wide.reserve(ucs4.size());
    for (char32_t cp: ucs4) {
        if (cp < 0x10000) {
            wide += static_cast<wchar_t>(cp);
        } else {
            char32_t c = cp - 0x10000;
            wide += static_cast<wchar_t>(0xD800 | (c >> 10));
            wide += static_cast<wchar_t>(0xDC00 | (c & 0x3FF));
        }
    }
    UINT cp = ansi_codepage(win_name_.view());
    int sz = ::WideCharToMultiByte(cp, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (sz <= 0) {
        return {};
    }
    string out(static_cast<size_t>(sz), '\0');
    ::WideCharToMultiByte(cp, 0, wide.data(), static_cast<int>(wide.size()), &out[0], sz, nullptr, nullptr);
    return out;

#else
    iconv_t cd = ::iconv_open(encoding_.c_str(), "UTF-32LE");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        throw locale_exception("to_multibyte: iconv_open failed");
    }

    const char* in = reinterpret_cast<const char*>(ucs4.data());
    size_t in_left = ucs4.size() * sizeof(char32_t);
    string result(in_left * 4 + 4, '\0');
    char* out = &result[0];
    size_t out_left = result.size();

    if (::iconv(cd, const_cast<char**>(&in), &in_left, &out, &out_left) == static_cast<size_t>(-1)) {
        ::iconv_close(cd);
        throw locale_exception("to_multibyte: iconv failed");
    }
    ::iconv_close(cd);
    result.resize(result.size() - out_left);
    return result;
#endif
}

u32string locale::to_ucs4(const string& mb) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    UINT cp = ansi_codepage(win_name_.view());
    int wsz = ::MultiByteToWideChar(cp, 0, mb.data(), static_cast<int>(mb.size()), nullptr, 0);
    if (wsz <= 0) {
        return {};
    }
    wstring wide(static_cast<size_t>(wsz), L'\0');
    ::MultiByteToWideChar(cp, 0, mb.data(), static_cast<int>(mb.size()), &wide[0], wsz);
    u32string out;
    out.reserve(wide.size());
    for (size_t i = 0; i < wide.size();) {
        wchar_t wc = wide[i];
        if (wc >= 0xD800 && wc <= 0xDBFF && i + 1 < wide.size()) {
            char32_t hi = static_cast<char32_t>(wc - 0xD800);
            char32_t lo = static_cast<char32_t>(wide[++i] - 0xDC00);
            out += static_cast<char32_t>(0x10000u + (hi << 10) + lo);
        } else {
            out += static_cast<char32_t>(wc);
        }
        ++i;
    }
    return out;

#else

    iconv_t cd = ::iconv_open("UTF-32LE", encoding_.c_str());
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        throw locale_exception("to_ucs4: iconv_open failed");
    }

    const char* in = mb.data();
    size_t in_left = mb.size();
    u32string result(mb.size() + 1, U'\0');
    char* out = reinterpret_cast<char*>(&result[0]);
    size_t out_left = result.size() * sizeof(char32_t);

    if (::iconv(cd, const_cast<char**>(&in), &in_left, &out, &out_left) == static_cast<size_t>(-1)) {
        ::iconv_close(cd);
        throw locale_exception("to_ucs4: iconv failed");
    }
    ::iconv_close(cd);
    size_t chars = (result.size() * sizeof(char32_t) - out_left) / sizeof(char32_t);
    result.resize(chars);
    return result;
#endif
}

vector<string> locale::available_locales() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    enum_ctx ctx;
    ::EnumSystemLocalesEx(enum_proc, LOCALE_ALL, reinterpret_cast<LPARAM>(&ctx), nullptr);
    sort(ctx.list.begin(), ctx.list.end());
    return ctx.list;

#else

    vector<string> result;

    auto collect = [&](const char* pattern) {
        ::glob_t g{};
        if (::glob(pattern, GLOB_NOSORT, nullptr, &g) == 0) {
            for (size_t i = 0; i < g.gl_pathc; ++i) {
                const char* sl = ::strrchr(g.gl_pathv[i], '/');
                if (!sl) {
                    continue;
                }
                string n(sl + 1);
                if (find(result.begin(), result.end(), n) == result.end()) {
                    result.push_back(move(n));
                }
            }
            ::globfree(&g);
        }
    };

    collect("/usr/share/locale/*");
    collect("/usr/lib/locale/*");

    for (const char* b: {"C", "POSIX"}) {
        if (find(result.begin(), result.end(), b) == result.end()) {
            result.emplace_back(b);
        }
    }

    sort(result.begin(), result.end());
    return result;
#endif
}

NEFORCE_END_NAMESPACE__
