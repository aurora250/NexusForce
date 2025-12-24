#include <MSTL/network/tcp_socket.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/exception/terminate.hpp>
#include <cstdlib>
#endif
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_PLATFORM_WINDOWS__
bool winsock_initialized() {
    static bool initialized = []() -> bool {
        ::WSADATA wsa_data;
        const int result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            return false;
        }
        std::atexit([]() {
            ::WSACleanup();
        });
        return true;
    }();
    return initialized;
}
#endif

MSTL_END_NAMESPACE__
