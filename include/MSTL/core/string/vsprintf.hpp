#ifndef MSTL_CORE_UTILITY_VSPRINTF_HPP__
#define MSTL_CORE_UTILITY_VSPRINTF_HPP__
#include <cstdarg>
#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

int MSTL_API vsprintf(char *buf, const char *fmt, std::va_list args) noexcept;
int MSTL_API vsnprintf(char *buf, size_t size, const char *fmt, std::va_list args) noexcept;
int MSTL_API sprintf(char *buf, const char *fmt, ...) noexcept;
int MSTL_API snprintf(char *buf, size_t size, const char *fmt, ...) noexcept;
int MSTL_API scprintf(const char *fmt, ...) noexcept;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_VSPRINTF_HPP__
