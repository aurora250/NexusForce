#!/usr/bin/env python3
"""Generate the Unicode tables used by the locale component's built-in (non-ICU) backend.

The tables are derived from the Unicode character database that ships with CPython
(``unicodedata``), so the script is self contained and reproducible without ICU or any
third party module::

    python3 scripts/generate_locale_tables.py

It rewrites ``src/core/system/locale_tables.hpp`` and ``src/core/system/locale_tables.cpp``.

The classification rules mirror ICU4C exactly (verified against libicuuc with
``bench_out/p0/icu_semantics.cpp``):

===========================  ==========================================================
bit                          true for
===========================  ==========================================================
``alpha``                    L* (Lu Ll Lt Lm Lo)
``digit``                    Nd
``upper`` / ``lower``        Lu / Ll
``titlecase``                Lt
``punct``                    P* (Pc Pd Ps Pe Pi Pf Po)
``space``                    Zs Zl Zp and U+0009..U+000D, U+001C..U+001F (``u_isspace``)
``print``                    L* M* N* P* S* Zs Zl Zp (``u_isprint``: no Cc Cf Co Cs Cn)
``white_space``              the Unicode White_Space property (``u_isUWhiteSpace``)
===========================  ==========================================================

Codepoints without a table entry answer "no" to every predicate, which is what ICU
reports for unassigned codepoints. The table therefore only has to store the assigned
and printable part of the code space.
"""

from __future__ import annotations

import sys
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "src" / "core" / "locale" / "locale_tables.hpp"
SOURCE = ROOT / "src" / "core" / "locale" / "locale_tables.cpp"

MAX_CODEPOINT = 0x10FFFF
SURROGATE_FIRST = 0xD800
SURROGATE_LAST = 0xDFFF

ALPHA = 0x0001
DIGIT = 0x0002
UPPER = 0x0004
LOWER = 0x0008
TITLECASE = 0x0010
PUNCT = 0x0020
SPACE = 0x0040
PRINT = 0x0080
WHITE_SPACE = 0x0100

# ICU's u_isspace: the separators plus the POSIX control whitespace.
CONTROL_SPACE = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x1C, 0x1D, 0x1E, 0x1F, 0x85}

# Unicode White_Space property.
WHITE_SPACE_RANGES = [
    (0x0009, 0x000D),
    (0x0020, 0x0020),
    (0x0085, 0x0085),
    (0x00A0, 0x00A0),
    (0x1680, 0x1680),
    (0x2000, 0x200A),
    (0x2028, 0x2029),
    (0x202F, 0x202F),
    (0x205F, 0x205F),
    (0x3000, 0x3000),
]

