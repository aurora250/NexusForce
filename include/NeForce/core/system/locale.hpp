#ifndef NEFORCE_CORE_SYSTEM_LOCALE_HPP__
#define NEFORCE_CORE_SYSTEM_LOCALE_HPP__

/**
 * @file locale.hpp
 * @brief 区域设置
 *
 * 此文件提供了跨平台的区域设置管理功能，支持获取和设置本地化信息，
 * 包括数字格式、货币格式、时间格式、字符分类和大小写转换等。
 */

#include "NeForce/core/string/string.hpp"
#include "NeForce/core/container/vector.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include <locale.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

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
    explicit locale_exception(const char* info = "Locale Operation Failed", const char* type = static_type,
                              const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit locale_exception(const exception& e) :
    system_exception(e) {}

    ~locale_exception() override = default;

    static constexpr auto static_type = "locale_exception";
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
 * 提供跨平台的区域设置信息查询和字符处理功能。
 *
 * 使用场景：
 * - 国际化/本地化应用程序
 * - 格式化数字、货币、日期时间
 * - 本地化字符串排序
 * - 字符分类和大小写转换
 *
 * @note 区域设置对象是独立的，修改一个对象不影响其他对象。
 */
class NEFORCE_API locale {
public:
    /**
     * @struct numeric_info
     * @brief 数字格式信息
     */
    struct numeric_info {
        string decimal_point; ///< 小数点分隔符
        string thousands_sep; ///< 千位分隔符
        string grouping;      ///< 数字分组规则
    };

    /**
     * @struct monetary_info
     * @brief 货币格式信息
     */
    struct monetary_info {
        string currency_symbol;   ///< 货币符号
        string int_curr_symbol;   ///< 国际货币符号
        string mon_decimal_point; ///< 货币小数点分隔符
        string mon_thousands_sep; ///< 货币千位分隔符
        string mon_grouping;      ///< 货币数字分组规则
        string positive_sign;     ///< 正数符号
        string negative_sign;     ///< 负数符号
        int frac_digits{2};       ///< 本地货币小数位数
        int int_frac_digits{2};   ///< 国际货币小数位数
        bool p_cs_precedes{true}; ///< 正数时货币符号是否前置
        bool n_cs_precedes{true}; ///< 负数时货币符号是否前置
    };

    /**
     * @struct time_info
     * @brief 时间格式信息
     */
    struct time_info {
        string date_fmt;                 ///< 日期格式
        string time_fmt;                 ///< 时间格式
        string datetime_fmt;             ///< 日期时间格式
        vector<string> day_names;        ///< 星期几全名
        vector<string> abbr_day_names;   ///< 星期几缩写
        vector<string> month_names;      ///< 月份全名
        vector<string> abbr_month_names; ///< 月份缩写
        string am_str;                   ///< 上午标识
        string pm_str;                   ///< 下午标识
    };

    /**
     * @enum collate_strength
     * @brief 字符串比较强度
     *
     * 定义字符串比较时考虑的敏感度级别。
     */
    enum class collate_strength : int32_t {
        primary = 1,   ///< 一级比较：忽略大小写、变音符号、标点符号
        secondary = 2, ///< 二级比较：忽略大小写、变音符号
        tertiary = 3,  ///< 三级比较：忽略大小写
        identical = 4  ///< 完全比较：区分所有字符
    };

private:
    string name_;     ///< 区域设置名称
    string encoding_; ///< 字符编码

#ifdef NEFORCE_PLATFORM_WINDOWS
    string win_name_; ///< 区域设置名称
#else
    ::locale_t loc_; ///< 区域设置对象
    bool owns_;      ///< 是否拥有区域设置对象所有权
#endif

    void load_locale(const string& name);

public:
    /**
     * @brief 默认构造函数
     *
     * 创建"C"区域设置。
     */
    locale();

    /**
     * @brief 从名称构造区域设置
     * @param name 区域设置名称（如"en_US.UTF-8"）
     * @throws locale_exception 区域设置不存在时抛出
     */
    explicit locale(const string& name);

    /**
     * @brief 析构函数
     */
    ~locale();

    /**
     * @brief 拷贝构造函数
     * @param other 源区域设置
     */
    locale(const locale& other);

    /**
     * @brief 拷贝赋值运算符
     * @param other 源区域设置
     * @return 自身引用
     */
    locale& operator=(const locale& other);

    /**
     * @brief 移动构造函数
     * @param other 源区域设置
     */
    locale(locale&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源区域设置
     * @return 自身引用
     */
    locale& operator=(locale&& other) noexcept;

    /**
     * @brief 获取"C"区域设置
     * @return "C"区域设置对象
     */
    NEFORCE_NODISCARD static locale classic();

    /**
     * @brief 获取系统区域设置
     * @return 系统默认区域设置
     */
    NEFORCE_NODISCARD static locale system();

    /**
     * @brief 从名称创建区域设置
     * @param name 区域设置名称
     * @return 区域设置对象
     */
    NEFORCE_NODISCARD static locale from_name(const string& name);

    /**
     * @brief 获取区域设置名称
     * @return 区域设置名称
     */
    NEFORCE_NODISCARD const string& name() const noexcept { return name_; }

    /**
     * @brief 获取字符编码
     * @return 编码名称（如"UTF-8"）
     */
    NEFORCE_NODISCARD const string& encoding() const noexcept { return encoding_; }

    /**
     * @brief 相等比较运算符
     * @param o 另一个区域设置
     * @return 名称相同返回true
     */
    NEFORCE_NODISCARD bool operator==(const locale& o) const noexcept { return name_ == o.name_; }

    /**
     * @brief 不等比较运算符
     * @param o 另一个区域设置
     * @return 名称不同返回true
     */
    NEFORCE_NODISCARD bool operator!=(const locale& o) const noexcept { return !(*this == o); }

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

    NEFORCE_NODISCARD bool is_alpha(char32_t cp) const noexcept; ///< 是否为字母
    NEFORCE_NODISCARD bool is_digit(char32_t cp) const noexcept; ///< 是否为数字
    NEFORCE_NODISCARD bool is_alnum(char32_t cp) const noexcept; ///< 是否为字母或数字
    NEFORCE_NODISCARD bool is_space(char32_t cp) const noexcept; ///< 是否为空白字符
    NEFORCE_NODISCARD bool is_upper(char32_t cp) const noexcept; ///< 是否为大写字母
    NEFORCE_NODISCARD bool is_lower(char32_t cp) const noexcept; ///< 是否为小写字母
    NEFORCE_NODISCARD bool is_punct(char32_t cp) const noexcept; ///< 是否为标点符号
    NEFORCE_NODISCARD bool is_print(char32_t cp) const noexcept; ///< 是否为可打印字符

    NEFORCE_NODISCARD char32_t to_upper(char32_t cp) const noexcept; ///< 转换为大写
    NEFORCE_NODISCARD char32_t to_lower(char32_t cp) const noexcept; ///< 转换为小写

    /**
     * @brief 比较两个字符串
     * @param a 第一个字符串
     * @param b 第二个字符串
     * @param strength 比较强度
     * @return 负数表示a<b，0表示a=b，正数表示a>b
     */
    NEFORCE_NODISCARD int compare(const string& a, const string& b,
                                  collate_strength strength = collate_strength::tertiary) const;

    /**
     * @brief 生成排序键
     * @param s 源字符串
     * @return 可用于二进制比较的排序键
     *
     * 排序键可直接用于快速比较，但需要更多存储空间。
     */
    NEFORCE_NODISCARD string collation_key(const string& s) const;

    /**
     * @brief 将UTF-32字符串转换为当前区域设置的多字节字符串
     * @param ucs4 UTF-32字符串
     * @return 多字节字符串
     */
    NEFORCE_NODISCARD string to_multibyte(const u32string& ucs4) const;

    /**
     * @brief 将当前区域设置的多字节字符串转换为UTF-32
     * @param mb 多字节字符串
     * @return UTF-32字符串
     */
    NEFORCE_NODISCARD u32string to_ucs4(const string& mb) const;

    /**
     * @brief 获取系统所有可用的区域设置列表
     * @return 区域设置名称列表
     */
    NEFORCE_NODISCARD static vector<string> available_locales();
};

/** @} */ // Locale

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_LOCALE_HPP__
