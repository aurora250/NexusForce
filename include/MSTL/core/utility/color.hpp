#ifndef MSTL_CORE_UTILITY_COLOR_HPP__
#define MSTL_CORE_UTILITY_COLOR_HPP__
#include "../memory/hexadecimal.hpp"
#include "packages.hpp"
MSTL_BEGIN_NAMESPACE__

class color : public iobject<color> {
private:
    using self = color;

    int r = 0, g = 0, b = 0, a = 255;

    static constexpr int clamp(const int value) noexcept {
        if (value < 0) return 0;
        if (value > 255) return 255;
        return value;
    }

public:
    constexpr color() noexcept = default;

    explicit constexpr color(const int gray) noexcept
    : r(clamp(gray)), g(clamp(gray)), b(clamp(gray)) {}

    constexpr color(const int gray, const int alpha) noexcept
    : r(clamp(gray)), g(clamp(gray)), b(clamp(gray)), a(clamp(alpha)) {}

    constexpr color(const int red, const int green, const int blue) noexcept
    : r(clamp(red)), g(clamp(green)), b(clamp(blue)) {}

    constexpr color(const int red, const int green, const int blue, const int alpha) noexcept
    : r(clamp(red)), g(clamp(green)), b(clamp(blue)), a(clamp(alpha)) {}

    MSTL_CONSTEXPR20 explicit color(const string_view str) { try_parse(str); }
    MSTL_CONSTEXPR20 explicit color(const string& str) { try_parse(str.view()); }
    MSTL_CONSTEXPR20 explicit color(const char* str) { try_parse(str); }

    constexpr color(const color& other) noexcept = default;
    constexpr color& operator =(const color& other) noexcept = default;
    constexpr color(color&& other) noexcept : r(other.r), g(other.g), b(other.b), a(other.a) {
        other.r = 0;
        other.g = 0;
        other.b = 0;
        other.a = 255;
    }
    constexpr color& operator =(color&& other) noexcept {
        if (_MSTL addressof(other) == this) return *this;
        this->swap(other);
        return *this;
    }

    MSTL_CONSTEXPR20 ~color() = default;

    constexpr int R() const noexcept { return r; }
    constexpr int G() const noexcept { return g; }
    constexpr int B() const noexcept { return b; }
    constexpr int A() const noexcept { return a; }

    constexpr void setR(const int red) noexcept { r = clamp(red); }
    constexpr void setG(const int green) noexcept { g = clamp(green); }
    constexpr void setB(const int blue) noexcept { b = clamp(blue); }
    constexpr void setA(const int alpha) noexcept { a = clamp(alpha); }

    constexpr void set_color(const int red, const int green, const int blue) noexcept {
        r = clamp(red);
        g = clamp(green);
        b = clamp(blue);
    }

    constexpr void set_color(const int red, const int green, const int blue, const int alpha) noexcept {
        r = clamp(red);
        g = clamp(green);
        b = clamp(blue);
        a = clamp(alpha);
    }

    constexpr void set_gray(const int gray) noexcept {
        r = g = b = clamp(gray);
    }

    constexpr void set_gray(const int gray, const int alpha) noexcept {
        set_gray(gray);
        a = clamp(alpha);
    }

    constexpr bool is_transparent() const noexcept { return a == 0; }
    constexpr bool is_opaque() const noexcept { return a == 255; }
    constexpr double is_opacity() const noexcept { return a / 255.0; }
    constexpr void set_opacity(double opacity) noexcept {
        if (opacity < 0.0) opacity = 0.0;
        if (opacity > 1.0) opacity = 1.0;
        a = static_cast<int>(_MSTL round(opacity * 255));
    }

    constexpr color operator +(const color& other) const noexcept {
        return color(r + other.r, g + other.g, b + other.b, a + other.a);
    }

    constexpr color operator -(const color& other) const noexcept {
        return color(r - other.r, g - other.g, b - other.b, a - other.a);
    }

    constexpr color operator *(const double scalar) const noexcept {
        return color(
            static_cast<int>(_MSTL round(r * scalar)),
            static_cast<int>(_MSTL round(g * scalar)),
            static_cast<int>(_MSTL round(b * scalar)),
            static_cast<int>(_MSTL round(a * scalar))
        );
    }

    constexpr color operator *(const int scalar) const noexcept {
        return {r * scalar, g * scalar, b * scalar, a * scalar};
    }

    constexpr color& operator +=(const color& other) noexcept {
        r = clamp(r + other.r);
        g = clamp(g + other.g);
        b = clamp(b + other.b);
        a = clamp(a + other.a);
        return *this;
    }

    constexpr color& operator -=(const color& other) noexcept {
        r = clamp(r - other.r);
        g = clamp(g - other.g);
        b = clamp(b - other.b);
        a = clamp(a - other.a);
        return *this;
    }