# CPython's str.upper()/str.lower()/str.title() implement full case mappings, which expand a
# single character into several (for example U+00DF -> "SS", U+0130 -> "i" + combining dot).
# libicuuc applies the simple mappings from UnicodeData.txt instead, so the expanding characters
# are listed explicitly. The table was verified codepoint by codepoint against libicuuc.
SIMPLE_CASE_OVERRIDES = {
    0x00DF: (0x00DF, 0x00DF, 0x00DF),  # ß
    0x0130: (0x0130, 0x0069, 0x0130),  # İ
    0x0149: (0x0149, 0x0149, 0x0149),  # ŉ
    0x01F0: (0x01F0, 0x01F0, 0x01F0),  # ǰ
    0x0390: (0x0390, 0x0390, 0x0390),  # ΐ
    0x03B0: (0x03B0, 0x03B0, 0x03B0),  # ΰ
    0x0587: (0x0587, 0x0587, 0x0587),  # և
    0x1E96: (0x1E96, 0x1E96, 0x1E96),
    0x1E97: (0x1E97, 0x1E97, 0x1E97),
    0x1E98: (0x1E98, 0x1E98, 0x1E98),
    0x1E99: (0x1E99, 0x1E99, 0x1E99),
    0x1E9A: (0x1E9A, 0x1E9A, 0x1E9A),
    0x1F50: (0x1F50, 0x1F50, 0x1F50),
    0x1F52: (0x1F52, 0x1F52, 0x1F52),
    0x1F54: (0x1F54, 0x1F54, 0x1F54),
    0x1F56: (0x1F56, 0x1F56, 0x1F56),
    0x1FB2: (0x1FB2, 0x1FB2, 0x1FB2),
    0x1FB3: (0x1FBC, 0x1FB3, 0x1FBC),
    0x1FB4: (0x1FB4, 0x1FB4, 0x1FB4),
    0x1FB6: (0x1FB6, 0x1FB6, 0x1FB6),
    0x1FB7: (0x1FB7, 0x1FB7, 0x1FB7),
    0x1FBC: (0x1FBC, 0x1FB3, 0x1FBC),
    0x1FC2: (0x1FC2, 0x1FC2, 0x1FC2),
    0x1FC3: (0x1FCC, 0x1FC3, 0x1FCC),
    0x1FC4: (0x1FC4, 0x1FC4, 0x1FC4),
    0x1FC6: (0x1FC6, 0x1FC6, 0x1FC6),
    0x1FC7: (0x1FC7, 0x1FC7, 0x1FC7),
    0x1FCC: (0x1FCC, 0x1FC3, 0x1FCC),
    0x1FD2: (0x1FD2, 0x1FD2, 0x1FD2),
    0x1FD3: (0x1FD3, 0x1FD3, 0x1FD3),
    0x1FD6: (0x1FD6, 0x1FD6, 0x1FD6),
    0x1FD7: (0x1FD7, 0x1FD7, 0x1FD7),
    0x1FE2: (0x1FE2, 0x1FE2, 0x1FE2),
    0x1FE3: (0x1FE3, 0x1FE3, 0x1FE3),
    0x1FE4: (0x1FE4, 0x1FE4, 0x1FE4),
    0x1FE6: (0x1FE6, 0x1FE6, 0x1FE6),
    0x1FE7: (0x1FE7, 0x1FE7, 0x1FE7),
    0x1FF2: (0x1FF2, 0x1FF2, 0x1FF2),
    0x1FF3: (0x1FFC, 0x1FF3, 0x1FFC),
    0x1FF4: (0x1FF4, 0x1FF4, 0x1FF4),
    0x1FF6: (0x1FF6, 0x1FF6, 0x1FF6),
    0x1FF7: (0x1FF7, 0x1FF7, 0x1FF7),
    0x1FFC: (0x1FFC, 0x1FF3, 0x1FFC),
    0xFB00: (0xFB00, 0xFB00, 0xFB00),
    0xFB01: (0xFB01, 0xFB01, 0xFB01),
    0xFB02: (0xFB02, 0xFB02, 0xFB02),
    0xFB03: (0xFB03, 0xFB03, 0xFB03),
    0xFB04: (0xFB04, 0xFB04, 0xFB04),
    0xFB05: (0xFB05, 0xFB05, 0xFB05),
    0xFB06: (0xFB06, 0xFB06, 0xFB06),
    0xFB13: (0xFB13, 0xFB13, 0xFB13),
    0xFB14: (0xFB14, 0xFB14, 0xFB14),
    0xFB15: (0xFB15, 0xFB15, 0xFB15),
    0xFB16: (0xFB16, 0xFB16, 0xFB16),
    0xFB17: (0xFB17, 0xFB17, 0xFB17),
}


# The Greek iota subscript characters carry a simple uppercase mapping that equals the titlecase
# character (U+1F80 -> U+1F88); CPython expands them into a two character full mapping instead.
for _base in (0x1F80, 0x1F90, 0x1FA0):
    for _offset in range(8):
        _codepoint = _base + _offset
        SIMPLE_CASE_OVERRIDES[_codepoint] = (_codepoint + 8, _codepoint, _codepoint + 8)


def in_ranges(codepoint: int, ranges: list[tuple[int, int]]) -> bool:
    for first, last in ranges:
        if first <= codepoint <= last:
            return True
    return False


def category_class(category: str) -> int:
    mask = 0
    major = category[0]

    if major == "L":
        mask |= ALPHA
    elif category == "Nd":
        mask |= DIGIT

    if category == "Lu":
        mask |= UPPER
    elif category == "Ll":
        mask |= LOWER
    elif category == "Lt":
        mask |= TITLECASE

    if major == "P":
        mask |= PUNCT

    return mask


