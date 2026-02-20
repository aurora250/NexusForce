#include <MSTL/core/exception/breakpoint.hpp>
#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <debugapi.h>
#else
#include <MSTL/core/file/file.hpp>
#endif
MSTL_BEGIN_NAMESPACE__

bool is_debugger_present() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::IsDebuggerPresent() != 0;
#else
    file status_file(path{"/proc/self/status"});
    if (!status_file.open()) {
        return false;
    }

    string line;
    size_t pos = 0;
    const string read = status_file.read();
    while (_MSTL getline(read, pos, line)) {
        if (line.compare(0, 10, "TracerPid:") == 0) {
            const size_t f = line.find_first_of("0123456789");
            if (f != string::npos) {
                return to_int32(line.view(f)) != 0;
            }
        }
    }
    return false;
#endif
}

void debug_assert(bool condition, const char* message) noexcept {
    if (!condition) {
        println(color::red(), "Debug assertion failed:", message);
        breakpoint_if_debugging();
    }
}

MSTL_END_NAMESPACE__
