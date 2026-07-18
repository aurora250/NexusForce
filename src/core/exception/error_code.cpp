#include <NeForce/core/exception/error_code.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <minwindef.h>
#    include <WinBase.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

error_code last_error() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return {static_cast<int32_t>(::GetLastError()), system_category()};
#else
    return {errno, system_category()};
#endif
}

NEFORCE_END_NAMESPACE__