def mask_for(codepoint: int) -> int:
    category = unicodedata.category(chr(codepoint))
    if category == "Cn":
        return 0

    mask = category_class(category)

    if category in ("Zs", "Zl", "Zp") or codepoint in CONTROL_SPACE:
        mask |= SPACE

    if category[0] not in ("C",) or category in ("Zs", "Zl", "Zp"):
        mask |= PRINT

    if in_ranges(codepoint, WHITE_SPACE_RANGES):
        mask |= WHITE_SPACE

    return mask


def simple_case(codepoint: int) -> tuple[int, int, int]:
    override = SIMPLE_CASE_OVERRIDES.get(codepoint)
    if override is not None:
        return override

    char = chr(codepoint)
    upper = char.upper()
    lower = char.lower()
    title = char.title()

    upper = codepoint if len(upper) != 1 else ord(upper)
    lower = codepoint if len(lower) != 1 else ord(lower)
    title = upper if len(title) != 1 else ord(title)
    return upper, lower, title


def canonical_base(codepoint: int, depth: int = 0) -> int:
    if depth > 8:
        return codepoint

    decomposition = unicodedata.decomposition(chr(codepoint))
    if not decomposition or decomposition.startswith("<"):
        return codepoint

    first = int(decomposition.split(" ", 1)[0], 16)
    if first == codepoint:
        return codepoint
    return canonical_base(first, depth + 1)


def class_ranges() -> list[tuple[int, int, int]]:
    ranges: list[tuple[int, int, int]] = []
    start = -1
    mask = 0

    for codepoint in range(0, MAX_CODEPOINT + 1):
        if SURROGATE_FIRST <= codepoint <= SURROGATE_LAST:
            current = 0
        else:
            current = mask_for(codepoint)

        if current == 0:
            if start >= 0:
                ranges.append((start, codepoint - 1, mask))
                start = -1
                mask = 0
            continue

        if start < 0:
            start = codepoint
            mask = current
        elif current != mask:
            ranges.append((start, codepoint - 1, mask))
            start = codepoint
            mask = current

    if start >= 0:
        ranges.append((start, MAX_CODEPOINT, mask))

    return ranges


def case_entries() -> tuple[list[tuple[int, int, int]], list[tuple[int, int]]]:
    entries: list[tuple[int, int, int]] = []
    titles: list[tuple[int, int]] = []

    for codepoint in range(0, MAX_CODEPOINT + 1):
        if SURROGATE_FIRST <= codepoint <= SURROGATE_LAST:
            continue
        if unicodedata.category(chr(codepoint)) == "Cn":
            continue

        upper, lower, title = simple_case(codepoint)
        if upper != codepoint or lower != codepoint:
            entries.append((codepoint, upper, lower))
        if title != upper:
            titles.append((codepoint, title))

    return entries, titles


def base_entries() -> list[tuple[int, int]]:
    entries: list[tuple[int, int]] = []

    for codepoint in range(0, MAX_CODEPOINT + 1):
        if SURROGATE_FIRST <= codepoint <= SURROGATE_LAST:
            continue
        if unicodedata.category(chr(codepoint)) == "Cn":
            continue

        base = canonical_base(codepoint)
        if base != codepoint:
            entries.append((codepoint, base))

    return entries


def write_header(ranges: int, cases: int, titles: int, bases: int) -> None:
    text = """#ifndef NEFORCE_CORE_SYSTEM_LOCALE_TABLES_HPP__
#define NEFORCE_CORE_SYSTEM_LOCALE_TABLES_HPP__

/**
 * @file locale_tables.hpp
 * @brief 内置区域设置后端使用的 Unicode 字符表
 *
 * 此文件由 scripts/generate_locale_tables.py 依据 Unicode 字符数据库生成，请勿手工修改。
 * 数据供未链接 ICU 时的内置后端完成字符分类、大小写转换与排序折叠，
 * 分类规则与 ICU4C 一致（依据 Unicode __UNICODE_VERSION__ 数据）。
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
"""
    text = text.replace("__UNICODE_VERSION__", unicodedata.unidata_version)
    HEADER.write_text(text, encoding="utf-8")
    print(f"wrote {HEADER.relative_to(ROOT)} ({ranges} ranges, {cases} cases, {titles} titles, {bases} bases)")