    constexpr color& operator *=(const double scalar) noexcept {
        r = clamp(static_cast<int>(_MSTL round(r * scalar)));
        g = clamp(static_cast<int>(_MSTL round(g * scalar)));
        b = clamp(static_cast<int>(_MSTL round(b * scalar)));
        a = clamp(static_cast<int>(_MSTL round(a * scalar)));
        return *this;
    }

    constexpr bool operator ==(const color& other) const noexcept {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    constexpr bool operator <(const color& other) const noexcept {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        if (b != other.b) return b < other.b;
        return a < other.a;
    }

    MSTL_NODISCARD constexpr color blend(const color& background) const noexcept {
        if (a == 255) return *this;
        if (a == 0) return background;

        const double alpha = a / 255.0;
        const double inv_alpha = 1.0 - alpha;

        const int newR = static_cast<int>(_MSTL round(r * alpha + background.r * inv_alpha));
        const int newG = static_cast<int>(_MSTL round(g * alpha + background.g * inv_alpha));
        const int newB = static_cast<int>(_MSTL round(b * alpha + background.b * inv_alpha));
        return color(newR, newG, newB, 255);
    }

    MSTL_NODISCARD constexpr color premultiply_alpha() const noexcept {
        const double alpha = a / 255.0;
        return color(
            static_cast<int>(_MSTL round(r * alpha)),
            static_cast<int>(_MSTL round(g * alpha)),
            static_cast<int>(_MSTL round(b * alpha)),
            a
        );
    }

    MSTL_NODISCARD constexpr double gray_value() const noexcept {
        return 0.299 * r + 0.587 * g + 0.114 * b;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _MSTL format("{02X}{02X}{02X}{02X}", r, g, b, a);
    }

    static MSTL_CONSTEXPR20 color parse(const string_view hex) {
        string_view clean_hex = hex;
        if (clean_hex[0] == '#') clean_hex = clean_hex.substr(1);

        if (clean_hex.length() == 6) {
            const int r = hexadecimal::parse(clean_hex.substr(0, 2)).to_int64();
            const int g = hexadecimal::parse(clean_hex.substr(2, 2)).to_int64();
            const int b = hexadecimal::parse(clean_hex.substr(4, 2)).to_int64();
            return {r, g, b};
        } else if (clean_hex.length() == 8) {
            const int r = hexadecimal::parse(clean_hex.substr(0, 2)).to_int64();
            const int g = hexadecimal::parse(clean_hex.substr(2, 2)).to_int64();
            const int b = hexadecimal::parse(clean_hex.substr(4, 2)).to_int64();
            const int a = hexadecimal::parse(clean_hex.substr(6, 2)).to_int64();
            return {r, g, b, a};
        } else {
            throw_exception(value_exception("Invalid hex string"));
        }
        return {};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        self tmp;
        try {
            tmp = self::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }

    MSTL_NODISCARD constexpr int to_ansi_256() const noexcept {
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

    MSTL_NODISCARD constexpr int to_ansi_basic(const bool is_background = false) const noexcept {
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

    MSTL_NODISCARD constexpr integer32 to_ansi_foreground(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return {38 * 100 + 5 * 10 + to_ansi_256()};
        }
        return {to_ansi_basic(false)};
    }

    MSTL_NODISCARD constexpr integer32 to_ansi_background(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return {48 * 100 + 5 * 10 + to_ansi_256()};
        }
        return {to_ansi_basic(true)};
    }

    MSTL_NODISCARD constexpr integer32 to_integer32(const bool use_256_color = true) const noexcept {
        if (use_256_color) {
            return {to_ansi_256()};
        }
        return {to_ansi_basic(false)};
    }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        constexpr hash<int> hasher;
        return hasher(r) ^ hasher(g) ^ hasher(b) ^ hasher(a);
    }

    constexpr void swap(self& other) noexcept {
        self tmp = _MSTL move(other);
        other = _MSTL move(*this);
        *this = _MSTL move(tmp);
    }

    static constexpr color black() noexcept { return color(0, 0, 0, 255); }
    static constexpr color white() noexcept { return color(255, 255, 255, 255); }
    static constexpr color red() noexcept { return color(255, 0, 0, 255); }
    static constexpr color green() noexcept { return color(0, 255, 0, 255); }
    static constexpr color blue() noexcept { return color(0, 0, 255, 255); }
    static constexpr color yellow() noexcept { return color(255, 255, 0, 255); }
    static constexpr color magenta() noexcept { return color(255, 0, 255, 255); }
    static constexpr color cyan() noexcept { return color(0, 255, 255, 255); }
    static constexpr color transparent() noexcept { return color(0, 0, 0, 0); }
};

constexpr color operator *(const double scalar, const color& color) noexcept {
    return color * scalar;
}
constexpr color operator *(const int scalar, const color& color) noexcept {
    return color * scalar;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_COLOR_HPP__
