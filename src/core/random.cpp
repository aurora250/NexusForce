#include <MSTL/core/random.hpp>
#include <MSTL/core/datetime.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include <wincrypt.h>
#include <MSTL/core/undef_cmacro.hpp>
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/fcntl.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

void random_lcd::set_seed(const seed_type seed) {
    if (seed == 0) {
        get_seed() = static_cast<seed_type>(_MSTL timestamp::now().seconds());
    } else {
        get_seed() = seed;
    }
}

int random_lcd::next_int() {
    return next_int(0, numeric_limits<int32_t>::max());
}

void random_mt::twist() {
    for (size_t i = 0; i < n; ++i) {
        const seed_type y = (state()[i] & 0x80000000) + (state()[(i + 1) % n] & 0x7fffffff);
        state()[i] = state()[(i + m) % n] ^ (y >> 1);
        if (y % 2 != 0) {
            state()[i] ^= a;
        }
    }
    index() = 0;
}

random_mt::seed_type* random_mt::get_state() {
    static bool initialized = false;
    if (!initialized) {
        set_seed();
        initialized = true;
    }
    return state();
}

void random_mt::set_seed(const seed_type seed) {
    seed_type init_seed = seed;
    if (init_seed == 0) {
        init_seed = static_cast<seed_type>(_MSTL timestamp::now().seconds());
    }

    state()[0] = init_seed;
    for (size_t i = 1; i < n; ++i) {
        state()[i] = 1812433253 * (state()[i - 1] ^ (state()[i - 1] >> 30)) + i;
    }
    index() = n;
}

int random_mt::next_int(const int max) {
    if (max <= 0) return 0;

    const seed_type* state = get_state();
    size_t& idx = index();

    if (idx >= n) {
        twist();
    }

    seed_type y = state[idx++];
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);

    return static_cast<int>(static_cast<double>(y) / numeric_limits<uint32_t>::max() * max);
}

int random_mt::next_int() {
    return next_int(0, numeric_limits<int32_t>::max());
}

double random_mt::next_double() {
    const seed_type* state = get_state();
    size_t& idx = index();
    if (idx >= n) twist();

    seed_type y = state[idx++];
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);

    return static_cast<double>(y) / numeric_limits<uint32_t>::max();
}

int32_t secret::next_int(const int32_t min, const int32_t max) {
    if(min >= max) Exception(ValueError("min is ge to max"));
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
    if(buffer == nullptr || length == 0) Exception(ValueError("Invalid buffer or length"));

#ifdef MSTL_PLATFORM_WINDOWS__
    HCRYPTPROV hProv = 0;
    if (!::CryptAcquireContext(&hProv, nullptr, nullptr,
        PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        Exception(DeviceOperateError("Failed to acquire crypto context"));
        }

    if (!::CryptGenRandom(hProv, static_cast<DWORD>(length), reinterpret_cast<BYTE*>(buffer))) {
        ::CryptReleaseContext(hProv, 0);
        Exception(DeviceOperateError("Failed to generate random bytes"));
    }

    ::CryptReleaseContext(hProv, 0);
#elif defined(MSTL_PLATFORM_LINUX__)
    const int fd = open("/dev/urandom", O_RDONLY);
    if(fd == -1) Exception(FileOperateError("Failed to open /dev/urandom"));

    ssize_t bytesRead = 0;
    while (bytesRead < static_cast<ssize_t>(length)) {
        const ssize_t result = ::read(fd, buffer + bytesRead, length - bytesRead);
        if (result == -1) {
            ::close(fd);
            if(fd == -1) Exception(FileOperateError("Failed to open /dev/urandom"));
        }
        bytesRead += result;
    }
    ::close(fd);
#endif
}

MSTL_END_NAMESPACE__
