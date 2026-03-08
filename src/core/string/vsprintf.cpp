#include <NeForce/core/string/vsprintf.hpp>
#include <NeForce/core/numeric/numeric_types.hpp>
#include <NeForce/core/memory/standard_allocator.hpp>
#include <NeForce/core/string/char_types.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    NEFORCE_CONST_FUNCTION constexpr int skip_atoi(const char **s) {
        int i = 0;
        while (_NEFORCE is_digit(**s)) {
            i = i * 10 + *((*s)++) - '0';
        }
        return i;
    }

    constexpr int32_t ZEROPAD = 1;
    constexpr int32_t SIGN = 2;
    constexpr int32_t PLUS = 4;
    constexpr int32_t SPACE = 8;
    constexpr int32_t LEFT = 16;
    constexpr int32_t SPECIAL = 32;
    constexpr int32_t SMALL = 64;

    constexpr uint32_t do_div(uint32_t* n, const uint32_t base) {
        const uint32_t remainder = *n % base;
        *n = *n / base;
        return remainder;
    }

    NEFORCE_CONSTEXPR20 char* number(
        char* str, const int64_t num,
        const int base, int size,
        int precision, int type) {

        char sign = 0, tmp[66];
        auto digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        if (type & SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
        if (type & LEFT) type &= ~ZEROPAD;
        if (base < 2 || base > 36) {
            *str++ = '[';
            *str++ = 'E';
            *str++ = ']';
            return str;
        }

        const char c = (type & ZEROPAD) ? '0' : ' ';
        unsigned long long unum;

        if (type & SIGN && num < 0) {
            sign = '-';
            unum = -static_cast<unsigned long long>(num);
        } else {
            sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
            unum = static_cast<unsigned long long>(num);
        }

        if (sign) size--;

        if (type & SPECIAL) {
            if (base == 16) size -= 2;
            else if (base == 8) size--;
        }

        int i = 0;
        if (unum == 0) {
            tmp[i++] = '0';
        } else {
            while (unum != 0) {
                const auto digit = static_cast<unsigned int>(unum % base);
                unum /= base;
                tmp[i++] = digits[digit];
            }
        }

        if (i > precision) precision = i;
        size -= precision;

        if (!(type & (ZEROPAD + LEFT))) {
            while(size-- > 0) *str++ = ' ';
        }

        if (sign) *str++ = sign;

        if (type & SPECIAL) {
            if (base == 8) {
                *str++ = '0';
            } else if (base == 16) {
                *str++ = '0';
                *str++ = (type & SMALL) ? 'x' : 'X';
            }
        }

        if (!(type & LEFT)) {
            while(size-- > 0) *str++ = c;
        }
        while(i < precision--) *str++ = '0';
        while(i-- > 0) *str++ = tmp[i];
        while(size-- > 0) *str++ = ' ';

        return str;
    }

    NEFORCE_CONSTEXPR20 char* float_number(
        char* str, double num,
        const int field_width, int precision,
        const int flags) {

        char sign = 0;
        char int_buf[64];
        char frac_buf[64];
        int int_len = 0;
        int frac_len = 0;

        if (_NEFORCE is_nan(num)) {
            const char* nan_str = (flags & SMALL) ? "nan" : "NAN";
            return _NEFORCE string_copy(str, nan_str);
        }
        if (_NEFORCE is_infinity(num)) {
            const char* inf_str = (flags & SMALL) ? "inf" : "INF";
            if (num < 0) {
                sign = '-';
            } else {
                sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);
            }

            if (sign) *str++ = sign;
            return _NEFORCE string_copy(str, inf_str);
        }

        if (num < 0) {
            sign = '-';
            num = -num;
        } else {
            sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);
        }

        if (precision < 0) precision = 6;
        if (precision > 20) precision = 20;

        auto int_part = static_cast<long long>(num);
        double fractional = num - static_cast<double>(int_part);

        if (int_part == 0) {
            int_buf[int_len++] = '0';
        } else {
            long long temp = int_part;
            while (temp != 0) {
                int_buf[int_len++] = '0' + static_cast<char>(temp % 10);
                temp /= 10;
            }
            for (int i = 0; i < int_len / 2; ++i) {
                const char tmp = int_buf[i];
                int_buf[i] = int_buf[int_len - 1 - i];
                int_buf[int_len - 1 - i] = tmp;
            }
        }

        if (precision > 0) {
            double scale = 1.0;
            for (int i = 0; i < precision; ++i) {
                scale *= 10.0;
            }

            fractional = fractional * scale + 0.5;
            auto frac_val = static_cast<long long>(fractional);

            if (frac_val >= static_cast<long long>(scale)) {
                frac_val -= static_cast<long long>(scale);
                int_part++;
                if (int_part > 0) {
                    int_len = 0;
                    long long temp = int_part;
                    while (temp != 0) {
                        int_buf[int_len++] = '0' + static_cast<char>(temp % 10);
                        temp /= 10;
                    }
                    for (int i = 0; i < int_len / 2; ++i) {
                        const char tmp = int_buf[i];
                        int_buf[i] = int_buf[int_len - 1 - i];
                        int_buf[int_len - 1 - i] = tmp;
                    }
                }
            }

            for (int i = precision - 1; i >= 0; --i) {
                frac_buf[i] = '0' + static_cast<char>(frac_val % 10);
                frac_val /= 10;
            }
            frac_len = precision;
        }

        const int sign_len = (sign != 0) ? 1 : 0;
        const int dot_len = (precision > 0) ? 1 : 0;
        const int total_num_len = int_len + dot_len + frac_len;
        const int total_len = sign_len + total_num_len;
        int pad = field_width > total_len ? field_width - total_len : 0;

        if (!(flags & LEFT) && !(flags & ZEROPAD)) {
            while (pad-- > 0) *str++ = ' ';
        }

        if (sign) *str++ = sign;

        if (!(flags & LEFT) && (flags & ZEROPAD)) {
            while (pad-- > 0) *str++ = '0';
        }

        for (int i = 0; i < int_len; ++i) {
            *str++ = int_buf[i];
        }

        if (precision > 0) {
            *str++ = '.';
            for (int i = 0; i < frac_len; ++i) {
                *str++ = frac_buf[i];
            }
        }

        if (flags & LEFT) {
            while (pad-- > 0) *str++ = ' ';
        }

        return str;
    }
}


