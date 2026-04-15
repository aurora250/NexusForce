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
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下颜色表示与合成相关标准规范：
 *
 * **CSS 颜色标准：**
 * - **W3C CSS Color Module Level 4**：CSS 颜色规范（RGB/RGBA 定义）
 *   https://www.w3.org/TR/css-color-4/
 * - **W3C CSS Color Module Level 5**：CSS 颜色规范（颜色混合与合成）
 *   https://www.w3.org/TR/css-color-5/
 *
 * **合成与混合标准：**
 * - **W3C Compositing and Blending Level 1**：合成与混合规范（Alpha 合成）
 *   https://www.w3.org/TR/compositing-1/
 *
 * **色彩空间标准：**
 * - **IEC 61966-2-1:1999**：多媒体系统与设备 — 色彩测量与管理 — 第2-1部分：sRGB 色彩空间
 *   https://webstore.iec.ch/publication/6169
 * - **ITU-R BT.709-6**：高清电视标准参数值（色彩空间定义）
 *   https://www.itu.int/rec/R-REC-BT.709/
 *
 * **ANSI 终端颜色标准：**
 * - **ECMA-48**：控制功能编码字符集（ANSI 转义序列）
 *   https://ecma-international.org/publications-and-standards/standards/ecma-48/
 * - **ISO/IEC 6429:1992**：信息技术 — 编码字符集的控制功能
 *   https://www.iso.org/standard/12782.html
 *
 * **XTerm 256 色规范：**
 * - **XTerm 256-Color Specification**：XTerm 256 色调色板定义
 *   https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Functions-using-CSI-_-ordered-by-the-final-character_s_
 *
 * **图形文件格式标准：**
 * - **ISO/IEC 15948:2004**：信息技术 — 计算机图形与图像处理 — PNG 规范
 *   https://www.iso.org/standard/29581.html
 * - **ISO 32000-2:2020**：文档管理 — 便携式文档格式 — 第2部分：PDF 2.0（透明度组）
 *   https://www.iso.org/standard/75839.html
 *
 * @section color_components 颜色分量定义
 * 根据 CSS Color Module Level 4 §4.2，RGB 颜色由以下分量组成：
 *
 * | 分量   | 范围     | 数据类型 | 说明                              |
 * |--------|----------|----------|--------------------------------|
 * | R (红) | 0-255    | int      | 红色通道强度                     |
 * | G (绿) | 0-255    | int      | 绿色通道强度                     |
 * | B (蓝) | 0-255    | int      | 蓝色通道强度                     |
 * | A (透) | 0-255    | int      | Alpha 通道（0=全透明，255=不透明）|
 *
 * @section hex_format 十六进制表示
 * 根据 CSS Color Module Level 4 §5.2，支持以下十六进制格式：
 *
 * | 格式        | 示例       | 说明                     |
 * |-------------|------------|--------------------------|
 * | #RRGGBB     | #FF0000    | 不透明红色（Alpha=255）   |
 * | #RRGGBBAA   | #FF000080  | 半透明红色（Alpha=128）   |
 *
 * @section alpha_compositing Alpha 合成规则
 * 根据 W3C Compositing and Blending Level 1 §4，Alpha 合成遵循以下规则：
 *
 * | 操作         | 公式                                                            |
 * |--------------|-----------------------------------------------------------------|
 * | 合成 Alpha   | α_result = α_src + α_dst × (1 - α_src)                          |
 * | 合成 RGB     | C_result = (C_src × α_src + C_dst × α_dst × (1 - α_src)) / α_result |
 *
 * 本实现的 `blend()` 方法遵循上述规范，在直通 Alpha (straight alpha) 空间进行合成。
 *
 * @section grayscale_conversion 灰度转换
 * 根据 ITU-R BT.709-6 和 sRGB 标准，人眼感知灰度采用以下加权平均：
 *
 * | 通道   | 权重   | 说明                         |
 * |--------|--------|------------------------------|
 * | 红色   | 0.299  | 人眼对红色敏感度             |
 * | 绿色   | 0.587  | 人眼对绿色最敏感             |
 * | 蓝色   | 0.114  | 人眼对蓝色最不敏感           |
 *
 * @section ansi_256_palette ANSI 256 色调色板
 * 根据 XTerm 256-Color 规范，调色板结构如下：
 *
 * | 索引范围   | 数量 | 说明                                         |
 * |------------|------|----------------------------------------------|
 * | 0-15       | 16   | 系统标准色（0-7 标准，8-15 高亮）            |
 * | 16-231     | 216  | 6×6×6 RGB 立方体（R,G,B ∈ {0,95,135,175,215,255}）|
 * | 232-255    | 24   | 灰度渐变（从 #080808 到 #EEEEEE）             |
 *
 * 6×6×6 颜色立方体的索引公式：`index = 16 + 36×R_idx + 6×G_idx + B_idx`
 *
 * @section basic_ansi_colors 基本 ANSI 8/16 色
 * 根据 ECMA-48 §8.3.117，基本 ANSI 颜色代码：
 *
 * | 颜色   | 前景码 | 背景码 | RGB 近似        |
 * |--------|--------|--------|-----------------|
 * | 黑色   | 30     | 40     | #000000         |
 * | 红色   | 31     | 41     | #CD0000         |
 * | 绿色   | 32     | 42     | #00CD00         |
 * | 黄色   | 33     | 43     | #CDCD00         |
 * | 蓝色   | 34     | 44     | #0000EE         |
 * | 品红   | 35     | 45     | #CD00CD         |
 * | 青色   | 36     | 46     | #00CDCD         |
 * | 白色   | 37     | 47     | #E5E5E5         |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 分量范围          | 0-255（8 位每通道）                       |
 * | Alpha 类型        | 直通 Alpha（non-premultiplied）           |
 * | 混合模式          | Normal（source-over）                      |
 * | 插值空间          | 直通 Alpha 空间                           |
 * | 灰度权重          | BT.709 / sRGB（0.299, 0.587, 0.114）      |
 * | 预乘转换          | 支持（to_premultiplied / from_premultiplied）|
 *
 * @section named_colors 预定义颜色常量
 * 本类提供以下预定义颜色常量：
 *
 * | 常量名       | RGB 值           | 说明         |
 * |--------------|------------------|--------------|
 * | black()      | 0, 0, 0, 255     | 黑色         |
 * | white()      | 255, 255, 255, 255 | 白色      |
 * | gray()       | 128, 128, 128, 255 | 灰色      |
 * | red()        | 255, 0, 0, 255     | 红色      |
 * | green()      | 0, 255, 0, 255     | 绿色      |
 * | blue()       | 0, 0, 255, 255     | 蓝色      |
 * | yellow()     | 255, 255, 0, 255   | 黄色      |
 * | magenta()    | 255, 0, 255, 255   | 品红      |
 * | cyan()       | 0, 255, 255, 255   | 青色      |
 * | transparent()| 0, 0, 0, 0        | 完全透明    |
 *
 * @note 本类采用直通 Alpha（straight alpha）表示，符合 CSS 和 SVG 标准。
 *       对于需要预乘 Alpha 的图形 API（如 OpenGL、DirectX、Metal），
 *       可使用 `to_premultiplied()` 方法进行转换。
 *
 * @warning 根据 W3C Compositing Level 1，混合操作应在直通 Alpha 空间进行，
 *          在预乘 Alpha 空间直接混合会产生错误的合成结果。
 *          在跨进程或跨 API 传输预乘颜色数据时务必注明格式。
 *
 * @see https://www.w3.org/TR/css-color-4/
 * @see https://www.w3.org/TR/compositing-1/
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
    int r = 0;   ///< 红分量
    int g = 0;   ///< 绿分量
    int b = 0;   ///< 蓝分量
    int a = 255; ///< 透明度分量

    /**
     * @brief 将整数值限制在0-255范围内
     * @param value 要限制的值
     * @return 限制后的值
     */
    static constexpr int clamp(const int value) noexcept {
        if (value < 0) {
            return 0;
        }
        if (value > 255) {
            return 255;
        }
        return value;
    }

    /**
     * @brief 将浮点数值限制在0.0-1.0范围内
     * @param value 要限制的值
     * @return 限制后的值
     */
    static constexpr double clamp_double(const double value) noexcept {
        if (value < 0.0) {
            return 0.0;
        }
        if (value > 1.0) {
            return 1.0;
        }
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
    constexpr color& operator=(const color& other) noexcept = default;

    /**
     * @brief 从灰度值构造
     * @param gray 灰度值（0-255）
     */
    explicit constexpr color(const int gray) noexcept :
    r(clamp(gray)),
    g(clamp(gray)),
    b(clamp(gray)) {}

    /**
     * @brief 从灰度值和透明度构造
     * @param gray 灰度值（0-255）
     * @param alpha 透明度（0-255）
     */
    constexpr color(const int gray, const int alpha) noexcept :
    r(clamp(gray)),
    g(clamp(gray)),
    b(clamp(gray)),
    a(clamp(alpha)) {}

    /**
     * @brief 从RGB分量构造
     * @param red 红色分量（0-255）
     * @param green 绿色分量（0-255）
     * @param blue 蓝色分量（0-255）
     */
    constexpr color(const int red, const int green, const int blue) noexcept :
    r(clamp(red)),
    g(clamp(green)),
    b(clamp(blue)) {}

    /**
     * @brief 从RGBA分量构造
     * @param red 红色分量（0-255）
     * @param green 绿色分量（0-255）
     * @param blue 蓝色分量（0-255）
     * @param alpha 透明度（0-255）
     */
    constexpr color(const int red, const int green, const int blue, const int alpha) noexcept :
    r(clamp(red)),
    g(clamp(green)),
    b(clamp(blue)),
    a(clamp(alpha)) {}

    /**
     * @brief 从十六进制字符串构造
     * @param str 十六进制字符串（格式：RRGGBB 或 RRGGBBAA）
     */
    NEFORCE_CONSTEXPR20 explicit color(const string_view str) { try_parse(str); }

    /**
     * @brief 从字符串对象构造
     * @param str 十六进制字符串
     */
    NEFORCE_CONSTEXPR20 explicit color(const string& str) { try_parse(str.view()); }

    /**
     * @brief 从C风格字符串构造
     * @param str 十六进制字符串
     */
    NEFORCE_CONSTEXPR20 explicit color(const char* str) { try_parse(string_view{str}); }

    /**
     * @brief 移动构造函数
     */
    constexpr color(color&& other) noexcept :
    r(other.r),
    g(other.g),
    b(other.b),
    a(other.a) {
        other.r = 0;
        other.g = 0;
        other.b = 0;
        other.a = 255;
    }

    /**
     * @brief 移动赋值运算符
     */
    constexpr color& operator=(color&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        this->swap(other);
        return *this;
    }

    NEFORCE_CONSTEXPR20 ~color() = default;

    /**
     * @brief 获取红色分量
     * @return 红色分量值
     */
    constexpr int R() const noexcept { return r; }

    /**
     * @brief 获取绿色分量
     * @return 绿色分量值
     */
    constexpr int G() const noexcept { return g; }

    /**
     * @brief 获取蓝色分量
     * @return 蓝色分量值
     */
    constexpr int B() const noexcept { return b; }

    /**
     * @brief 获取透明度
     * @return 透明度值
     */
    constexpr int A() const noexcept { return a; }

    /**
     * @brief 设置红色分量
     * @param red 红色分量值
     */
    constexpr void setR(const int red) noexcept { r = clamp(red); }

    /**
     * @brief 设置绿色分量
     * @param green 绿色分量值
     */
    constexpr void setG(const int green) noexcept { g = clamp(green); }

    /**
     * @brief 设置蓝色分量
     * @param blue 蓝色分量值
     */
    constexpr void setB(const int blue) noexcept { b = clamp(blue); }

    /**
     * @brief 设置透明度
     * @param alpha 透明度值
     */
    constexpr void setA(const int alpha) noexcept { a = clamp(alpha); }

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
    constexpr void set_gray(const int gray) noexcept { r = g = b = clamp(gray); }

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
    constexpr bool is_transparent() const noexcept { return a == 0; }

    /**
     * @brief 检查是否完全不透明
     * @return 是否完全不透明
     */
    constexpr bool is_opaque() const noexcept { return a == 255; }

    /**
     * @brief 获取不透明度（0.0-1.0）
     * @return 不透明度
     */
    constexpr double opacity() const noexcept { return a / 255.0; }

    /**
     * @brief 设置不透明度
     * @param opacity 不透明度（0.0-1.0）
     */
    constexpr void set_opacity(double opacity) noexcept {
        if (opacity < 0.0) {
            opacity = 0.0;
        }
        if (opacity > 1.0) {
            opacity = 1.0;
        }
        a = static_cast<int>(_NEFORCE round(opacity * 255));
    }

    /**
     * @brief 线性插值两个颜色
     * @param from 起始颜色
     * @param to 目标颜色
     * @param t 插值因子（0.0-1.0）
     * @return 插值结果
     * @note 插值在直通 Alpha (non-premultiplied) 空间进行，符合 W3C 标准。
     */
    static constexpr color lerp(const color& from, const color& to, double t) noexcept {
        t = clamp_double(t);
        return color(lerp_component(from.r, to.r, t), lerp_component(from.g, to.g, t), lerp_component(from.b, to.b, t),
                     lerp_component(from.a, to.a, t));
    }

    /**
     * @brief 颜色加法
     * @param other 另一个颜色
     * @return 相加后的颜色
     */
    constexpr color operator+(const color& other) const noexcept {
        return color(r + other.r, g + other.g, b + other.b, a + other.a);
    }

    /**
     * @brief 颜色减法
     * @param other 另一个颜色
     * @return 相减后的颜色
     */
    constexpr color operator-(const color& other) const noexcept {
        return color(r - other.r, g - other.g, b - other.b, a - other.a);
    }

    /**
     * @brief 颜色乘以标量
     * @param scalar 浮点标量
     * @return 缩放后的颜色
     */
    constexpr color operator*(const double scalar) const noexcept {
        return color(static_cast<int>(_NEFORCE round(r * scalar)), static_cast<int>(_NEFORCE round(g * scalar)),
                     static_cast<int>(_NEFORCE round(b * scalar)), static_cast<int>(_NEFORCE round(a * scalar)));
    }

    /**
     * @brief 颜色乘以整数标量
     * @param scalar 整数标量
     * @return 缩放后的颜色
     */
    constexpr color operator*(const int scalar) const noexcept {
        return {r * scalar, g * scalar, b * scalar, a * scalar};
    }

    /**
     * @brief 颜色加等赋值
     * @param other 另一个颜色
     * @return 自身引用
     */
    constexpr color& operator+=(const color& other) noexcept {
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
    constexpr color& operator-=(const color& other) noexcept {
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
    constexpr color& operator*=(const double scalar) noexcept {
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
    constexpr bool operator==(const color& other) const noexcept {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    /**
     * @brief 小于比较
     * @param other 另一个颜色
     * @return 是否小于
     */
    constexpr bool operator<(const color& other) const noexcept {
        if (r != other.r) {
            return r < other.r;
        }
        if (g != other.g) {
            return g < other.g;
        }
        if (b != other.b) {
            return b < other.b;
        }
        return a < other.a;
    }

    /**
     * @brief 将当前颜色与背景色混合
     * @param background 背景色
     * @return 混合后的颜色
     *
     * 符合 W3C Compositing and Blending Level 1 规范。
     */
    NEFORCE_NODISCARD constexpr color blend(const color& background) const noexcept {
        if (a == 255) {
            return *this;
        }
        if (a == 0) {
            return background;
        }

        const double src_a = a / 255.0;
        const double dst_a = background.a / 255.0;
        const double inv_src_a = 1.0 - src_a;

        // Result_A = Src_A + Dst_A * (1 - Src_A)
        const double out_a = src_a + dst_a * inv_src_a;

        if (out_a <= 0.0) {
            return transparent();
        }

        // Result_RGB = (Src_RGB * Src_A + Dst_RGB * Dst_A * (1 - Src_A)) / Result_A
        const double inv_out_a = 1.0 / out_a;
        const int newR = static_cast<int>(_NEFORCE round((r * src_a + background.r * dst_a * inv_src_a) * inv_out_a));
        const int newG = static_cast<int>(_NEFORCE round((g * src_a + background.g * dst_a * inv_src_a) * inv_out_a));
        const int newB = static_cast<int>(_NEFORCE round((b * src_a + background.b * dst_a * inv_src_a) * inv_out_a));
        const int newA = static_cast<int>(_NEFORCE round(out_a * 255.0));

        return color(newR, newG, newB, newA);
    }

    /**
     * @brief 计算灰度值
     * @return 灰度值（基于人眼感知的加权平均）
     */
    NEFORCE_NODISCARD constexpr double gray_value() const noexcept { return 0.299 * r + 0.587 * g + 0.114 * b; }

    /**
     * @brief 转换为十六进制字符串
     * @return 格式为RRGGBBAA的十六进制字符串
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const {
        return _NEFORCE format("{:02X}{:02X}{:02X}{:02X}", r, g, b, a);
    }

    /**
     * @brief 从十六进制字符串解析颜色
     * @param hex 十六进制字符串（支持#RRGGBB或#RRGGBBAA格式）
     * @return 解析得到的颜色
     * @throws value_exception 格式无效时抛出
     */
    static NEFORCE_CONSTEXPR20 color parse(const string_view hex) {
        string_view clean_hex = hex;
        if (clean_hex[0] == '#') {
            clean_hex = clean_hex.substr(1);
        }

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
            NEFORCE_THROW_EXCEPTION(value_exception("Invalid hex string"));
        }
        unreachable();
    }

    /**
     * @brief 转换为256色ANSI颜色索引
     * @return ANSI 256色索引（0-255）
     *
     * 符合 XTerm 256-Color Cube 规范。
     * 彩色部分使用标准区间: 0, 95, 135, 175, 215, 255
     */
    NEFORCE_NODISCARD constexpr int to_ansi_256() const noexcept {
        if (r == 0 && g == 0 && b == 0) {
            return 16;
        }
        if (r == 255 && g == 255 && b == 255) {
            return 231;
        }

        if (r == g && g == b) {
            if (r < 8) {
                return 16;
            }
            if (r > 248) {
                return 231;
            }
            const int gray_index = (r - 8) / 10;
            return 232 + (gray_index > 23 ? 23 : gray_index);
        }

        constexpr int cube_levels[6] = {0, 95, 135, 175, 215, 255};

        constexpr auto find_closest = [](const int value, const int levels[6]) -> int {
            int closest_idx = 0;
            int min_diff = 255 * 255;
            for (int i = 0; i < 6; ++i) {
                const int diff = value - levels[i];
                const int dist = diff * diff;
                if (dist < min_diff) {
                    min_diff = dist;
                    closest_idx = i;
                }
            }
            return closest_idx;
        };

        const int r_idx = find_closest(r, cube_levels);
        const int g_idx = find_closest(g, cube_levels);
        const int b_idx = find_closest(b, cube_levels);

        // 16 + 36 * R + 6 * G + B
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
            if (g > 128 && b > 128) {
                return base + 7;
            }
            return base + 1;
        } else if (g > r && g > b) {
            if (r > 128 && b > 128) {
                return base + 7;
            }
            return base + 2;
        } else if (b > r && b > g) {
            if (r > 128 && g > 128) {
                return base + 7;
            }
            return base + 4;
        } else if (r == g && g == b) {
            if (r < 64) {
                return base + 0;
            }
            if (r < 192) {
                return base + 7;
            }
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
            return integer32{38 * 100 + 5 * 10 + to_ansi_256()};
        }
        return integer32{to_ansi_basic(false)};
    }

    /**
     * @brief 转换为ANSI背景色代码
     * @param use_256_color 是否使用256色模式
     * @return ANSI背景色代码（包装为integer32）
     */
    NEFORCE_NODISCARD constexpr integer32 to_ansi_background(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return integer32{48 * 100 + 5 * 10 + to_ansi_256()};
        }
        return integer32{to_ansi_basic(true)};
    }

    /**
     * @brief 转换为标准预乘 Alpha 表示
     * @return 预乘后的颜色对象
     * @note 仅用于需要预乘数据的图形 API，结果不宜直接用于 blend 输入。
     */
    NEFORCE_NODISCARD constexpr color to_premultiplied() const noexcept {
        const double alpha = a / 255.0;
        return color(static_cast<int>(_NEFORCE round(r * alpha)), static_cast<int>(_NEFORCE round(g * alpha)),
                     static_cast<int>(_NEFORCE round(b * alpha)), a);
    }

    /**
     * @brief 从预乘 Alpha 转换回直通 Alpha（Straight Alpha）
     * @return 还原后的颜色对象
     * @note 符合 ISO 32000-2 对于 PDF 透明组的处理要求。
     */
    NEFORCE_NODISCARD constexpr color from_premultiplied() const noexcept {
        if (a == 0) {
            return transparent();
        }
        const double inv_alpha = 255.0 / a;
        return color(static_cast<int>(_NEFORCE round(r * inv_alpha)), static_cast<int>(_NEFORCE round(g * inv_alpha)),
                     static_cast<int>(_NEFORCE round(b * inv_alpha)), a);
    }

    /**
     * @brief 转换为整数表示
     * @param use_256_color 是否使用256色模式
     * @return 颜色索引（包装为integer32）
     */
    NEFORCE_NODISCARD constexpr integer32 to_integer32(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return integer32{to_ansi_256()};
        }
        return integer32{to_ansi_basic(false)};
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
    static constexpr color black() noexcept { return color(0, 0, 0, 255); }

    /** @brief 白色 */
    static constexpr color white() noexcept { return color(255, 255, 255, 255); }

    /** @brief 灰色 */
    static constexpr color gray() noexcept { return color(128, 128, 128, 255); }

    /** @brief 红色 */
    static constexpr color red() noexcept { return color(255, 0, 0, 255); }

    /** @brief 绿色 */
    static constexpr color green() noexcept { return color(0, 255, 0, 255); }

    /** @brief 蓝色 */
    static constexpr color blue() noexcept { return color(0, 0, 255, 255); }

    /** @brief 黄色 */
    static constexpr color yellow() noexcept { return color(255, 255, 0, 255); }

    /** @brief 品红 */
    static constexpr color magenta() noexcept { return color(255, 0, 255, 255); }

    /** @brief 青色 */
    static constexpr color cyan() noexcept { return color(0, 255, 255, 255); }

    /** @brief 完全透明 */
    static constexpr color transparent() noexcept { return color(0, 0, 0, 0); }
};

/**
 * @brief 标量乘以颜色
 * @param scalar 标量
 * @param color 颜色
 * @return 缩放后的颜色
 */
constexpr color operator*(const double scalar, const color& color) noexcept { return color * scalar; }

/**
 * @brief 整数标量乘以颜色
 * @param scalar 整数标量
 * @param color 颜色
 * @return 缩放后的颜色
 */
constexpr color operator*(const int scalar, const color& color) noexcept { return color * scalar; }

/** @} */ // Color

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_COLOR_HPP__
