#include <MSTL/core/vsprintf.hpp>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

static constexpr int skip_atoi(const char **s) {
	int i = 0;
	while (_INNER is_digit(**s)) {
	    i = i * 10 + *((*s)++) - '0';
	}
	return i;
}

MSTL_INLINE17 int32_t constexpr ZEROPAD = 1;
MSTL_INLINE17 int32_t constexpr SIGN = 2;
MSTL_INLINE17 int32_t constexpr PLUS = 4;
MSTL_INLINE17 int32_t constexpr SPACE = 8;
MSTL_INLINE17 int32_t constexpr LEFT = 16;
MSTL_INLINE17 int32_t constexpr SPECIAL = 32;
MSTL_INLINE17 int32_t constexpr SMALL = 64;

MSTL_INLINE17 int32_t constexpr MAX_VSPRINTF_BUFFER_SIZE = 4096;

static constexpr unsigned int do_div(unsigned int* n, const unsigned int base) {
    const unsigned int remainder = *n % base;
    *n = *n / base;
    return remainder;
}

static MSTL_CONSTEXPR20 char* number(char * str, unsigned int num,
    const int base, int size, int precision ,int type) {
	char sign, tmp[36];
    auto digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (type & SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (type & LEFT) type &= ~ZEROPAD;
	if (base < 2 || base > 36) return nullptr;

    const char c = (type & ZEROPAD) ? '0' : ' ';
	if (type & SIGN && num < 0) {
		sign = '-';
		num = -num;
	} else {
	    sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	}
	if (sign) size--;
	if (type & SPECIAL) {
		if (base == 16) size -= 2;
		else if (base == 8) size--;
	}

	int i = 0;
	if (num == 0) {
	    tmp[i++] = '0';
	}
	else {
	    while (num != 0) {
	        tmp[i++] = digits[do_div(&num, base)];
	    }
	}

	if (i > precision) precision = i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT))) {
	    while(size --> 0) *str++ = ' ';
	}

	if (sign) *str++ = sign;
	if (type & SPECIAL) {
		if (base == 8) {
		    *str++ = '0';
		} else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}

	if (!(type & LEFT)) while(size --> 0) *str++ = c;
	while(i < precision--) *str++ = '0';
	while(i-- > 0) *str++ = tmp[i];
	while(size-- > 0) *str++ = ' ';
	return str;
}