int vsprintf(char *buf, const char *fmt, std::va_list args) noexcept {
    if (!buf || !fmt) return -1;

    char* str;
    char* end = buf + MEMORY_BIG_ALLOC_THRESHHOLD - 1;

    for (str = buf; *fmt; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		int flags = 0;
		bool break_flag = false;
		while(!break_flag) {
			++fmt;
			switch (*fmt) {
				case '-': flags |= LEFT; break;
				case '+': flags |= PLUS; break;
				case ' ': flags |= SPACE; break;
				case '#': flags |= SPECIAL; break;
				case '0': flags |= ZEROPAD; break;
				default: break_flag = true; break;
			}
		}

		int field_width = -1;
		if (_NEFORCE is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		int precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (_NEFORCE is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				precision = va_arg(args, int);
			}
			if (precision < 0) precision = 0;
		}

		int qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		switch (*fmt) {
		    case 'c': {
		        if (!(flags & LEFT)) {
		            while (--field_width > 0) *str++ = ' ';
		        }
		        *str++ = static_cast<byte_t>(va_arg(args, int));
		        while (--field_width > 0) *str++ = ' ';
		        break;
		    }
		    case 's': {
                const char *s = va_arg(args, char *);
		        if (!s) s = "(null)";
		        int len = _NEFORCE string_length(s);
		        if (precision < 0) {
		            precision = len;
		        } else if (len > precision) {
		            len = precision;
		        }

		        if (!(flags & LEFT)) {
		            while (len < field_width--) *str++ = ' ';
		        }

		        for (int i = 0; i < len; ++i) *str++ = *s++;
		        while (len < field_width--) *str++ = ' ';
		        break;
		    }
		    case 'o': {
		        str = number(str, va_arg(args, unsigned long), 8,
                    field_width, precision, flags);
		        break;
		    }
		    case 'p': {
		        if (field_width == -1) {
		            field_width = 8;
		            flags |= ZEROPAD;
		        }
		        str = number(str,
                    reinterpret_cast<size_t>(va_arg(args, void *)), 16,
                    field_width, precision, flags);
		        break;
		    }
		    case 'x': {
		        flags |= SMALL;
		    }
		    case 'X': {
		        str = number(str, va_arg(args, unsigned long), 16,
                    field_width, precision, flags);
		        break;
		    }
		    case 'd': case 'i': {
		        flags |= SIGN;
		    }
		    case 'u': {
		        str = number(str, va_arg(args, unsigned long), 10,
                    field_width, precision, flags);
		        break;
		    }
		    case 'f': {
		        if (qualifier == 'L') {
		            const long double ld = va_arg(args, long double);
		            str = float_number(str, static_cast<double>(ld), field_width, precision, flags);
		        } else {
		            const double d = va_arg(args, double);
		            str = float_number(str, d, field_width, precision, flags);
		        }
		        break;
		    }
		    case 'n': {
		        int* ip = va_arg(args, int *);
		        *ip = (str - buf);
		        break;
		    }
		    default: {
		        if (*fmt != '%')
		            *str++ = '%';
		        if (*fmt)
		            *str++ = *fmt;
		        else
		            --fmt;
		        break;
		    }
		}
	}

    if (str < end) {
        *str = '\0';
    } else {
        *(end - 1) = '\0';
    }

	return static_cast<int>(str - buf);
}

int vsnprintf(char *buf, const size_t size, const char *fmt, std::va_list args) noexcept {
    if (!buf || size == 0 || !fmt) return -1;

    char temp[MEMORY_BIG_ALLOC_THRESHHOLD];
    int len = _NEFORCE vsprintf(temp, fmt, args);

    if (len < 0) {
        buf[0] = '\0';
        return -1;
    }

    if (len >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        len = MEMORY_BIG_ALLOC_THRESHHOLD - 1;
        temp[len] = '\0';
    }

    if (size > 0) {
        const size_t copy_len = (len < size - 1) ? len : size - 1;
        _NEFORCE memory_copy(buf, temp, copy_len);
        buf[copy_len] = '\0';
    }
    return len;
}

int sprintf(char *buf, const char *fmt, ...) noexcept {
    std::va_list args;

    va_start(args, fmt);
    const int result = _NEFORCE vsprintf(buf, fmt, args);
    va_end(args);

    return result;
}

int snprintf(char *buf, const size_t size, const char *fmt, ...) noexcept {
    std::va_list args;

    va_start(args, fmt);
    const int result = _NEFORCE vsnprintf(buf, size, fmt, args);
    va_end(args);

    return result;
}

int scprintf(const char *fmt, ...) noexcept {
    std::va_list args;

    va_start(args, fmt);
    char temp[MEMORY_BIG_ALLOC_THRESHHOLD];
    const int length = _NEFORCE vsprintf(temp, fmt, args);
    va_end(args);

    return length;
}

NEFORCE_END_NAMESPACE__
