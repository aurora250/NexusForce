#ifndef NEFORCE_CORE_SYSTEM_LOCALE_DATA_HPP__
#define NEFORCE_CORE_SYSTEM_LOCALE_DATA_HPP__

/**
 * @file locale_data.hpp
 * @brief 内置区域设置后端使用的区域数据
 *
 * 此文件声明了未链接 ICU 时内置后端使用的区域数据：
 * 数字与货币格式风格、日期时间名称与模式、语言与地区名称、可用区域列表。
 * 数据为自研精简数据，覆盖面小于 ICU 的 CLDR 数据，未收录的语言回退到英语规则。
 */

#include "NeForce/core/container/array.hpp"
#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup LocaleData 区域数据
 * @brief 内置区域设置后端的精简区域数据
 * @{
 */

namespace locale_data {

    /**
     * @struct number_style
     * @brief 数字与货币格式风格
     */
    struct number_style {
        string_view decimal_point;      ///< 小数点
        string_view group_separator;    ///< 千位分隔符
        uint32_t digit_zero;            ///< 本地数字零的码点，0 表示 ASCII 数字
        bool indian_grouping;           ///< 是否使用印度式分组（3;2;2）
        bool currency_before;           ///< 货币符号是否前置
        string_view currency_separator; ///< 货币符号与数值之间的分隔符，空串表示紧邻
    };

    /**
     * @struct calendar_style
     * @brief 日期时间名称与模式
     *
     * 模式使用 ICU / SimpleDateFormat 风格的模式字母。
     */
    struct calendar_style {
        string_view language;                ///< ISO 639 语言代码
        array<string_view, 7> long_days;     ///< 星期全名，索引 0 为星期日
        array<string_view, 7> short_days;    ///< 星期缩写
        array<string_view, 12> long_months;  ///< 月份全名，索引 0 为一月
        array<string_view, 12> short_months; ///< 月份缩写
        string_view am;                      ///< 上午标识
        string_view pm;                      ///< 下午标识
        string_view full_date;               ///< 完整日期模式
        string_view long_date;               ///< 长日期模式
        string_view medium_date;             ///< 中日期模式
        string_view short_date;              ///< 短日期模式
        string_view full_time;               ///< 完整时间模式
        string_view long_time;               ///< 长时间模式
        string_view medium_time;             ///< 中时间模式
        string_view short_time;              ///< 短时间模式
    };

    /**
     * @struct currency_entry
     * @brief 货币符号信息
     */
    struct currency_entry {
        string_view code;        ///< ISO 4217 货币代码
        string_view symbol;      ///< 货币符号
        uint8_t fraction_digits; ///< 小数位数
    };

    /**
     * @struct name_entry
     * @brief 语言或国家地区的本地名与英文名
     */
    struct name_entry {
        string_view code;         ///< ISO 639 语言代码或 ISO 3166 地区代码
        string_view native_name;  ///< 本地名
        string_view english_name; ///< 英文名
    };

    /**
     * @brief 获取语言对应的数字格式风格
     * @param language ISO 639 语言代码
     * @return 数字格式风格，未收录的语言返回默认风格
     */
    NEFORCE_NODISCARD const number_style& number_style_of(string_view language) noexcept;

    /**
     * @brief 获取语言与地区对应的日期时间名称与模式
     * @param language ISO 639 语言代码
     * @param country ISO 3166 地区代码，可为空
     * @return 日期时间风格，地区变体未收录时回退到语言风格，语言未收录时返回英语风格
     */
    NEFORCE_NODISCARD const calendar_style& calendar_style_of(string_view language, string_view country) noexcept;

    /**
     * @brief 查找货币符号信息
     * @param iso_4217 ISO 4217 货币代码
     * @return 货币信息，未收录时返回 nullptr
     */
    NEFORCE_NODISCARD const currency_entry* currency_of(string_view iso_4217) noexcept;

    /**
     * @brief 查找语言名称
     * @param language ISO 639 语言代码
     * @return 名称信息，未收录时返回 nullptr
     */
    NEFORCE_NODISCARD const name_entry* language_name_of(string_view language) noexcept;

    /**
     * @brief 查找国家地区名称
     * @param country ISO 3166 地区代码
     * @return 名称信息，未收录时返回 nullptr
     */
    NEFORCE_NODISCARD const name_entry* country_name_of(string_view country) noexcept;

    /**
     * @brief 获取地区的本币代码
     * @param country ISO 3166 地区代码
     * @return ISO 4217 货币代码，未收录时返回 "USD"
     */
    NEFORCE_NODISCARD string_view home_currency_of(string_view country) noexcept;

    /**
     * @brief 判断语言是否从右向左书写
     * @param language ISO 639 语言代码
     * @return 从右向左返回 true
     */
    NEFORCE_NODISCARD bool is_right_to_left(string_view language) noexcept;

    /**
     * @brief 获取地区的度量系统
     * @param country ISO 3166 地区代码
     * @return 0 公制，1 美制，2 英制
     */
    NEFORCE_NODISCARD uint8_t measurement_system_of(string_view country) noexcept;

    /**
     * @brief 获取地区的一周首日
     * @param country ISO 3166 地区代码
     * @return 1=周日, 2=周一, ..., 7=周六
     */
    NEFORCE_NODISCARD uint8_t first_day_of_week_of(string_view country) noexcept;

    /**
     * @brief 获取语言的百分号
     * @param language ISO 639 语言代码
     * @return 百分号，未收录的语言返回 "%"
     */
    NEFORCE_NODISCARD string_view percent_sign_of(string_view language) noexcept;

    /**
     * @brief 获取语言在显示名中包裹地区的括号
     * @param language ISO 639 语言代码
     * @param before 输出括号前缀
     * @param after 输出括号后缀
     */
    void display_name_brackets(string_view language, string_view& before, string_view& after) noexcept;

    /**
     * @brief 判断语言代码是否为已收录的语言
     * @param language ISO 639 语言代码
     * @return 已收录返回 true
     */
    NEFORCE_NODISCARD bool is_known_language(string_view language) noexcept;

    /**
     * @brief 获取内置后端报告的可用区域列表
     * @return BCP-47 标签数组
     */
    NEFORCE_NODISCARD array<string_view, 114> available_locales() noexcept;

    /**
     * @brief 获取 ISO 3166-1 地区代码列表
     * @return 地区代码数组
     */
    NEFORCE_NODISCARD array<string_view, 254> iso_countries() noexcept;

} // namespace locale_data

/** @} */ // LocaleData

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_LOCALE_DATA_HPP__
