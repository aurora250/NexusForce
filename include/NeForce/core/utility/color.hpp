#ifndef NEFORCE_CORE_UTILITY_COLOR_HPP__
#define NEFORCE_CORE_UTILITY_COLOR_HPP__

/**
 * @file color.hpp
 * @brief 颜色类
 *
 * 此文件提供了RGB颜色模型的实现，支持颜色分量操作、混合、
 * 线性插值、ANSI终端颜色转换等功能。
 */

#include "NeForce/core/utility/hexadecimal.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Color 颜色
 * @brief RGB颜色模型的实现
 * @{
 */

/**
 * @class color
 * @brief RGB颜色类
 *
 * 表示一个RGB颜色，包含红、绿、蓝和透明度四个分量。
 * 每个分量的取值范围为0-255，透明度255表示不透明。
 * 支持颜色混合、线性插值、ANSI终端颜色转换等操作。
 */
class color : public iobject<color>, public icommon<color> {
private:
    int r = 0;    ///< 红分量
    int g = 0;    ///< 绿分量
    int b = 0;    ///< 蓝分量
    int a = 255;  ///< 透明度分量

    /**
     * @brief 将整数值限制在0-255范围内
     * @param value 要限制的值
     * @return 限制后的值
     */
    static constexpr int clamp(const int value) noexcept {
        if (value < 0) return 0;
        if (value > 255) return 255;
        return value;
    }

    /**
     * @brief 将浮点数值限制在0.0-1.0范围内
     * @param value 要限制的值
     * @return 限制后的值
     */
    static constexpr double clamp_double(const double value) noexcept {
        if (value < 0.0) return 0.0;
        if (value > 1.0) return 1.0;
        return value;
    }

    /**
     * @brief 线性插值单个颜色分量
     * @param from 起始值
     * @param to 目标值
     * @param t 插值因子（0.0-1.0）
     * @return 插值结果
     */
    static constexpr int lerp_component(const int from, const int to, const double t) noexcept {
        const double result = from + (to - from) * clamp_double(t);
        return static_cast<int>(_NEFORCE round(result));
    }

public:
    /**
     * @brief 默认构造函数，创建黑色（0,0,0,255）
     */
    constexpr color() noexcept = default;

    constexpr color(const color& other) noexcept = default;
    constexpr color& operator =(const color& other) noexcept = default;

    /**
     * @brief 从灰度值构造
     * @param gray 灰度值（0-255）
     */
    explicit constexpr color(const int gray) noexcept
    : r(clamp(gray)), g(clamp(gray)), b(clamp(gray)) {}

    /**
     * @brief 从灰度值和透明度构造
     * @param gray 灰度值（0-255）
     * @param alpha 透明度（0-255）
     */
    constexpr color(const int gray, const int alpha) noexcept
    : r(clamp(gray)), g(clamp(gray)), b(clamp(gray)), a(clamp(alpha)) {}

    /**
     * @brief 从RGB分量构造
     * @param red 红色分量（0-255）
     * @param green 绿色分量（0-255）
     * @param blue 蓝色分量（0-255）
     */
    constexpr color(const int red, const int green, const int blue) noexcept
    : r(clamp(red)), g(clamp(green)), b(clamp(blue)) {}

    /**
     * @brief 从RGBA分量构造
     * @param red 红色分量（0-255）
     * @param green 绿色分量（0-255）
     * @param blue 蓝色分量（0-255）
     * @param alpha 透明度（0-255）
     */
    constexpr color(const int red, const int green, const int blue, const int alpha) noexcept
    : r(clamp(red)), g(clamp(green)), b(clamp(blue)), a(clamp(alpha)) {}

    /**
     * @brief 从十六进制字符串构造
     * @param str 十六进制字符串（格式：RRGGBB 或 RRGGBBAA）
     */
    NEFORCE_CONSTEXPR20 explicit color(const string_view str) {
        try_parse(str);
    }

    /**
     * @brief 从字符串对象构造
     * @param str 十六进制字符串
     */
    NEFORCE_CONSTEXPR20 explicit color(const string& str) {
        try_parse(str.view());
    }

    /**
     * @brief 从C风格字符串构造
     * @param str 十六进制字符串
     */
    NEFORCE_CONSTEXPR20 explicit color(const char* str) {
        try_parse(string_view{str});
    }

    /**
     * @brief 移动构造函数
     */
    constexpr color(color&& other) noexcept
    : r(other.r), g(other.g), b(other.b), a(other.a) {
        other.r = 0;
        other.g = 0;
        other.b = 0;
        other.a = 255;
    }

    /**
     * @brief 移动赋值运算符
     */
    constexpr color& operator =(color&& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        this->swap(other);
        return *this;
    }

    NEFORCE_CONSTEXPR20 ~color() = default;

    /**
     * @brief 获取红色分量
     * @return 红色分量值
     */
    constexpr int R() const noexcept {
        return r;
    }

    /**
     * @brief 获取绿色分量
     * @return 绿色分量值
     */
    constexpr int G() const noexcept {
        return g;
    }

    /**
     * @brief 获取蓝色分量
     * @return 蓝色分量值
     */
    constexpr int B() const noexcept {
        return b;
    }

    /**
     * @brief 获取透明度
     * @return 透明度值
     */
    constexpr int A() const noexcept {
        return a;
    }

    /**
     * @brief 设置红色分量
     * @param red 红色分量值
     */
    constexpr void setR(const int red) noexcept {
        r = clamp(red);
    }

    /**
     * @brief 设置绿色分量
     * @param green 绿色分量值
     */
    constexpr void setG(const int green) noexcept {
        g = clamp(green);
    }

    /**
     * @brief 设置蓝色分量
     * @param blue 蓝色分量值
     */
    constexpr void setB(const int blue) noexcept {
        b = clamp(blue);
    }

    /**
     * @brief 设置透明度
     * @param alpha 透明度值
     */
    constexpr void setA(const int alpha) noexcept {
        a = clamp(alpha);
    }

    /**
     * @brief 设置RGB颜色
     * @param red 红色分量
     * @param green 绿色分量
     * @param blue 蓝色分量
     */
    constexpr void set_color(const int red, const int green, const int blue) noexcept {
        r = clamp(red);
        g = clamp(green);
        b = clamp(blue);
    }

    /**
     * @brief 设置RGBA颜色
     * @param red 红色分量
     * @param green 绿色分量
     * @param blue 蓝色分量
     * @param alpha 透明度
     */
    constexpr void set_color(const int red, const int green, const int blue, const int alpha) noexcept {
        r = clamp(red);
        g = clamp(green);
        b = clamp(blue);
        a = clamp(alpha);
    }

    /**
     * @brief 设置灰度值
     * @param gray 灰度值
     */
    constexpr void set_gray(const int gray) noexcept {
        r = g = b = clamp(gray);
    }

    /**
     * @brief 设置灰度值和透明度
     * @param gray 灰度值
     * @param alpha 透明度
     */
    constexpr void set_gray(const int gray, const int alpha) noexcept {
        set_gray(gray);
        a = clamp(alpha);
    }

    /**
     * @brief 检查是否完全透明
     * @return 是否完全透明
     */
    constexpr bool is_transparent() const noexcept {
        return a == 0;
    }

    /**
     * @brief 检查是否完全不透明
     * @return 是否完全不透明
     */
    constexpr bool is_opaque() const noexcept {
        return a == 255;
    }

    /**
     * @brief 获取不透明度（0.0-1.0）
     * @return 不透明度
     */
    constexpr double opacity() const noexcept {
        return a / 255.0;
    }

    /**
     * @brief 设置不透明度
     * @param opacity 不透明度（0.0-1.0）
     */
    constexpr void set_opacity(double opacity) noexcept {
        if (opacity < 0.0) opacity = 0.0;
        if (opacity > 1.0) opacity = 1.0;
        a = static_cast<int>(_NEFORCE round(opacity * 255));
    }

    /**
     * @brief 线性插值两个颜色
     * @param from 起始颜色
     * @param to 目标颜色
     * @param t 插值因子（0.0-1.0）
     * @return 插值结果
     */
    static constexpr color lerp(const color& from, const color& to, double t) noexcept {
        t = clamp_double(t);
        return color(
            lerp_component(from.r, to.r, t),
            lerp_component(from.g, to.g, t),
            lerp_component(from.b, to.b, t),
            lerp_component(from.a, to.a, t)
        );
    }

    /**
     * @brief 颜色加法
     * @param other 另一个颜色
     * @return 相加后的颜色
     */
    constexpr color operator +(const color& other) const noexcept {
        return color(r + other.r, g + other.g, b + other.b, a + other.a);
    }

    /**
     * @brief 颜色减法
     * @param other 另一个颜色
     * @return 相减后的颜色
     */
    constexpr color operator -(const color& other) const noexcept {
        return color(r - other.r, g - other.g, b - other.b, a - other.a);
    }

    /**
     * @brief 颜色乘以标量
     * @param scalar 浮点标量
     * @return 缩放后的颜色
     */
    constexpr color operator *(const double scalar) const noexcept {
        return color(
            static_cast<int>(_NEFORCE round(r * scalar)),
            static_cast<int>(_NEFORCE round(g * scalar)),
            static_cast<int>(_NEFORCE round(b * scalar)),
            static_cast<int>(_NEFORCE round(a * scalar))
        );
    }

    /**
     * @brief 颜色乘以整数标量
     * @param scalar 整数标量
     * @return 缩放后的颜色
     */
    constexpr color operator *(const int scalar) const noexcept {
        return {r * scalar, g * scalar, b * scalar, a * scalar};
    }

    /**
     * @brief 颜色加等赋值
     * @param other 另一个颜色
     * @return 自身引用
     */
    constexpr color& operator +=(const color& other) noexcept {
        r = clamp(r + other.r);
        g = clamp(g + other.g);
        b = clamp(b + other.b);
        a = clamp(a + other.a);
        return *this;
    }

    /**
     * @brief 颜色减等赋值
     * @param other 另一个颜色
     * @return 自身引用
     */
    constexpr color& operator -=(const color& other) noexcept {
        r = clamp(r - other.r);
        g = clamp(g - other.g);
        b = clamp(b - other.b);
        a = clamp(a - other.a);
        return *this;
    }

    /**
     * @brief 颜色乘等赋值
     * @param scalar 浮点标量
     * @return 自身引用
     */
    constexpr color& operator *=(const double scalar) noexcept {
        r = clamp(static_cast<int>(_NEFORCE round(r * scalar)));
        g = clamp(static_cast<int>(_NEFORCE round(g * scalar)));
        b = clamp(static_cast<int>(_NEFORCE round(b * scalar)));
        a = clamp(static_cast<int>(_NEFORCE round(a * scalar)));
        return *this;
    }

    /**
     * @brief 相等比较
     * @param other 另一个颜色
     * @return 是否相等
     */
    constexpr bool operator ==(const color& other) const noexcept {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    /**
     * @brief 小于比较
     * @param other 另一个颜色
     * @return 是否小于
     */
    constexpr bool operator <(const color& other) const noexcept {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        if (b != other.b) return b < other.b;
        return a < other.a;
    }

    /**
     * @brief 将当前颜色与背景色混合
     * @param background 背景色
     * @return 混合后的颜色
     *
     * 使用Alpha混合算法：前景色与背景色根据透明度混合。
     */
    NEFORCE_NODISCARD constexpr color blend(const color& background) const noexcept {
        if (a == 255) return *this;
        if (a == 0) return background;

        const double alpha = a / 255.0;
        const double inv_alpha = 1.0 - alpha;

        const int newR = static_cast<int>(_NEFORCE round(r * alpha + background.r * inv_alpha));
        const int newG = static_cast<int>(_NEFORCE round(g * alpha + background.g * inv_alpha));
        const int newB = static_cast<int>(_NEFORCE round(b * alpha + background.b * inv_alpha));
        return color(newR, newG, newB, 255);
    }

    /**
     * @brief 预乘Alpha
     * @return 预乘Alpha后的颜色
     *
     * 将RGB分量乘以透明度，用于某些图形处理算法。
     */
    NEFORCE_NODISCARD constexpr color premultiply_alpha() const noexcept {
        const double alpha = a / 255.0;
        return color(
            static_cast<int>(_NEFORCE round(r * alpha)),
            static_cast<int>(_NEFORCE round(g * alpha)),
            static_cast<int>(_NEFORCE round(b * alpha)),
            a
        );
    }

    /**
     * @brief 计算灰度值
     * @return 灰度值（基于人眼感知的加权平均）
     */
    NEFORCE_NODISCARD constexpr double gray_value() const noexcept {
        return 0.299 * r + 0.587 * g + 0.114 * b;
    }

    /**
     * @brief 转换为十六进制字符串
     * @return 格式为RRGGBBAA的十六进制字符串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const {
        return _NEFORCE format("{02X}{02X}{02X}{02X}", r, g, b, a);
    }

    /**
     * @brief 从十六进制字符串解析颜色
     * @param hex 十六进制字符串（支持#RRGGBB或#RRGGBBAA格式）
     * @return 解析得到的颜色
     * @throws value_exception 格式无效时抛出
     */
    static NEFORCE_CONSTEXPR20 color parse(const string_view hex) {
        string_view clean_hex = hex;
        if (clean_hex[0] == '#') clean_hex = clean_hex.substr(1);

        if (clean_hex.length() == 6) {
            const int r = hexadecimal::parse(clean_hex.substr(0, 2)).value();
            const int g = hexadecimal::parse(clean_hex.substr(2, 2)).value();
            const int b = hexadecimal::parse(clean_hex.substr(4, 2)).value();
            return {r, g, b};
        } else if (clean_hex.length() == 8) {
            const int r = hexadecimal::parse(clean_hex.substr(0, 2)).value();
            const int g = hexadecimal::parse(clean_hex.substr(2, 2)).value();
            const int b = hexadecimal::parse(clean_hex.substr(4, 2)).value();
            const int a = hexadecimal::parse(clean_hex.substr(6, 2)).value();
            return {r, g, b, a};
        } else {
            throw_exception(value_exception("Invalid hex string"));
        }
        NEFORCE_UNREACHABLE;
    }

    /**
     * @brief 转换为256色ANSI颜色索引
     * @return ANSI 256色索引（0-255）
     */
    NEFORCE_NODISCARD constexpr int to_ansi_256() const noexcept {
        if (r < 8 && g < 8 && b < 8) return 16;
        if (r > 248 && g > 248 && b > 248) return 231;

        if (r == g && g == b) {
            const int gray_index = static_cast<int>((r - 8) / 247.0 * 24.0 + 0.5);
            return 232 + gray_index;
        }

        const int r_idx = static_cast<int>((r / 255.0) * 5.0 + 0.5);
        const int g_idx = static_cast<int>((g / 255.0) * 5.0 + 0.5);
        const int b_idx = static_cast<int>((b / 255.0) * 5.0 + 0.5);

        return 16 + 36 * r_idx + 6 * g_idx + b_idx;
    }

    /**
     * @brief 转换为基本ANSI颜色代码
     * @param is_background 是否为背景色
     * @return 基本ANSI颜色代码
     */
    NEFORCE_NODISCARD constexpr int to_ansi_basic(const bool is_background = false) const noexcept {
        const int base = is_background ? 40 : 30;

        if (r > g && r > b) {
            if (g > 128 && b > 128) return base + 7;
            return base + 1;
        } else if (g > r && g > b) {
            if (r > 128 && b > 128) return base + 7;
            return base + 2;
        } else if (b > r && b > g) {
            if (r > 128 && g > 128) return base + 7;
            return base + 4;
        } else if (r == g && g == b) {
            if (r < 64) return base + 0;
            if (r < 192) return base + 7;
            return base + 7;
        } else if (r == g && r > b) {
            return base + 3;
        } else if (r == b && r > g) {
            return base + 5;
        } else if (g == b && g > r) {
            return base + 6;
        }

        return base + 7;
    }

    /**
     * @brief 转换为ANSI前景色代码
     * @param use_256_color 是否使用256色模式
     * @return ANSI前景色代码（包装为integer32）
     */
    NEFORCE_NODISCARD constexpr integer32 to_ansi_foreground(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return {38 * 100 + 5 * 10 + to_ansi_256()};
        }
        return {to_ansi_basic(false)};
    }

    /**
     * @brief 转换为ANSI背景色代码
     * @param use_256_color 是否使用256色模式
     * @return ANSI背景色代码（包装为integer32）
     */
    NEFORCE_NODISCARD constexpr integer32 to_ansi_background(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return {48 * 100 + 5 * 10 + to_ansi_256()};
        }
        return {to_ansi_basic(true)};
    }

    /**
     * @brief 转换为整数表示
     * @param use_256_color 是否使用256色模式
     * @return 颜色索引（包装为integer32）
     */
    NEFORCE_NODISCARD constexpr integer32 to_integer32(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return {to_ansi_256()};
        }
        return {to_ansi_basic(false)};
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<int> hasher;
        return hasher(r) ^ hasher(g) ^ hasher(b) ^ hasher(a);
    }

    /**
     * @brief 交换两个颜色对象
     * @param other 要交换的对象
     */
    constexpr void swap(color& other) noexcept {
        color tmp = _NEFORCE move(other);
        other = _NEFORCE move(*this);
        *this = _NEFORCE move(tmp);
    }

    /** @brief 黑色 */
    static constexpr color black() noexcept {
        return color(0, 0, 0, 255);
    }

    /** @brief 白色 */
    static constexpr color white() noexcept {
        return color(255, 255, 255, 255);
    }

    /** @brief 红色 */
    static constexpr color red() noexcept {
        return color(255, 0, 0, 255);
    }

    /** @brief 绿色 */
    static constexpr color green() noexcept {
        return color(0, 255, 0, 255);
    }

    /** @brief 蓝色 */
    static constexpr color blue() noexcept {
        return color(0, 0, 255, 255);
    }

    /** @brief 黄色 */
    static constexpr color yellow() noexcept {
        return color(255, 255, 0, 255);
    }

    /** @brief 品红 */
    static constexpr color magenta() noexcept {
        return color(255, 0, 255, 255);
    }

    /** @brief 青色 */
    static constexpr color cyan() noexcept {
        return color(0, 255, 255, 255);
    }

    /** @brief 完全透明 */
    static constexpr color transparent() noexcept {
        return color(0, 0, 0, 0);
    }
};

/**
 * @brief 标量乘以颜色
 * @param scalar 标量
 * @param color 颜色
 * @return 缩放后的颜色
 */
constexpr color operator *(const double scalar, const color& color) noexcept {
    return color * scalar;
}

/**
 * @brief 整数标量乘以颜色
 * @param scalar 整数标量
 * @param color 颜色
 * @return 缩放后的颜色
 */
constexpr color operator *(const int scalar, const color& color) noexcept {
    return color * scalar;
}

/** @} */ // Color

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_COLOR_HPP__
