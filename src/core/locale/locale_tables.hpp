#ifndef NEFORCE_CORE_SYSTEM_LOCALE_TABLES_HPP__
#define NEFORCE_CORE_SYSTEM_LOCALE_TABLES_HPP__

/**
 * @file locale_tables.hpp
 * @brief 内置区域设置后端使用的 Unicode 字符表
 *
 * 此文件由 scripts/generate_locale_tables.py 依据 Unicode 字符数据库生成，请勿手工修改。
 * 数据供未链接 ICU 时的内置后端完成字符分类、大小写转换与排序折叠，
 * 分类规则与 ICU4C 一致（依据 Unicode 15.0.0 数据）。
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup LocaleTables Unicode 字符表
 * @brief 内置区域设置后端的字符分类与大小写映射数据
 * @{
 */

namespace locale_tables {

    /// 字母（L*）
    NEFORCE_INLINE17 constexpr uint16_t ALPHA = 0x0001;
    /// 十进制数字（Nd）
    NEFORCE_INLINE17 constexpr uint16_t DIGIT = 0x0002;
    /// 大写字母（Lu）
    NEFORCE_INLINE17 constexpr uint16_t UPPER = 0x0004;
    /// 小写字母（Ll）
    NEFORCE_INLINE17 constexpr uint16_t LOWER = 0x0008;
    /// 首字母大写字母（Lt）
    NEFORCE_INLINE17 constexpr uint16_t TITLECASE = 0x0010;
    /// 标点符号（P*）
    NEFORCE_INLINE17 constexpr uint16_t PUNCT = 0x0020;
    /// 空白分隔符（Zs Zl Zp 与 POSIX 控制空白）
    NEFORCE_INLINE17 constexpr uint16_t SPACE = 0x0040;
    /// 可打印字符
    NEFORCE_INLINE17 constexpr uint16_t PRINTABLE = 0x0080;
    /// Unicode White_Space 属性
    NEFORCE_INLINE17 constexpr uint16_t WHITE_SPACE = 0x0100;

    /**
     * @struct class_range
     * @brief 具有相同类别标志的码点区间
     */
    struct class_range {
        uint32_t first; ///< 区间起始码点
        uint32_t last;  ///< 区间结束码点
        uint16_t mask;  ///< 类别标志位组合
    };

    /**
     * @struct case_entry
     * @brief 码点的大小写映射
     */
    struct case_entry {
        uint32_t code;  ///< 码点
        uint32_t upper; ///< 大写映射
        uint32_t lower; ///< 小写映射
    };

    /**
     * @struct title_entry
     * @brief 码点的首字母大写映射（仅在与大写映射不同时记录）
     */
    struct title_entry {
        uint32_t code;  ///< 码点
        uint32_t title; ///< 首字母大写映射
    };

    /**
     * @struct base_entry
     * @brief 码点的规范分解基础字符，用于排序时忽略重音
     */
    struct base_entry {
        uint32_t code; ///< 码点
        uint32_t base; ///< 规范分解得到的首个非组合字符
    };

    /// 类别区间表，按码点升序排列
    extern const class_range CLASS_RANGES[];
    /// 类别区间表长度
    extern const uint32_t CLASS_RANGE_COUNT;

    /// 大小写映射表，按码点升序排列
    extern const case_entry CASE_ENTRIES[];
    /// 大小写映射表长度
    extern const uint32_t CASE_ENTRY_COUNT;

    /// 首字母大写映射例外表，按码点升序排列
    extern const title_entry TITLE_ENTRIES[];
    /// 首字母大写映射例外表长度
    extern const uint32_t TITLE_ENTRY_COUNT;

    /// 规范分解基础字符表，按码点升序排列
    extern const base_entry BASE_ENTRIES[];
    /// 规范分解基础字符表长度
    extern const uint32_t BASE_ENTRY_COUNT;

    /// 生成这些表所用的 Unicode 版本
    extern const char UNICODE_VERSION[];

} // namespace locale_tables

/** @} */ // LocaleTables

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_LOCALE_TABLES_HPP__
