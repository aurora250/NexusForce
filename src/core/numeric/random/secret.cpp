#include <NeForce/core/exception/system_exception.hpp>
#include <NeForce/core/numeric/random/secret.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <minwinbase.h>
#    include <minwindef.h>
#    include <wincrypt.h>
#    include <bcrypt.h>
#    include <intrin.h>
#    include <winternl.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/random.h>
#    include <sys/fcntl.h>
#    include <unistd.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

bool secret::system_supported() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    // Vista+
    uint8_t probe = 0;
    const ::NTSTATUS status = ::BCryptGenRandom(nullptr, &probe, sizeof(probe), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return NT_SUCCESS(status);
#else
    uint8_t probe = 0;
    const ssize_t ret = ::getrandom(&probe, sizeof(probe), 0);
    if (ret == 1) {
        return true;
    }
    const int fd = ::open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return false;
    }
    ::close(fd);
    return true;
#endif
}

void secret::get_random_bytes(byte_t* buffer, size_t length) {
    if (buffer == nullptr || length == 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid buffer or length"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::NTSTATUS status = ::BCryptGenRandom(nullptr, reinterpret_cast<::PUCHAR>(buffer),
                                                static_cast<::ULONG>(length), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!(NT_SUCCESS(status))) {
        NEFORCE_THROW_EXCEPTION(device_exception("BCryptGenRandom failed"));
    }
#else
    size_t bytes_filled = 0;

    while (bytes_filled < length) {
        const size_t chunk = (length - bytes_filled > 256) ? 256 : (length - bytes_filled);
        const ssize_t ret = ::getrandom(buffer + bytes_filled, chunk, 0);

        if (ret > 0) {
            bytes_filled += static_cast<size_t>(ret);
            continue;
        }

        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOSYS) {
                break;
            }
            NEFORCE_THROW_EXCEPTION(device_exception("getrandom failed with unexpected error"));
        }
    }

    if (bytes_filled >= length) {
        return;
    }

    const int fd = ::open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        NEFORCE_THROW_EXCEPTION(file_exception("Failed to open /dev/urandom"));
    }

    while (bytes_filled < length) {
        const ssize_t result = ::read(fd, buffer + bytes_filled, length - bytes_filled);

        if (result > 0) {
            bytes_filled += static_cast<size_t>(result);
            continue;
        }

        if (result == -1) {
            if (errno == EINTR) {
                continue;
            }
            ::close(fd);
            NEFORCE_THROW_EXCEPTION(system_exception("Failed to read from /dev/urandom"));
        }

        ::close(fd);
        NEFORCE_THROW_EXCEPTION(system_exception("/dev/urandom returned EOF unexpectedly"));
    }

    ::close(fd);
#endif
}

NEFORCE_END_NAMESPACE__
