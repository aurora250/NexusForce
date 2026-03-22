#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/time/datetime.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/config/windef.hpp>
#include <minwindef.h>
#include <minwinbase.h>
#include <windef.h>
#include <wincrypt.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <sys/fcntl.h>
#include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

random_lcd::random_lcd() noexcept
: seed_(timestamp::now().value()) {}

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
    uint64_t result = 0;
    result = static_cast<uint64_t>(generate_32bit()) << 32;
    result |= generate_32bit();
    return result;
}

random_mt::random_mt() noexcept {
    set_seed(static_cast<seed_type>(timestamp::now()));
}

void random_mt::set_seed(const seed_type seed) noexcept {
    state_[0] = seed;
    for (size_t i = 1; i < n; ++i) {
        state_[i] = 1812433253 * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i;
    }
    index_ = n;
}

bool secret::system_supported() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::HCRYPTPROV hProv;
    const bool supported = ::CryptAcquireContext(
        &hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (supported) {
        ::CryptReleaseContext(hProv, 0);
    }
    return supported;
#elif defined(NEFORCE_PLATFORM_LINUX)
    const int fd = ::open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return false;
    }
    ::close(fd);
    return true;
#else
    return false;
#endif
}

void secret::get_random_bytes(byte_t* buffer, size_t length) {
    if(buffer == nullptr || length == 0) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid buffer or length"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    HCRYPTPROV hProv = 0;
    if (!::CryptAcquireContext(&hProv, nullptr, nullptr,
        PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        NEFORCE_THROW_EXCEPTION(device_exception("Failed to acquire crypto context"));
    }

    if (!::CryptGenRandom(hProv, static_cast<::DWORD>(length), reinterpret_cast<::BYTE*>(buffer))) {
        ::CryptReleaseContext(hProv, 0);
        NEFORCE_THROW_EXCEPTION(device_exception("Failed to generate random bytes"));
    }

    ::CryptReleaseContext(hProv, 0);
#elif defined(NEFORCE_PLATFORM_LINUX)
    const int fd = ::open("/dev/urandom", O_RDONLY);
    if(fd == -1) {
        NEFORCE_THROW_EXCEPTION(file_exception("Failed to open /dev/urandom"));
    }

    ssize_t bytes_read = 0;
    while (bytes_read < static_cast<ssize_t>(length)) {
        const ssize_t result = ::read(fd, buffer + bytes_read, length - bytes_read);
        if (result == -1) {
            ::close(fd);
            if(fd == -1) {
                NEFORCE_THROW_EXCEPTION(file_exception("Failed to open /dev/urandom"));
            }
        }
        bytes_read += result;
    }
    ::close(fd);
#endif
}

NEFORCE_END_NAMESPACE__
