#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/time/datetime.hpp>
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

random_lcd::random_lcd() noexcept :
seed_(static_cast<seed_type>(timestamp::now().value())) {}

void random_mt::twist() noexcept {
    for (size_t i = 0; i < n; ++i) {
        const seed_type y = (state_[i] & 0x80000000) + (state_[(i + 1) % n] & 0x7fffffff);
        state_[i] = state_[(i + m) % n] ^ (y >> 1);
        if (y % 2 != 0) {
            state_[i] ^= a;
        }
    }
    index_ = 0;
}

random_mt::seed_type random_mt::generate_32bit() noexcept {
    if (index_ >= n) {
        twist();
    }
    seed_type y = state_[index_++];
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);
    return y;
}

uint64_t random_mt::generate_64bit() noexcept {
    const auto hi = static_cast<uint64_t>(generate_32bit()) << 32;
    const auto lo = static_cast<uint64_t>(generate_32bit());
    return hi | lo;
}

random_mt::random_mt() noexcept { set_seed(static_cast<seed_type>(timestamp::now().value())); }

void random_mt::set_seed(const seed_type seed) noexcept {
    state_[0] = seed;
    for (size_t i = 1; i < n; ++i) {
        state_[i] = static_cast<seed_type>(1812433253ULL * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i);
    }
    index_ = n;
}

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
