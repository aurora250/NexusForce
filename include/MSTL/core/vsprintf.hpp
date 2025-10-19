#ifndef MSTL_VSPRINTF_HPP__
#define MSTL_VSPRINTF_HPP__
#include "environment.hpp"
#include <cstdarg>
MSTL_BEGIN_NAMESPACE__

int MSTL_API vsprintf(char *buf, const char *fmt, ::va_list args);
int MSTL_API vsnprintf(char *buf, size_t size, const char *fmt, ::va_list args);
int MSTL_API sprintf(char *buf, const char *fmt, ...);
int MSTL_API snprintf(char *buf, size_t size, const char *fmt, ...);
int MSTL_API scprintf(const char *fmt, ...);

MSTL_END_NAMESPACE__
#endif // MSTL_VSPRINTF_HPP__
