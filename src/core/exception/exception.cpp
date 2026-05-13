#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/stacktrace.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    ifdef NEFORCE_COMPILER_MSVC
#        include <eh.h>
#    endif
#    ifdef NEFORCE_COMPILER_MINGW
extern "C" int __cxa_uncaught_exception() __attribute__((weak));
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <exception>
#endif
NEFORCE_BEGIN_NAMESPACE__

int uncaught_exceptions() noexcept {
#ifdef NEFORCE_PLATFORM_LINUX
#ifdef NEFORCE_STANDARD_17
    return std::uncaught_exceptions();
#else
    return static_cast<int>(std::uncaught_exception());
#endif
#else
#    ifdef NEFORCE_COMPILER_MSVC
    return ::__uncaught_exceptions();
#    else
    return ::__cxa_uncaught_exception();
#    endif
#endif
}

void throw_with_stack(const exception& err) {
    printcln(color::red(), "\nException : (", err.type(), ") ", err.what());
    printcln(color::red(), stacktrace());
    throw err;
}

NEFORCE_END_NAMESPACE__
