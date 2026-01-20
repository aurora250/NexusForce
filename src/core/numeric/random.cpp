#include <MSTL/core/numeric/random.hpp>
#include <MSTL/core/time/datetime.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include <wincrypt.h>
#include <MSTL/core/config/undef_cmacro.hpp>
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/fcntl.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

random_lcd::random_lcd()
: seed_(static_cast<seed_type>(_MSTL timestamp::now())) {}

void random_mt::twist() {
    for (size_t i = 0; i < n; ++i) {
        const seed_type y = (state_[i] & 0x80000000) + (state_[(i + 1) % n] & 0x7fffffff);
        state_[i] = state_[(i + m) % n] ^ (y >> 1);
        if (y % 2 != 0) {
            state_[i] ^= a;
        }
    }
    index_ = 0;
}

random_mt::random_mt() {
    set_seed(static_cast<seed_type>(_MSTL timestamp::now()));
}

void random_mt::set_seed(const seed_type seed) {
    state_[0] = seed;
    for (size_t i = 1; i < n; ++i) {
        state_[i] = 1812433253 * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i;
    }
    index_ = n;
}

int random_mt::next_int(const int max) {
    if (max <= 0) return 0;

    if (index_ >= n) {
        twist();
    }

    seed_type y = state_[index_++];
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);

    if (max == 1) return 0;

    const uint64_t product = static_cast<uint64_t>(y) * static_cast<uint64_t>(max);
    return static_cast<int>(product >> 32);
}

double random_mt::next_double() {
    if (index_ >= n) {
        twist();
    }

    seed_type y = state_[index_++];
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);

    return static_cast<double>(y) / numeric_traits<uint32_t>::max();
}

int32_t secret::next_int(const int32_t min, const int32_t max) {
    if(min >= max) throw_exception(value_exception("min is ge to max"));
    const auto range = static_cast<uint32_t>(max - min + 1);

    int bits = 0;
    uint32_t temp = range - 1;
    while (temp > 0) {
        temp >>= 1;
        bits++;
    }

    uint32_t value;
    do {
        get_random_bytes(reinterpret_cast<byte_t*>(&value), sizeof(value));
        value &= (1ULL << bits) - 1;
    } while (value >= range);

    return min + static_cast<int32_t>(value);
}

double secret::next_double() {
    uint64_t value;
    get_random_bytes(reinterpret_cast<byte_t*>(&value), sizeof(value));
    return static_cast<double>(value) / (static_cast<double>(1ULL << 63) + 1.0);
}

bool secret::is_supported() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::HCRYPTPROV hProv;
    const bool supported = ::CryptAcquireContext(
        &hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (supported) {
        ::CryptReleaseContext(hProv, 0);
    }
    return supported;
#elif defined(MSTL_PLATFORM_LINUX__)
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
        throw_exception(value_exception("Invalid buffer or length"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    HCRYPTPROV hProv = 0;
    if (!::CryptAcquireContext(&hProv, nullptr, nullptr,
        PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        throw_exception(device_exception("Failed to acquire crypto context"));
    }

    if (!::CryptGenRandom(hProv, static_cast<DWORD>(length), reinterpret_cast<BYTE*>(buffer))) {
        ::CryptReleaseContext(hProv, 0);
        throw_exception(device_exception("Failed to generate random bytes"));
    }

    ::CryptReleaseContext(hProv, 0);
#elif defined(MSTL_PLATFORM_LINUX__)
    const int fd = ::open("/dev/urandom", O_RDONLY);
    if(fd == -1) {
        throw_exception(file_exception("Failed to open /dev/urandom"));
    }

    ssize_t bytes_read = 0;
    while (bytes_read < static_cast<ssize_t>(length)) {
        const ssize_t result = ::read(fd, buffer + bytes_read, length - bytes_read);
        if (result == -1) {
            ::close(fd);
            if(fd == -1) {
                throw_exception(file_exception("Failed to open /dev/urandom"));
            }
        }
        bytes_read += result;
    }
    ::close(fd);
#endif
}

MSTL_END_NAMESPACE__
