#ifndef NEFORCE_CORE_TIME_DATETIME_HPP__
#define NEFORCE_CORE_TIME_DATETIME_HPP__

/**
 * @file datetime.hpp
 * @brief 日期时间处理库
 *
 * 此文件提供了完整的日期时间处理功能，包括：
 * - 日期类（年、月、日）
 * - 时间类（时、分、秒）
 * - 日期时间类（日期+时间+时区）
 * - 时间戳类
 * - 儒略日转换
 * - 多种格式的解析和格式化
 */

#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup DateTime 日期时间
 * @brief 日期时间处理功能
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下日期时间相关标准规范：
 *
 * **公历与日期计算标准：**
 * - **ISO 8601-1:2019**：日期和时间 — 信息交换的表示法 — 第1部分：基本规则
 *   https://www.iso.org/standard/70907.html
 * - **ISO 8601-2:2019**：日期和时间 — 信息交换的表示法 — 第2部分：扩展
 *   https://www.iso.org/standard/70908.html
 *
 * **互联网时间格式标准：**
 * - **IETF RFC 3339**：互联网上的日期和时间格式（ISO 8601 配置文件）
 *   https://www.rfc-editor.org/rfc/rfc3339.html
 * - **IETF RFC 1123**：互联网主机要求 — 应用和支持（§5.2.14 日期时间格式）
 *   https://www.rfc-editor.org/rfc/rfc1123.html#section-5.2.14
 * - **IETF RFC 2616**：HTTP/1.1（§3.3.1 完整日期，已由 RFC 7231 更新）
 *   https://www.rfc-editor.org/rfc/rfc2616.html#section-3.3.1
 *
 * **时间戳与系统时钟标准：**
 * - **POSIX.1-2017 (IEEE Std 1003.1)**：Unix 时间戳定义
 *   https://pubs.opengroup.org/onlinepubs/9699919799/
 * - **ISO/IEC 9899:2018**：C 语言标准（time_t 类型定义）
 *   https://www.iso.org/standard/74528.html
 *
 * **儒略日计算标准：**
 * - **国际天文学联合会 (IAU) 标准**：儒略日计算公式（Fliegel & Van Flandern, 1968）
 *
 * @section format_specifications 格式规范
 * | 格式类型         | 示例                                  | 标准引用                     |
 * |------------------|---------------------------------------|------------------------------|
 * | ISO 8601 基本    | 2024-01-15T14:30:00                   | ISO 8601-1:2019 §5.4         |
 * | ISO 8601 UTC     | 2024-01-15T14:30:00Z                  | ISO 8601-1:2019 §5.4         |
 * | ISO 8601 带时区  | 2024-01-15T14:30:00+08:00             | ISO 8601-1:2019 §5.4         |
 * | RFC 3339         | 2024-01-15T14:30:00+08:00             | RFC 3339 §5.6                |
 * | RFC 1123 (GMT)   | Mon, 15 Jan 2024 14:30:00 GMT         | RFC 1123 §5.2.14             |
 * | 简单格式         | 2024-01-15 14:30:00                   | 非标准（用于内部表示）        |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 纪元起始          | 1970-01-01 00:00:00 UTC (Unix 纪元)       |
 * | 年份范围          | 1900-9999                                 |
 * | 闰年规则          | 公历闰年（能被4整除但不能被100整除，或能被400整除） |
 * | 儒略日基准        | 公元前4713年1月1日中午12点                |
 * | 时区偏移范围      | ±12:00                                    |
 * | 时间戳精度        | 秒级                                      |
 *
 * @section date_calculation 日期计算参考
 * - **星期计算**：采用 Zeller 同余式变体
 * - **儒略日转换**：采用 Fliegel & Van Flandern 算法
 * - **闰年判断**：公历规则（1582年后）
 *
 * @note 所有日期验证符合公历规则，年份范围限制为 1900-9999。
 *       儒略日转换算法支持公元纪年，结果与 IAU 标准一致。
 *
 * @see https://www.iso.org/iso-8601-date-and-time-format.html
 * @see https://www.ietf.org/rfc/rfc3339.txt
 * @see https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_16
 * @{
 */

/**
 * @class date
 * @brief 日期类
 *
 * 表示公历日期，支持日期间隔计算、儒略日转换、日期验证等功能。
 * 年份范围：1900-9999
 */
class NEFORCE_API date : public iobject<date>, public icommon<date> {
public:
    using date_type = int32_t; ///< 日期分量类型

private:
    date_type year_ = 1970; ///< 年份
    date_type month_ = 1;   ///< 月份（1-12）
    date_type day_ = 1;     ///< 日期（1-31）

public:
    /**
     * @brief 每月天数表（非闰年）
     */
    static constexpr int32_t month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

public:
    /**
     * @brief 默认构造函数，创建1970-01-01
     */
    constexpr date() noexcept = default;

    NEFORCE_CONSTEXPR20 ~date() = default;

    /**
     * @brief 构造函数
     * @param year 年份
     * @param month 月份
     * @param day 日期
     *
     * 如果日期无效，将保持默认值1970-01-01。
     */
    constexpr explicit date(date_type year, const date_type month, const date_type day) noexcept {
        if (is_valid(year, month, day)) {
            year_ = year;
            month_ = month;
            day_ = day;
        }
    }

    constexpr date(const date&) noexcept = default;
    constexpr date& operator=(const date&) noexcept = default;
    constexpr date(date&&) noexcept = default;
    constexpr date& operator=(date&&) noexcept = default;

    /**
     * @brief 获取年份
     * @return 年份
     */
    NEFORCE_NODISCARD constexpr date_type year() const noexcept { return year_; }

    /**
     * @brief 获取月份
     * @return 月份
     */
    NEFORCE_NODISCARD constexpr date_type month() const noexcept { return month_; }

    /**
     * @brief 获取日期
     * @return 日期
     */
    NEFORCE_NODISCARD constexpr date_type day() const noexcept { return day_; }

    /**
     * @brief 检查日期是否有效
     * @param year 年份
     * @param month 月份
     * @param day 日期
     * @return 是否有效
     */
    static constexpr bool is_valid(date_type year, date_type month, date_type day) noexcept {
        if (year < 1900 || year > 9999) {
            return false;
        }
        if (month < 1 || month > 12) {
            return false;
        }
        return day > 0 && day <= days_of_month(year, month);
    }

    /**
     * @brief 检查日期是否有效
     * @return 是否有效
     */
    constexpr bool is_valid() const noexcept { return is_valid(year_, month_, day_); }

    /**
     * @brief 获取纪元起始日期（1970-01-01）
     * @return 纪元起始日期
     */
    static constexpr date epoch() noexcept { return date{}; }

    /**
     * @brief 检查是否为闰年
     * @param year 年份
     * @return 是否为闰年
     */
    static constexpr bool is_leap_year(const date_type year) noexcept {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    /**
     * @brief 获取星期几
     * @return 0=周日，1=周一，...，6=周六
     */
    NEFORCE_NODISCARD constexpr date_type days_of_week() const noexcept {
        date_type y = year_;
        date_type m = month_;
        const date_type d = day_;
        if (m < 3) {
            y--;
            m += 12;
        }
        return (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
    }

    /**
     * @brief 获取指定年月的天数
     * @param year 年份
     * @param month 月份
     * @return 天数
     */
    static constexpr date_type days_of_month(const date_type year, const date_type month) noexcept {
        date_type day = month_days[month - 1];
        if (month == 2 && is_leap_year(year)) {
            day += 1;
        }
        return day;
    }

    /**
     * @brief 获取一年中的第几天
     * @return 天数（1-366）
     */
    NEFORCE_NODISCARD constexpr date_type days_of_year() const noexcept {
        date_type days = 0;
        for (date_type i = 1; i < month_; ++i) {
            days += days_of_month(year_, i);
        }
        return days + day_;
    }

    /**
     * @brief 转换为儒略日
     * @return 儒略日数
     */
    constexpr int64_t to_julian_day() const noexcept {
        const date_type a = (14 - month_) / 12;
        const date_type year = year_ + 4800 - a;
        const date_type month = month_ + 12 * a - 3;
        return day_ + (153 * month + 2) / 5 + 365 * year + year / 4 - year / 100 + year / 400 - 32045;
    }

    /**
     * @brief 从儒略日转换
     * @param julian_day 儒略日数
     * @return 对应的日期
     */
    static constexpr date from_julian_day(const int64_t julian_day) noexcept {
        const int64_t a = julian_day + 32044;
        const int64_t b = (4 * a + 3) / 146097;
        const int64_t c = a - (146097 * b) / 4;
        const int64_t d = (4 * c + 3) / 1461;
        const int64_t e = c - (1461 * d) / 4;
        const int64_t m = (5 * e + 2) / 153;

        const date_type day = static_cast<date_type>(e - (153 * m + 2) / 5 + 1);
        const date_type month = static_cast<date_type>(m + 3 - 12 * (m / 10));
        const date_type year = static_cast<date_type>(100 * b + d - 4800 + (m / 10));

        return date(year, month, day);
    }

    /**
     * @brief 重置为纪元起始日期
     */
    constexpr void clear() noexcept {
        year_ = 1970;
        month_ = 1;
        day_ = 1;
    }

    /**
     * @brief 相等比较
     */
    constexpr bool operator==(const date& rhs) const noexcept {
        return year_ == rhs.year_ && month_ == rhs.month_ && day_ == rhs.day_;
    }

    /**
     * @brief 小于比较
     */
    constexpr bool operator<(const date& rhs) const noexcept {
        if (year_ < rhs.year_) {
            return true;
        }
        if (year_ == rhs.year_ && month_ < rhs.month_) {
            return true;
        }
        if (year_ == rhs.year_ && month_ == rhs.month_ && day_ < rhs.day_) {
            return true;
        }
        return false;
    }

    /**
     * @brief 日期加天数
     * @param day 要加的天数
     * @return 自身引用
     */
    constexpr date& operator+=(const date_type day) noexcept {
        if (day == 0) {
            return *this;
        }
        if (day < 0) {
            return *this -= -day;
        }

        if (day > 365) {
            const int64_t jd = to_julian_day() + day;
            *this = from_julian_day(jd);
            return *this;
        }

        date_type remaining = day;
        while (remaining > 0) {
            const date_type days_in_month = days_of_month(year_, month_);
            const date_type available = days_in_month - day_ + 1;

            if (remaining < available) {
                day_ += remaining;
                break;
            }

            remaining -= available;
            day_ = 1;
            if (++month_ > 12) {
                month_ = 1;
                ++year_;
            }
        }
        return *this;
    }

    /**
     * @brief 日期减天数
     * @param day 要减的天数
     * @return 自身引用
     */
    constexpr date& operator-=(const date_type day) noexcept {
        if (day < 0) {
            return *this += -day;
        }

        day_ -= day;
        while (day_ <= 0) {
            if (--month_ == 0) {
                month_ = 12;
                year_--;
            }
            day_ += days_of_month(year_, month_);
        }
        return *this;
    }

    /**
     * @brief 日期加天数
     * @param day 天数
     * @return 新日期
     */
    constexpr date operator+(const date_type day) const noexcept {
        date ret(*this);
        ret += day;
        return ret;
    }

    /**
     * @brief 日期减天数
     * @param day 天数
     * @return 新日期
     */
    constexpr date operator-(const date_type day) const noexcept {
        date ret(*this);
        ret -= day;
        return ret;
    }

    /**
     * @brief 前置递增（加1天）
     */
    constexpr date& operator++() {
        *this += 1;
        return *this;
    }

    /**
     * @brief 后置递增（加1天）
     */
    constexpr date operator++(int) {
        const date ret(*this);
        *this += 1;
        return ret;
    }

    /**
     * @brief 日期差（天数）
     * @param other 另一个日期
     * @return 相差的天数
     */
    constexpr date_type operator-(const date& other) const noexcept {
        return static_cast<date_type>(to_julian_day() - other.to_julian_day());
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<date_type> hasher;
        return hasher(day()) ^ hasher(month()) ^ hasher(year());
    }

    /**
     * @brief 转换为字符串
     * @return 格式为 YYYY-MM-DD 的字符串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const {
        return _NEFORCE format("{:04d}-{:02d}-{:02d}", year(), month(), day());
    }

    /**
     * @brief 从字符串解析
     * @param view 格式为 YYYY-MM-DD 的字符串
     * @return 解析得到的日期
     * @throws value_exception 格式错误时抛出
     */
    NEFORCE_NODISCARD constexpr static date parse(const string_view view) {
        if (view.size() != 10 || view[4] != '-' || view[7] != '-') {
            NEFORCE_THROW_EXCEPTION(value_exception("Wrong string formation."));
        }
        const date_type year = integer32::parse(view.substr(0, 4)).value();
        const date_type month = integer32::parse(view.substr(5, 2)).value();
        const date_type day = integer32::parse(view.substr(8, 2)).value();
        return date(year, month, day);
    }

    /**
     * @brief 交换两个日期
     * @param other 另一个日期
     */
    constexpr void swap(date& other) noexcept {
        _NEFORCE swap(year_, other.year_);
        _NEFORCE swap(month_, other.month_);
        _NEFORCE swap(day_, other.day_);
    }
};


/**
 * @class time
 * @brief 时间类
 *
 * 表示一天中的时间（时、分、秒），支持时间间隔计算。
 */
class NEFORCE_API time : public iobject<time>, public icommon<time> {
public:
    using time_type = int32_t; ///< 时间分量类型

private:
    time_type hours_ = 0;   ///< 小时（0-23）
    time_type minutes_ = 0; ///< 分钟（0-59）
    time_type seconds_ = 0; ///< 秒（0-59）

public:
    /**
     * @brief 默认构造函数，创建00:00:00
     */
    constexpr time() noexcept = default;

    /**
     * @brief 析构函数
     */
    NEFORCE_CONSTEXPR20 ~time() = default;

    /**
     * @brief 构造函数
     * @param hour 小时
     * @param minute 分钟
     * @param second 秒
     */
    constexpr explicit time(const time_type hour, const time_type minute, const time_type second) noexcept {
        if (is_valid(hour, minute, second)) {
            hours_ = hour;
            minutes_ = minute;
            seconds_ = second;
        }
    }

    constexpr time(const time&) noexcept = default;
    constexpr time& operator=(const time&) noexcept = default;
    constexpr time(time&&) noexcept = default;
    constexpr time& operator=(time&&) noexcept = default;

    /**
     * @brief 获取小时
     * @return 小时
     */
    NEFORCE_NODISCARD constexpr time_type hours() const noexcept { return hours_; }

    /**
     * @brief 获取分钟
     * @return 分钟
     */
    NEFORCE_NODISCARD constexpr time_type minutes() const noexcept { return minutes_; }

    /**
     * @brief 获取秒
     * @return 秒
     */
    NEFORCE_NODISCARD constexpr time_type seconds() const noexcept { return seconds_; }

    /**
     * @brief 检查时间是否有效
     * @param hour 小时
     * @param minute 分钟
     * @param second 秒
     * @return 是否有效
     */
    static constexpr bool is_valid(time_type hour, time_type minute, time_type second) noexcept {
        return hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60;
    }

    /**
     * @brief 检查时间是否有效
     * @return 是否有效
     */
    constexpr bool is_valid() const noexcept { return is_valid(hours_, minutes_, seconds_); }

    /**
     * @brief 重置为00:00:00
     */
    constexpr void clear() noexcept {
        hours_ = 0;
        minutes_ = 0;
        seconds_ = 0;
    }

    /**
     * @brief 转换为总秒数
     * @return 从00:00:00开始的秒数
     */
    NEFORCE_NODISCARD constexpr time_type to_seconds() const noexcept {
        return hours_ * 3600 + minutes_ * 60 + seconds_;
    }

    /**
     * @brief 相等比较
     */
    constexpr bool operator==(const time& other) const noexcept {
        return hours_ == other.hours_ && minutes_ == other.minutes_ && seconds_ == other.seconds_;
    }

    /**
     * @brief 小于比较
     */
    constexpr bool operator<(const time& other) const noexcept {
        if (hours_ < other.hours_) {
            return true;
        }
        if (hours_ == other.hours_) {
            if (minutes_ < other.minutes_) {
                return true;
            }
            if (minutes_ == other.minutes_ && seconds_ < other.seconds_) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 时间加秒数
     * @param seconds 要加的秒数
     * @return 自身引用
     */
    constexpr time& operator+=(const time_type seconds) {
        if (seconds < 0) {
            return *this -= -seconds;
        }

        seconds_ += seconds;
        const time_type extra_min = seconds_ / 60;
        seconds_ %= 60;

        minutes_ += extra_min;
        const time_type extra_hour = minutes_ / 60;
        minutes_ %= 60;

        hours_ += extra_hour;
        hours_ %= 24;

        return *this;
    }

    /**
     * @brief 时间减秒数
     * @param seconds 要减的秒数
     * @return 自身引用
     */
    constexpr time& operator-=(const time_type seconds) noexcept {
        if (seconds < 0) {
            return *this += -seconds;
        }

        int64_t total_sec = to_seconds() - seconds;
        total_sec %= 86400;
        if (total_sec < 0) {
            total_sec += 86400;
        }

        hours_ = static_cast<time_type>(total_sec / 3600);
        minutes_ = static_cast<time_type>((total_sec % 3600) / 60);
        seconds_ = static_cast<time_type>(total_sec % 60);
        return *this;
    }

    /**
     * @brief 时间加秒数
     * @param seconds 秒数
     * @return 新时间
     */
    constexpr time operator+(const time_type seconds) const noexcept {
        time ret(*this);
        ret += seconds;
        return ret;
    }

    /**
     * @brief 时间减秒数
     * @param seconds 秒数
     * @return 新时间
     */
    constexpr time operator-(const time_type seconds) const noexcept {
        time ret(*this);
        ret -= seconds;
        return ret;
    }

    /**
     * @brief 前置递增（加1秒）
     */
    constexpr time& operator++() { return *this += 1; }

    /**
     * @brief 后置递增（加1秒）
     */
    constexpr time operator++(int) {
        const time ret(*this);
        *this += 1;
        return ret;
    }

    /**
     * @brief 前置递减（减1秒）
     */
    constexpr time& operator--() { return *this -= 1; }

    /**
     * @brief 后置递减（减1秒）
     */
    constexpr time operator--(int) {
        const time ret(*this);
        *this -= 1;
        return ret;
    }

    /**
     * @brief 时间差（秒数）
     * @param other 另一个时间
     * @return 相差的秒数
     */
    constexpr time_type operator-(const time& other) const noexcept {
        time_type sec_diff = (hours_ - other.hours_) * 3600;
        sec_diff += (minutes_ - other.minutes_) * 60;
        sec_diff += (seconds_ - other.seconds_);
        return sec_diff;
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<time_type> hasher;
        return hasher(hours()) ^ hasher(minutes()) ^ hasher(seconds());
    }

    /**
     * @brief 转换为字符串
     * @return 格式为 HH:MM:SS 的字符串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const {
        return _NEFORCE format("{:02d}:{:02d}:{:02d}", hours(), minutes(), seconds());
    }

    /**
     * @brief 从字符串解析
     * @param view 格式为 HH:MM:SS 的字符串
     * @return 解析得到的时间
     * @throws value_exception 格式错误时抛出
     */
    NEFORCE_NODISCARD static constexpr time parse(const string_view view) {
        if (view.size() != 8 || view[2] != ':' || view[5] != ':') {
            NEFORCE_THROW_EXCEPTION(value_exception("Wrong string formation."));
        }
        const time_type hour = integer32::parse(view.substr(0, 2)).value();
        const time_type minute = integer32::parse(view.substr(3, 2)).value();
        const time_type second = integer32::parse(view.substr(6, 2)).value();
        return time(hour, minute, second);
    }

    /**
     * @brief 交换两个时间
     * @param other 另一个时间
     */
    constexpr void swap(time& other) noexcept {
        _NEFORCE swap(hours_, other.hours_);
        _NEFORCE swap(minutes_, other.minutes_);
        _NEFORCE swap(seconds_, other.seconds_);
    }
};


/**
 * @class datetime
 * @brief 日期时间类
 *
 * 组合日期和时间，支持时区处理。
 * 提供多种格式的解析和格式化：
 * - ISO格式（YYYY-MM-DDTHH:MM:SS）
 * - ISO带时区（YYYY-MM-DDTHH:MM:SS±HH:MM 或 Z）
 * - GMT格式（RFC 1123）
 * - 简单格式（YYYY-MM-DD HH:MM:SS）
 */
class NEFORCE_API datetime : public iobject<datetime>, public icommon<datetime> {
public:
    using date_type = _NEFORCE date::date_type; ///< 日期分量类型
    using time_type = _NEFORCE time::time_type; ///< 时间分量类型

private:
    _NEFORCE date date_{};       ///< 日期部分
    _NEFORCE time time_{};       ///< 时间部分
    int64_t offset_seconds_ = 0; ///< 时区偏移
    bool has_timezone_ = false;  ///< 是否有时区信息

private:
    /**
     * @brief 将月份名称转换为月份数字
     * @param view 月份名称（如"Jan"）
     * @return 月份数字（1-12），失败返回0
     */
    static constexpr int months_to_int(const string_view view) {
        constexpr string_view months_string[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        for (int i = 0; i < 12; ++i) {
            if (view == months_string[i]) {
                return i + 1;
            }
        }
        return 0;
    }

public:
    constexpr datetime() noexcept = default;

    NEFORCE_CONSTEXPR20 ~datetime() = default;

    constexpr datetime(const datetime&) noexcept = default;
    constexpr datetime& operator=(const datetime&) noexcept = default;
    constexpr datetime(datetime&&) noexcept = default;
    constexpr datetime& operator=(datetime&&) noexcept = default;

    /**
     * @brief 从日期和时间构造
     * @param year 年份
     * @param month 月份
     * @param day 日期
     * @param hour 小时
     * @param minute 分钟
     * @param second 秒
     */
    constexpr explicit datetime(const date_type year, const date_type month, const date_type day, const time_type hour,
                                const time_type minute, const time_type second) noexcept :
    date_(year, month, day),
    time_(hour, minute, second) {}

    /**
     * @brief 从日期、时间和时区构造
     * @param year 年份
     * @param month 月份
     * @param day 日期
     * @param hour 小时
     * @param minute 分钟
     * @param second 秒
     * @param offset 时区偏移（秒）
     */
    constexpr explicit datetime(const date_type year, const date_type month, const date_type day, const time_type hour,
                                const time_type minute, const time_type second, const int64_t offset) noexcept :
    date_(year, month, day),
    time_(hour, minute, second),
    offset_seconds_(offset),
    has_timezone_(true) {}

    /**
     * @brief 从日期和时间对象构造
     * @param date 日期对象
     * @param time 时间对象
     */
    constexpr explicit datetime(const _NEFORCE date& date, const _NEFORCE time& time) noexcept :
    date_(date),
    time_(time) {}

    /**
     * @brief 从日期、时间对象和时区构造
     * @param date 日期对象
     * @param time 时间对象
     * @param offset 时区偏移（秒）
     */
    constexpr explicit datetime(const _NEFORCE date& date, const _NEFORCE time& time, const int64_t offset) noexcept :
    date_(date),
    time_(time),
    offset_seconds_(offset),
    has_timezone_(true) {}

    /**
     * @brief 从日期构造（时间部分为00:00:00）
     * @param date 日期对象
     */
    constexpr explicit datetime(const _NEFORCE date& date) noexcept :
    date_(date) {}

    /**
     * @brief 从时间构造（日期部分为1970-01-01）
     * @param time 时间对象
     */
    constexpr explicit datetime(const _NEFORCE time& time) noexcept :
    time_(time) {}

    /**
     * @brief 从日期构造（时间部分为00:00:00）
     * @param date 日期对象
     */
    constexpr explicit datetime(_NEFORCE date&& date) noexcept :
    date_(date) {
        date.clear();
    }

    /**
     * @brief 从时间构造（日期部分为1970-01-01）
     * @param time 时间对象
     */
    constexpr explicit datetime(_NEFORCE time&& time) noexcept :
    time_(time) {
        time.clear();
    }

    /**
     * @brief 从日期和时间对象移动构造
     * @param date 日期对象
     * @param time 时间对象
     */
    constexpr explicit datetime(_NEFORCE date&& date, _NEFORCE time&& time) noexcept :
    date_(date),
    time_(time) {
        date.clear();
        time.clear();
    }

    /**
     * @brief 从日期、时间对象和时区移动构造
     * @param date 日期对象
     * @param time 时间对象
     * @param offset 时区偏移（秒）
     */
    constexpr explicit datetime(_NEFORCE date&& date, _NEFORCE time&& time, const int64_t offset) noexcept :
    date_(date),
    time_(time),
    offset_seconds_(offset),
    has_timezone_(true) {
        date.clear();
        time.clear();
    }

    /**
     * @brief 检查时间是否有效
     * @return 是否有效
     */
    constexpr bool is_valid() const noexcept { return date_.is_valid() && time_.is_valid(); }

    /**
     * @brief 从日期赋值
     * @param date 日期对象
     * @return 自身引用
     */
    constexpr datetime& operator=(const _NEFORCE date& date) noexcept {
        date_ = date;
        return *this;
    }

    /**
     * @brief 从日期移动赋值
     * @param date 日期对象
     * @return 自身引用
     */
    constexpr datetime& operator=(_NEFORCE date&& date) noexcept {
        date_ = date;
        date.clear();
        return *this;
    }

    /**
     * @brief 从时间赋值
     * @param time 时间对象
     * @return 自身引用
     */
    constexpr datetime& operator=(const _NEFORCE time& time) noexcept {
        time_ = time;
        return *this;
    }

    /**
     * @brief 从时间移动赋值
     * @param time 时间对象
     * @return 自身引用
     */
    constexpr datetime& operator=(_NEFORCE time&& time) noexcept {
        time_ = time;
        time.clear();
        return *this;
    }

    /**
     * @brief 获取日期部分
     * @return 日期对象
     */
    NEFORCE_NODISCARD constexpr const _NEFORCE date& date() const noexcept { return date_; }

    /**
     * @brief 获取时间部分
     * @return 时间对象
     */
    NEFORCE_NODISCARD constexpr const _NEFORCE time& time() const noexcept { return time_; }

    /**
     * @brief 获取小时
     * @return 小时
     */
    NEFORCE_NODISCARD constexpr time_type hours() const noexcept { return time_.hours(); }

    /**
     * @brief 获取分钟
     * @return 分钟
     */
    NEFORCE_NODISCARD constexpr time_type minutes() const noexcept { return time_.minutes(); }

    /**
     * @brief 获取秒
     * @return 秒
     */
    NEFORCE_NODISCARD constexpr time_type seconds() const noexcept { return time_.seconds(); }

    /**
     * @brief 获取年份
     * @return 年份
     */
    NEFORCE_NODISCARD constexpr date_type year() const noexcept { return date_.year(); }

    /**
     * @brief 获取月份
     * @return 月份
     */
    NEFORCE_NODISCARD constexpr date_type month() const noexcept { return date_.month(); }

    /**
     * @brief 获取日期
     * @return 日期
     */
    NEFORCE_NODISCARD constexpr date_type day() const noexcept { return date_.day(); }

    /**
     * @brief 检查是否有时区信息
     * @return 是否有时区
     */
    NEFORCE_NODISCARD constexpr bool has_timezone() const noexcept { return has_timezone_; }

    /**
     * @brief 获取时区偏移
     * @return 时区偏移（秒）
     */
    NEFORCE_NODISCARD constexpr int64_t offset_seconds() const noexcept { return offset_seconds_; }

    /**
     * @brief 获取纪元起始时间（1970-01-01 00:00:00 UTC）
     * @return 纪元起始时间
     */
    NEFORCE_NODISCARD static constexpr datetime epoch() noexcept { return datetime{}; }

    /**
     * @brief 获取当前本地时间
     * @return 当前本地时间
     */
    NEFORCE_NODISCARD static datetime now() noexcept;

    /**
     * @brief 重置为纪元起始时间
     */
    constexpr void clear() noexcept {
        date_.clear();
        time_.clear();
        offset_seconds_ = 0;
        has_timezone_ = false;
    }

    /**
     * @brief 相等比较
     */
    constexpr bool operator==(const datetime& other) const noexcept {
        return date_ == other.date_ && time_ == other.time_ && has_timezone_ == other.has_timezone_ &&
               offset_seconds_ == other.offset_seconds_;
    }

    /**
     * @brief 小于比较
     */
    constexpr bool operator<(const datetime& other) const noexcept {
        if (date_ < other.date_) {
            return true;
        } else if (date_ == other.date_) {
            if (time_ < other.time_) {
                return true;
            } else if (time_ == other.time_) {
                if (has_timezone_ && other.has_timezone_ && offset_seconds_ < other.offset_seconds_) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief 加秒数
     * @param seconds 秒数
     * @return 自身引用
     */
    constexpr datetime& operator+=(const int64_t seconds) {
        if (seconds < 0) {
            return *this -= -seconds;
        }

        const int64_t current_total_sec = static_cast<int64_t>(time_.hours()) * 3600 +
                                          static_cast<int64_t>(time_.minutes()) * 60 +
                                          static_cast<int64_t>(time_.seconds());

        int64_t new_total_sec = current_total_sec + seconds;
        int64_t days_to_add = new_total_sec / 86400;
        new_total_sec %= 86400;

        if (new_total_sec < 0) {
            new_total_sec += 86400;
            days_to_add--;
        }

        date_ += static_cast<date_type>(days_to_add);
        time_ = _NEFORCE time(static_cast<time_type>(new_total_sec / 3600),
                              static_cast<time_type>((new_total_sec % 3600) / 60),
                              static_cast<time_type>(new_total_sec % 60));
        return *this;
    }

    /**
     * @brief 减秒数
     * @param seconds 秒数
     * @return 自身引用
     */
    constexpr datetime& operator-=(const int64_t seconds) noexcept {
        if (seconds < 0) {
            return *this += -seconds;
        }

        const int64_t current_total_sec = static_cast<int64_t>(time_.hours()) * 3600 +
                                          static_cast<int64_t>(time_.minutes()) * 60 +
                                          static_cast<int64_t>(time_.seconds());

        int64_t new_total_sec = current_total_sec - seconds;
        int64_t days_to_subtract = 0;

        if (new_total_sec < 0) {
            days_to_subtract = (-new_total_sec + 86399) / 86400;
            new_total_sec += days_to_subtract * 86400;
        }

        date_ -= static_cast<date_type>(days_to_subtract);
        time_ = _NEFORCE time(static_cast<time_type>(new_total_sec / 3600),
                              static_cast<time_type>((new_total_sec % 3600) / 60),
                              static_cast<time_type>(new_total_sec % 60));
        return *this;
    }

    /**
     * @brief 加秒数
     * @param seconds 秒数
     * @return 新时间
     */
    constexpr datetime operator+(const int64_t seconds) const noexcept {
        datetime ret(*this);
        ret += seconds;
        return ret;
    }

    /**
     * @brief 减秒数
     * @param seconds 秒数
     * @return 新时间
     */
    constexpr datetime operator-(const int64_t seconds) const noexcept {
        datetime ret(*this);
        ret -= seconds;
        return ret;
    }

    /**
     * @brief 前置递增（加1秒）
     */
    constexpr datetime& operator++() { return *this += 1; }

    /**
     * @brief 后置递增（加1秒）
     */
    constexpr datetime operator++(int) {
        const datetime ret(*this);
        *this += 1;
        return ret;
    }

    /**
     * @brief 前置递减（减1秒）
     */
    constexpr datetime& operator--() { return *this -= 1; }

    /**
     * @brief 后置递减（减1秒）
     */
    constexpr datetime operator--(int) {
        const datetime ret(*this);
        *this -= 1;
        return ret;
    }

    /**
     * @brief 时间差（秒数）
     * @param other 另一个时间
     * @return 相差的秒数
     */
    constexpr time_type operator-(const datetime& other) const noexcept {
        const datetime lhs_utc = this->to_UTC();
        const datetime rhs_utc = other.to_UTC();

        const time_type day_diff = lhs_utc.date_ - rhs_utc.date_;
        time_type sec_diff = day_diff * 86400;
        sec_diff += (lhs_utc.time_ - rhs_utc.time_);
        return sec_diff;
    }

    /**
     * @brief 转换为时区偏移字符串
     * @return 格式为 ±HH:MM 或 "Z"
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_offset_string() const {
        if (!has_timezone_) {
            return {};
        }
        if (offset_seconds_ == 0) {
            return "Z";
        }
        int64_t total_sec = offset_seconds_;
        const char sign = total_sec >= 0 ? '+' : '-';
        total_sec = total_sec >= 0 ? total_sec : -total_sec;
        const int64_t hours = total_sec / 3600;
        const int64_t minutes = (total_sec % 3600) / 60;
        return _NEFORCE format("{}{:02d}:{:02d}", sign, hours, minutes);
    }

    /**
     * @brief 从UTC时间转换为本地时间
     * @param utc UTC时间
     * @param offset 本地时区偏移（秒）
     * @return 本地时间
     */
    static constexpr datetime from_UTC(const datetime& utc, const int32_t offset = 0) noexcept {
        datetime utc_time = utc;
        if (utc.has_timezone_ && utc.offset_seconds_ != 0) {
            utc_time = utc.to_UTC();
        }
        datetime local = utc_time + offset;
        local.offset_seconds_ = offset;
        local.has_timezone_ = true;
        return local;
    }

    /**
     * @brief 转换为UTC时间
     * @return UTC时间
     */
    NEFORCE_NODISCARD constexpr datetime to_UTC() const noexcept {
        if (!has_timezone_) {
            return *this;
        }
        datetime utc = *this;
        utc -= offset_seconds_;
        utc.offset_seconds_ = 0;
        utc.has_timezone_ = true;
        return utc;
    }

    /**
     * @brief 转换为 RFC 3339
     * @return 格式为 YYYY-MM-DDTHH:MM:SS±HH:MM 或 YYYY-MM-DDTHH:MM:SSZ
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_RFC3339() const {
        if (has_timezone_) {
            return date_.to_string() + "T" + time_.to_string() + to_offset_string();
        } else {
            return date_.to_string() + "T" + time_.to_string();
        }
    }

    /**
     * @brief 解析 RFC 3339
     * @param view RFC 3339 格式字符串
     * @return 解析得到的日期时间
     * @throws value_exception 格式错误时抛出
     */
    NEFORCE_NODISCARD static constexpr datetime parse_RFC3339(const string_view view) {
        if (view.size() < 20 || view[10] != 'T') {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid RFC3339 datetime format."));
        }

        const _NEFORCE date d = date::parse(view.substr(0, 10));
        const _NEFORCE time t = time::parse(view.substr(11, 8));

        if (view.size() > 19 && view[19] == 'Z') {
            return datetime(d, t, 0);
        } else if (view.size() > 19 && (view[19] == '+' || view[19] == '-')) {
            const char sign = view[19];
            int hours = 0, minutes = 0;
            size_t pos = 20;
            if (view.size() >= pos + 2) {
                hours = integer32::parse(view.substr(pos, 2)).value();
                pos += 2;
                if (view.size() >= pos + 3 && view[pos] == ':') {
                    pos++;
                    if (view.size() >= pos + 2) {
                        minutes = integer32::parse(view.substr(pos, 2)).value();
                    }
                }
            }

            int32_t total_offset = hours * 3600 + minutes * 60;
            if (sign == '-') {
                total_offset = -total_offset;
            }
            return datetime(d, t, total_offset);
        }
        return datetime(d, t);
    }

    /**
     * @brief 尝试解析 RFC 3339
     * @param view RFC 3339 格式字符串
     * @return 是否解析成功
     */
    NEFORCE_CONSTEXPR20 bool try_parse_RFC3339(const string_view view) noexcept {
        try {
            datetime tmp = datetime::parse_RFC3339(view);
            this->swap(tmp);
        } catch (...) {
            return false;
        }
        return true;
    }

    /**
     * @brief 转换为 RFC 1123
     * @return 格式为 "Wed, 21 Dec 2022 10:00:00 GMT"
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_RFC1123() const noexcept {
        constexpr string_view months_string[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

        constexpr string_view weekdays_string[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

        const _NEFORCE date utc_date = date();
        const _NEFORCE time utc_time = time();

        int wday = utc_date.days_of_week();
        if (wday < 0 || wday >= 7) {
            wday = 0;
        }

        int mon_idx = utc_date.month() - 1;
        if (mon_idx < 0 || mon_idx >= 12) {
            mon_idx = 0;
        }

        return _NEFORCE format("{}, {:02d} {} {} {} GMT", weekdays_string[wday], utc_date.day(), months_string[mon_idx],
                               utc_date.year(), utc_time.to_string());
    }

    /**
     * @brief 解析 RFC 1123
     * @param view RFC 1123 格式字符串
     * @return 解析得到的日期时间
     * @throws value_exception 格式错误时抛出
     */
    NEFORCE_NODISCARD static constexpr datetime parse_RFC1123(string_view view) {
        if (view.size() < 29) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid date length."));
        }
        if (view.substr(3, 2) != ", ") {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid date format"));
        }

        view.remove_prefix(5);
        const int day = integer32::parse(view.substr(0, 2)).value();
        view.remove_prefix(3);
        const int mon = months_to_int(view.substr(0, 3));
        if (mon == 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid month in date"));
        }
        view.remove_prefix(4);
        const int year = integer32::parse(view.substr(0, 4)).value();
        view.remove_prefix(5);
        const int hour = integer32::parse(view.substr(0, 2)).value();
        view.remove_prefix(3);
        const int minute = integer32::parse(view.substr(0, 2)).value();
        view.remove_prefix(3);
        const int second = integer32::parse(view.substr(0, 2)).value();
        view.remove_prefix(3);

        if (view != "GMT") {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid timezone in date"));
        }
        return datetime(year, mon, day, hour, minute, second);
    }

    /**
     * @brief 尝试解析 RFC 1123
     * @param view RFC 1123 格式字符串
     * @return 是否解析成功
     */
    NEFORCE_CONSTEXPR20 bool try_parse_RFC1123(const string_view view) noexcept {
        try {
            datetime tmp = datetime::parse_RFC1123(view);
            this->swap(tmp);
        } catch (...) {
            return false;
        }
        return true;
    }

    /**
     * @brief 转换为 ISO 8601
     * @return 格式为 YYYY-MM-DDTHH:MM:SS
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_ISO8601() const {
        return date_.to_string() + "T" + time_.to_string();
    }

    /**
     * @brief 解析 ISO 8601
     * @param view ISO 8601 格式字符串
     * @return 解析得到的日期时间
     * @throws value_exception 格式错误时抛出
     */
    NEFORCE_NODISCARD static constexpr datetime parse_ISO8601(const string_view view) {
        if (view.size() < 19 || view[10] != 'T') {
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid ISO 8601 datetime format."));
        }
        const _NEFORCE date d = _NEFORCE date::parse(view.substr(0, 10));
        size_t time_len = 8;
        if (view.size() >= 19) {
            time_len = 8;
        }
        const _NEFORCE time t = _NEFORCE time::parse(view.substr(11, time_len));
        return datetime(d, t);
    }

    /**
     * @brief 尝试解析 ISO 8601
     * @param view ISO 8601 格式字符串
     * @return 是否解析成功
     */
    NEFORCE_CONSTEXPR20 bool try_parse_ISO8601(const string_view view) noexcept {
        try {
            datetime tmp = datetime::parse_ISO8601(view);
            this->swap(tmp);
        } catch (...) {
            return false;
        }
        return true;
    }

    /**
     * @brief 转换为简单格式
     * @return 格式为 YYYY-MM-DD HH:MM:SS
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const {
        return date_.to_string() + " " + time_.to_string();
    }

    /**
     * @brief 解析简单格式
     * @param view 格式为 YYYY-MM-DD HH:MM:SS 的字符串
     * @return 解析得到的日期时间
     * @throws value_exception 格式错误时抛出
     */
    NEFORCE_NODISCARD static constexpr datetime parse(const string_view view) {
        if (view.size() != 19 || view[10] != ' ') {
            NEFORCE_THROW_EXCEPTION(value_exception("Wrong string formation."));
        }
        const _NEFORCE date d = date::parse(view.substr(0, 10));
        const _NEFORCE time t = time::parse(view.substr(11, 8));
        return datetime(d, t);
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        return date_.to_hash() ^ time_.to_hash() ^ hash<bool>()(has_timezone_) ^ hash<int64_t>()(offset_seconds_);
    }

    /**
     * @brief 交换两个日期时间
     * @param other 另一个日期时间
     */
    constexpr void swap(datetime& other) noexcept {
        _NEFORCE swap(date_, other.date_);
        _NEFORCE swap(time_, other.time_);
        _NEFORCE swap(offset_seconds_, other.offset_seconds_);
        _NEFORCE swap(has_timezone_, other.has_timezone_);
    }
};


/**
 * @class timestamp
 * @brief 时间戳类
 *
 * 表示从1970-01-01 00:00:00 UTC开始的秒数。
 * 包装了int64_t，提供与datetime的相互转换。
 */
class NEFORCE_API timestamp : public iobject<timestamp>, public ipackage<timestamp, int64_t> {
public:
    using value_type = int64_t; ///< 值类型

    constexpr timestamp() noexcept = default;

    NEFORCE_CONSTEXPR20 ~timestamp() = default;

    constexpr timestamp(const timestamp& other) noexcept :
    ipackage(other.value_) {}

    constexpr timestamp& operator=(const timestamp& other) noexcept {
        value_ = other.value_;
        return *this;
    }

    constexpr timestamp(timestamp&& other) noexcept :
    ipackage(other.value_) {
        other.clear();
    }

    constexpr timestamp& operator=(timestamp&& other) noexcept {
        value_ = other.value_;
        other.clear();
        return *this;
    }

    /**
     * @brief 从秒数构造
     * @param value 秒数
     */
    constexpr explicit timestamp(const value_type value) noexcept :
    ipackage(value) {}

    /**
     * @brief 从日期时间构造
     * @param dt 日期时间
     */
    constexpr explicit timestamp(const datetime& dt) noexcept {
        const datetime utc = dt.to_UTC();
        value_ = utc - datetime::epoch().to_UTC();
    }

    /**
     * @brief 获取当前时间戳
     * @return 当前时间戳
     */
    NEFORCE_NODISCARD static timestamp now() noexcept { return timestamp(datetime::now()); }

    /**
     * @brief 转换为日期时间
     * @return 对应的日期时间
     */
    NEFORCE_NODISCARD constexpr datetime to_datetime() const noexcept { return datetime::epoch() + value_; }

    /**
     * @brief 转换为字符串
     * @return 数字字符串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const { return integer64(value_).to_string(); }

    /**
     * @brief 从字符串解析
     * @param view 数字字符串
     * @return 解析得到的时间戳
     */
    NEFORCE_NODISCARD static constexpr timestamp parse(const string_view view) {
        return timestamp{integer64::parse(view).value()};
    }

    /**
     * @brief 重置为0
     */
    constexpr void clear() noexcept { value_ = 0; }
};

/** @} */ // DateTime

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TIME_DATETIME_HPP__