def write_source(
    ranges: list[tuple[int, int, int]],
    cases: list[tuple[int, int, int]],
    titles: list[tuple[int, int]],
    bases: list[tuple[int, int]],
) -> None:
    out: list[str] = []
    out.append("// Generated by scripts/generate_locale_tables.py -- do not edit by hand.")
    out.append(f"// Source of truth: the Unicode character database of CPython (Unicode {unicodedata.unidata_version}).")
    out.append("")
    out.append('#include "locale_tables.hpp"')
    out.append("NEFORCE_BEGIN_NAMESPACE__")
    out.append("")
    out.append("namespace locale_tables {")
    out.append("")

    out.append("// Classification ranges: maximal runs of codepoints that share one category mask.")
    out.append("// Codepoints outside every range answer \"no\" to every predicate, exactly like an")
    out.append("// unassigned codepoint does in ICU, so the table only stores the assigned and printable part.")
    out.append("const class_range CLASS_RANGES[] = {")
    for first, last, mask in ranges:
        flags = " | ".join(
            name
            for name, bit in (
                ("ALPHA", ALPHA),
                ("DIGIT", DIGIT),
                ("UPPER", UPPER),
                ("LOWER", LOWER),
                ("TITLECASE", TITLECASE),
                ("PUNCT", PUNCT),
                ("SPACE", SPACE),
                ("PRINTABLE", PRINT),
                ("WHITE_SPACE", WHITE_SPACE),
            )
            if mask & bit
        )
        out.append(f"    {{0x{first:04X}, 0x{last:04X}, {flags}}},")
    out.append("};")
    out.append("const uint32_t CLASS_RANGE_COUNT = sizeof(CLASS_RANGES) / sizeof(CLASS_RANGES[0]);")
    out.append("")

    out.append("// Simple case mappings: only codepoints that actually change are listed.")
    out.append("const case_entry CASE_ENTRIES[] = {")
    for code, upper, lower in cases:
        out.append(f"    {{0x{code:04X}, 0x{upper:04X}, 0x{lower:04X}}},")
    out.append("};")
    out.append("const uint32_t CASE_ENTRY_COUNT = sizeof(CASE_ENTRIES) / sizeof(CASE_ENTRIES[0]);")
    out.append("")

    out.append("// Titlecase mappings that differ from the uppercase mapping.")
    out.append("const title_entry TITLE_ENTRIES[] = {")
    for code, title in titles:
        out.append(f"    {{0x{code:04X}, 0x{title:04X}}},")
    out.append("};")
    out.append("const uint32_t TITLE_ENTRY_COUNT = sizeof(TITLE_ENTRIES) / sizeof(TITLE_ENTRIES[0]);")
    out.append("")

    out.append("// Canonical decomposition head: the first non-combining character of the full")
    out.append("// canonical decomposition, used to ignore accents at the primary collation level.")
    out.append("const base_entry BASE_ENTRIES[] = {")
    for code, base in bases:
        out.append(f"    {{0x{code:04X}, 0x{base:04X}}},")
    out.append("};")
    out.append("const uint32_t BASE_ENTRY_COUNT = sizeof(BASE_ENTRIES) / sizeof(BASE_ENTRIES[0]);")
    out.append("")

    out.append(f'const char UNICODE_VERSION[] = "{unicodedata.unidata_version}";')
    out.append("")
    out.append("} // namespace locale_tables")
    out.append("")
    out.append("NEFORCE_END_NAMESPACE__")
    out.append("")

    SOURCE.write_text("\n".join(out), encoding="utf-8")
    print(f"wrote {SOURCE.relative_to(ROOT)}")


def main() -> int:
    print(f"unicodedata version: {unicodedata.unidata_version}")

    ranges = class_ranges()
    cases, titles = case_entries()
    bases = base_entries()

    write_header(len(ranges), len(cases), len(titles), len(bases))
    write_source(ranges, cases, titles, bases)
    return 0


if __name__ == "__main__":
    sys.exit(main())