static MSTL_CONSTEXPR20 char* float_number(char* str, double num,
    const int field_width, int precision, const int flags) {
    char sign = 0;
    long long frac_part = 0;
    char int_buf[64];
    char frac_buf[64];
    int int_len = 0;
    int frac_len = 0;

    if (num < 0) {
        sign = '-';
        num = -num;
    }
    else {
        if (flags & PLUS) sign = '+';
        else if (flags & SPACE) sign = ' ';
    }

    if (precision < 0) precision = 6;
    if (precision > DECIMAL_MAX_DIGITS) precision = DECIMAL_MAX_DIGITS;

    long long int_part = static_cast<long long>(num);
    double frac = num - int_part;

    if (precision > 0) {
        double scale = 1.0;
        for (int i = 0; i < precision; ++i) scale *= 10.0;

        frac += 0.5 / scale;
        frac_part = static_cast<long long>(frac * scale);

        if (frac_part >= static_cast<long long>(scale)) {
            frac_part -= static_cast<long long>(scale);
            int_part += 1;
        }
    }

    if (int_part == 0) {
        int_buf[int_len++] = '0';
    }
    else {
        while (int_part != 0) {
            int_buf[int_len++] = '0' + static_cast<int>(int_part % 10);
            int_part /= 10;
        }
    }

    if (precision > 0) {
        long long temp = frac_part;
        for (int i = 0; i < precision; ++i) {
            frac_buf[precision - 1 - i] = '0' + (temp % 10);
            temp /= 10;
        }
        frac_len = precision;
    }

    const int sign_len = (sign != 0) ? 1 : 0;
    const int dot_len = (precision > 0) ? 1 : 0;
    const int total_num_len = int_len + dot_len + frac_len;
    const int total_len = sign_len + total_num_len;
    int pad = field_width - total_len;
    if (pad < 0) pad = 0;

    if (!(flags & LEFT) && !(flags & ZEROPAD)) {
        while (pad-- > 0) *str++ = ' ';
    }

    if (sign != 0) *str++ = sign;

    if (!(flags & LEFT) && (flags & ZEROPAD)) {
        while (pad-- > 0) *str++ = '0';
    }
    for (int i = int_len - 1; i >= 0; --i) {
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

MSTL_END_INNER__


int vsprintf(char *buf, const char *fmt, ::va_list args) {
	int len, i, *ip;
	char *str, *s;

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
				case '-': flags |= _INNER LEFT; break;
				case '+': flags |= _INNER PLUS; break;
				case ' ': flags |= _INNER SPACE; break;
				case '#': flags |= _INNER SPECIAL; break;
				case '0': flags |= _INNER ZEROPAD; break;
				default: break_flag = true; break;
			}
		}

		int field_width = -1;
		if (_INNER is_digit(*fmt))
			field_width = _INNER skip_atoi(&fmt);
		else if (*fmt == '*') {
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= _INNER LEFT;
			}
		}

		int precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (_INNER is_digit(*fmt))
				precision = _INNER skip_atoi(&fmt);
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
		case 'c':
			if (!(flags & _INNER LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = static_cast<unsigned char>(va_arg(args, int));
			while (--field_width > 0)
				*str++ = ' ';
			break;

		case 's':
			s = va_arg(args, char *);
			len = _MSTL string_length(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;

			if (!(flags & _INNER LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			break;

		case 'o':
			str = _INNER number(str, va_arg(args, unsigned long), 8,
				field_width, precision, flags);
			break;

		case 'p':
			if (field_width == -1) {
				field_width = 8;
				flags |= _INNER ZEROPAD;
			}
			str = _INNER number(str,
				reinterpret_cast<size_t>(va_arg(args, void *)), 16,
				field_width, precision, flags);
			break;

		case 'x':
			flags |= _INNER SMALL;
		case 'X':
			str = _INNER number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		case 'd':
		case 'i':
			flags |= _INNER SIGN;
		case 'u':
			str = _INNER number(str, va_arg(args, unsigned long), 10,
				field_width, precision, flags);
			break;

        case 'f':
            if (qualifier == 'L') {
                const long double ld = va_arg(args, long double);
                str = _INNER float_number(str, static_cast<double>(ld), field_width, precision, flags);
            } else {
                const double d = va_arg(args, double);
                str = _INNER float_number(str, d, field_width, precision, flags);
            }
		    break;
		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;

		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}
	*str = '\0';
	return str - buf;
}

int vsnprintf(char *buf, const size_t size, const char *fmt, ::va_list args) {
    char temp[_INNER MAX_VSPRINTF_BUFFER_SIZE];
    int len = _MSTL vsprintf(temp, fmt, args);

    if (len < 0) {
        return -1;
    }

    if (len >= _INNER MAX_VSPRINTF_BUFFER_SIZE) {
        len = _INNER MAX_VSPRINTF_BUFFER_SIZE - 1;
        temp[len] = '\0';
    }

    if (size > 0) {
        const size_t copy_len = (len < size - 1) ? len : size - 1;
        _MSTL memory_copy(buf, temp, copy_len);
        buf[copy_len] = '\0';
    }
    return len;
}

int sprintf(char *buf, const char *fmt, ...) {
    ::va_list args;

    va_start(args, fmt);
    const int result = _MSTL vsprintf(buf, fmt, args);
    va_end(args);

    return result;
}

int snprintf(char *buf, const size_t size, const char *fmt, ...) {
    ::va_list args;

    va_start(args, fmt);
    const int result = _MSTL vsnprintf(buf, size, fmt, args);
    va_end(args);

    return result;
}

int scprintf(const char *fmt, ...) {
    ::va_list args;

    va_start(args, fmt);
    char temp[_INNER MAX_VSPRINTF_BUFFER_SIZE];
    const int length = _MSTL vsprintf(temp, fmt, args);
    va_end(args);

    return length;
}

MSTL_END_NAMESPACE__
