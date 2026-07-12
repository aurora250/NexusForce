#ifndef NEFORCE_CORE_SYSTEM_LOCALE_HPP__
#define NEFORCE_CORE_SYSTEM_LOCALE_HPP__

/**
 * @file locale.hpp
 * @brief 区域设置
 *
 * 此文件提供了基于 ICU4C 的区域设置管理功能，
 * 包括数字、货币、日期时间格式化、字符分类、排序和编码转换。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/exception/system_exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

class date;
class time;
class datetime;
class timestamp;


/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct locale_exception
 * @brief 区域设置操作异常
 */
struct locale_exception final : system_exception {
    explicit locale_exception(const char* info = "Locale Operation Failed",
                              const error_code code = last_error()) noexcept :
    system_exception(info, code) {}

    explicit locale_exception(const exception& e) :
    system_exception(e) {}

    ~locale_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "locale_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup Locale 区域设置
 * @brief 区域设置实现
 * @{
 */

/**
 * @class locale
 * @brief 区域设置管理类
 *
 * 基于 ICU4C 提供跨平台的区域设置信息查询和格式化功能。
 * 所有 locale 数据来自 CLDR（Unicode Common Locale Data Repository）。
 *
 * 使用场景：
 * - 国际化/本地化应用程序
 * - 格式化数字、货币、日期时间
 * - 本地化字符串排序
 * - 字符分类和大小写转换
 */
class NEFORCE_API locale {
public:
    /**
     * @enum collate_strength
     * @brief 字符串比较强度
     */
    enum class collate_strength : int32_t {
        primary = 0,    ///< 一级比较：仅基础字符
        secondary = 1,  ///< 二级比较：区分重音符号
        tertiary = 2,   ///< 三级比较：区分大小写
        quaternary = 3, ///< 四级比较：区分标点
        identical = 15  ///< 完全比较：逐码点比较
    };

    /**
     * @enum date_style
     * @brief 日期格式化风格
     */
    enum class date_style : int8_t {
        none = -1,     ///< 不生成日期部分
        full = 0,      ///< 完整格式："Tuesday, April 15, 2025"
        long_fmt = 1,  ///< 长格式："April 15, 2025"
        medium = 2,    ///< 中格式："Apr 15, 2025"
        short_fmt = 3, ///< 短格式："4/15/25"
        relative = 4   ///< 相对时间："yesterday", "3 hours ago"
    };

    /**
     * @enum time_style
     * @brief 时间格式化风格
     */
    enum class time_style : int8_t {
        none = -1,     ///< 不生成时间部分
        full = 0,      ///< 完整格式："3:45:30 PM Pacific Standard Time"
        long_fmt = 1,  ///< 长格式："3:45:30 PM PST"
        medium = 2,    ///< 中格式："3:45:30 PM"
        short_fmt = 3, ///< 短格式："3:45 PM"
        relative = 4   ///< 相对时间："5 minutes ago"
    };

    /**
     * @enum measurement_system
     * @brief 度量系统
     */
    enum class measurement_system : uint8_t {
        SI, ///< 公制
        US, ///< 美制
        UK  ///< 英制
    };

    /**
     * @enum text_direction
     * @brief 文字方向
     */
    enum class text_direction : uint8_t {
        LTR, ///< 从左到右
        RTL  ///< 从右到左
    };

    /**
     * @struct numeric_info
     * @brief 数字格式信息
     */
    struct numeric_info {
        string decimal_point;   ///< 小数点
        string thousands_sep;   ///< 千位分隔符
        string grouping;        ///< 分组规则
        string percent_sign;    ///< 百分号
        string minus_sign;      ///< 负号
        string plus_sign;       ///< 正号
        string exponential;     ///< 科学记数法符号
        string nan_symbol;      ///< NaN 表示
        string infinity_symbol; ///< ∞ 表示
    };

    /**
     * @struct monetary_info
     * @brief 货币格式信息
     */
    struct monetary_info {
        string currency_symbol;      ///< 货币符号
        string int_curr_symbol;      ///< 国际货币符号（ISO 4217）
        string mon_decimal_point;    ///< 货币小数点
        string mon_thousands_sep;    ///< 货币千位分隔符
        string mon_grouping;         ///< 货币分组规则
        string positive_sign;        ///< 正数符号
        string negative_sign;        ///< 负数符号
        int32_t frac_digits{2};      ///< 本地货币小数位数
        int32_t int_frac_digits{2};  ///< 国际货币小数位数
        int32_t cash_frac_digits{2}; ///< 现金小数位数
        bool p_cs_precedes{true};    ///< 正数时货币符号前置
        bool n_cs_precedes{true};    ///< 负数时货币符号前置
        int32_t p_sep_by_space{0};   ///< 正数货币符号间距
        int32_t n_sep_by_space{0};   ///< 负数货币符号间距
    };

    /**
     * @struct time_info
     * @brief 时间格式信息
     */
    struct time_info {
        string date_fmt;                 ///< 日期格式模式
        string time_fmt;                 ///< 时间格式模式
        string datetime_fmt;             ///< 日期时间格式模式
        vector<string> day_names;        ///< 星期几全名
        vector<string> abbr_day_names;   ///< 星期几缩写
        vector<string> month_names;      ///< 月份全名
        vector<string> abbr_month_names; ///< 月份缩写
        string am_str;                   ///< 上午标识
        string pm_str;                   ///< 下午标识
    };

private:
    string name_;              ///< 规范 BCP-47 标识符，如 "en-US"
    string icu_name_;          ///< ICU 规范名称（下划线格式），用于内部 API 调用
    void* collator_{nullptr};  ///< 缓存的排序器
    void* converter_{nullptr}; ///< 缓存的编码转换器

    mutable void* num_fmt_{nullptr};  ///< 数字格式化器
    mutable void* curr_fmt_{nullptr}; ///< 货币格式化器
    mutable void* date_fmt_{nullptr}; ///< 日期时间格式化器

    string language_code_; ///< ISO 639 语言代码
    string script_code_;   ///< ISO 15924 文字代码
    string country_code_;  ///< ISO 3166 国家/地区代码
    string variant_code_;  ///< 变体代码

    /**
     * @brief 从 BCP-47 标签加载 locale
     * @param bcp47 BCP-47 或 POSIX 格式的名称
     * @throws locale_exception 名称无效时抛出
     */
    void load(const string& bcp47);

    /**
     * @brief 清理所有 ICU 资源
     */
    void cleanup() noexcept;

public:
    /**
     * @brief 默认构造函数，创建 en-US-POSIX locale
     */
    locale();

    /**
     * @brief 从名称构造
     * @param name BCP-47 标签（"en-US"）或 POSIX 名（"en_US.UTF-8"）
     * @throws locale_exception 名称无效时抛出
     */
    explicit locale(const string& name);

    /**
     * @brief 析构函数
     */
    ~locale();

    /**
     * @brief 拷贝构造函数
     * @param other 源 locale
     */
    locale(const locale& other);

    /**
     * @brief 拷贝赋值运算符
     * @param other 源 locale
     * @return 自身引用
     */
    locale& operator=(const locale& other);

    /**
     * @brief 移动构造函数
     * @param other 源 locale
     */
    locale(locale&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源 locale
     * @return 自身引用
     */
    locale& operator=(locale&& other) noexcept;

    /**
     * @brief 获取经典（en-US-POSIX）locale
     * @return 经典 locale
     */
    NEFORCE_NODISCARD static locale classic();

    /**
     * @brief 获取系统默认 locale
     * @return 系统 locale
     */
    NEFORCE_NODISCARD static locale system();

    /**
     * @brief 从名称构造 locale
     * @param name BCP-47 或 POSIX 名称
     * @return locale 对象
     */
    NEFORCE_NODISCARD static locale from_name(const string& name);

    /**
     * @brief 获取规范 BCP-47 名称
     * @return BCP-47 标签，如 "en-US"
     */
    NEFORCE_NODISCARD const string& name() const noexcept { return name_; }

    /**
     * @brief 获取 ISO 639 语言代码
     * @return 语言代码，如 "en"、"zh"
     */
    NEFORCE_NODISCARD const string& language_code() const noexcept { return language_code_; }

    /**
     * @brief 获取 ISO 15924 文字代码
     * @return 文字代码，如 "Latn"、"Hans"
     */
    NEFORCE_NODISCARD const string& script_code() const noexcept { return script_code_; }

    /**
     * @brief 获取 ISO 3166 国家/地区代码
     * @return 地区代码，如 "US"、"CN"
     */
    NEFORCE_NODISCARD const string& country_code() const noexcept { return country_code_; }

    /**
     * @brief 获取变体代码
     * @return 变体代码，如 "POSIX"、"1996"
     */
    NEFORCE_NODISCARD const string& variant_code() const noexcept { return variant_code_; }

    /**
     * @brief 获取在该 locale 中的显示名
     * @param display_locale 显示所用的 locale，默认使用自身
     * @return 本地化显示名
     */
    NEFORCE_NODISCARD string display_name(const locale& display_locale) const;

    /**
     * @brief 获取本地语言名
     * @return 以该语言自身显示的语言名，如 "English"、"中文"
     */
    NEFORCE_NODISCARD string native_language_name() const;

    /**
     * @brief 获取本地国家/地区名
     * @return 以该语言自身显示的地区名，如 "United States"、"中国"
     */
    NEFORCE_NODISCARD string native_country_name() const;

    /**
     * @brief 获取文字方向
     * @return 从左到右或从右到左
     */
    NEFORCE_NODISCARD text_direction direction() const;

    /**
     * @brief 获取度量系统
     * @return 度量系统
     */
    NEFORCE_NODISCARD measurement_system measurement() const;

    /**
     * @brief 获取一周首日
     * @return 1=周日, 2=周一, ..., 7=周六
     */
    NEFORCE_NODISCARD int32_t first_day_of_week() const;

    /**
     * @brief 获取 UI 语言回退链
     * @return BCP-47 标签列表，按优先级降序
     *
     * 示例："zh-Hans-CN" → {"zh-Hans-CN", "zh-Hans", "zh-CN", "zh"}
     * 用于查找最优翻译资源。
     */
    NEFORCE_NODISCARD vector<string> ui_languages() const;

    NEFORCE_NODISCARD static bool is_alpha(char32_t cp) noexcept;       ///< 是否为字母
    NEFORCE_NODISCARD static bool is_digit(char32_t cp) noexcept;       ///< 是否为数字
    NEFORCE_NODISCARD static bool is_alnum(char32_t cp) noexcept;       ///< 是否为字母或数字
    NEFORCE_NODISCARD static bool is_space(char32_t cp) noexcept;       ///< 是否为空白字符（POSIX/C）
    NEFORCE_NODISCARD static bool is_upper(char32_t cp) noexcept;       ///< 是否为大写字母
    NEFORCE_NODISCARD static bool is_lower(char32_t cp) noexcept;       ///< 是否为小写字母
    NEFORCE_NODISCARD static bool is_punct(char32_t cp) noexcept;       ///< 是否为标点符号
    NEFORCE_NODISCARD static bool is_print(char32_t cp) noexcept;       ///< 是否为可打印字符
    NEFORCE_NODISCARD static bool is_titlecase(char32_t cp) noexcept;   ///< 是否为首字母大写
    NEFORCE_NODISCARD static bool is_white_space(char32_t cp) noexcept; ///< 是否为 Unicode 空白字符

    NEFORCE_NODISCARD static char32_t to_upper(char32_t cp) noexcept;     ///< 转换为大写
    NEFORCE_NODISCARD static char32_t to_lower(char32_t cp) noexcept;     ///< 转换为小写
    NEFORCE_NODISCARD static char32_t to_titlecase(char32_t cp) noexcept; ///< 转换为首字母大写

    /**
     * @brief 比较两个字符串
     * @param a 第一个字符串
     * @param b 第二个字符串
     * @param strength 比较强度
     * @return 负数表示 a < b，0 表示相等，正数表示 a > b
     */
    NEFORCE_NODISCARD int compare(const string& a, const string& b,
                                  collate_strength strength = collate_strength::tertiary) const;

    /**
     * @brief 生成排序键
     * @param s 源字符串
     * @return 可用于二进制比较的排序键
     */
    NEFORCE_NODISCARD string collation_key(const string& s) const;

    /**
     * @brief 将 UTF-32 字符串转换为 locale 编码的多字节字符串
     * @param ucs4 UTF-32 字符串
     * @return 多字节字符串
     * @throws locale_exception 转换失败时抛出
     */
    NEFORCE_NODISCARD string to_multibyte(const u32string& ucs4) const;

    /**
     * @brief 将 locale 编码的多字节字符串转换为 UTF-32
     * @param mb 多字节字符串
     * @return UTF-32 字符串
     * @throws locale_exception 转换失败时抛出
     */
    NEFORCE_NODISCARD u32string to_ucs4(const string& mb) const;

    /**
     * @brief 格式化整数
     * @param value 整数值
     * @return 格式化的字符串（含千位分隔符）
     */
    NEFORCE_NODISCARD string format_number(int64_t value) const;

    /**
     * @brief 格式化浮点数
     * @param value 浮点数值
     * @param fraction_digits 小数位数，-1 使用默认值
     * @return 格式化的字符串
     */
    NEFORCE_NODISCARD string format_number(double value, int32_t fraction_digits = 2) const;

    /**
     * @brief 格式化货币金额
     * @param value 金额
     * @param iso_4217_code ISO 4217 货币代码（如 "USD"、"CNY"、"EUR"）
     * @return 格式化的货币字符串
     */
    NEFORCE_NODISCARD string format_currency(double value, const string& iso_4217_code) const;

    /**
     * @brief 格式化日期时间
     * @param dt 日期时间对象
     * @param ds 日期风格
     * @param ts 时间风格
     * @return 格式化的日期时间字符串
     */
    NEFORCE_NODISCARD string format_datetime(const datetime& dt, date_style ds = date_style::medium,
                                             time_style ts = time_style::medium) const;

    /**
     * @brief 使用 ICU 模式字符串格式化日期时间
     * @param dt 日期时间对象
     * @param pattern ICU 日期格式模式（如 "yyyy-MM-dd HH:mm:ss"）
     * @return 格式化的字符串
     *
     * ICU 模式语法与 SimpleDateFormat 兼容。
     */
    NEFORCE_NODISCARD string format_datetime(const datetime& dt, const string& pattern) const;

    /**
     * @brief 格式化当前日期时间
     * @return 格式化的日期时间字符串（中日期 + 中时间）
     */
    NEFORCE_NODISCARD string format_datetime() const;

    /**
     * @brief 获取数字格式信息
     * @return 数字格式信息
     */
    NEFORCE_NODISCARD numeric_info numeric() const;

    /**
     * @brief 获取货币格式信息
     * @return 货币格式信息
     */
    NEFORCE_NODISCARD monetary_info monetary() const;

    /**
     * @brief 获取时间格式信息
     * @return 时间格式信息
     */
    NEFORCE_NODISCARD time_info time() const;

    /**
     * @brief 获取所有可用 locale 列表
     * @return BCP-47 标签列表
     */
    NEFORCE_NODISCARD static vector<string> available_locales();

    /**
     * @brief 获取指定语言下的可用国家/地区
     * @param language ISO 639 语言代码
     * @return 国家/地区代码列表
     */
    NEFORCE_NODISCARD static vector<string> available_countries(const string& language);

    /**
     * @brief 验证 locale 名称是否有效
     * @param name BCP-47 或 POSIX 名称
     * @return 是否可构造为有效 locale
     */
    NEFORCE_NODISCARD static bool is_valid_locale(const string& name);

    /**
     * @brief 相等比较
     * @param o 另一个 locale
     * @return BCP-47 规范名称相同返回 true
     */
    NEFORCE_NODISCARD bool operator==(const locale& o) const noexcept { return name_ == o.name_; }

    /**
     * @brief 不等比较
     * @param o 另一个 locale
     * @return BCP-47 规范名称不同返回 true
     */
    NEFORCE_NODISCARD bool operator!=(const locale& o) const noexcept { return !(*this == o); }
};

/** @} */ // Locale

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_LOCALE_HPP__
