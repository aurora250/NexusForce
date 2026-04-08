#include <NeForce/core/exception/breakpoint.hpp>
#include <NeForce/core/system/console.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <debugapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/file/file.hpp>
#endif
NEFORCE_BEGIN_NAMESPACE__

bool is_debugger_present() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::IsDebuggerPresent() != 0;
#else
    file status_file(path{"/proc/self/status"});
    if (!status_file.open()) {
        return false;
    }

    string line;
    size_t pos = 0;
    const string read = status_file.read();
    while (getline(read, pos, line)) {
        if (line.starts_with("TracerPid:")) {
            const size_t f = line.find_first_of("0123456789");
            if (f != string::npos) {
                return to_int32(line.view(f)) != 0;
            }
        }
    }
    return false;
#endif
}

void debug_assert(bool condition, const char* message) {
    if (!condition) {
        println(color::red(), "Debug assertion failed:", message);
        breakpoint_if_debugging();
    }
}

NEFORCE_END_NAMESPACE__
