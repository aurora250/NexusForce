#ifndef MSTL_CORE_NUMERIC_RANDOM_HPP__
#define MSTL_CORE_NUMERIC_RANDOM_HPP__
#include "../numeric/numeric_limits.hpp"
MSTL_BEGIN_NAMESPACE__

// based on LCD algorithm to generate pseudorandom number
class MSTL_API random_lcd {
public:
    using seed_type = uint32_t;

private:
    static constexpr seed_type a = 1103515245;
    static constexpr seed_type c = 12345;
    static constexpr seed_type m = 1u << 31;

    seed_type seed_;

public:
    random_lcd();
    explicit random_lcd(const seed_type seed) : seed_(seed) {}

    random_lcd(const random_lcd&) = default;
    random_lcd& operator =(const random_lcd&) = default;
    random_lcd(random_lcd&&) = default;
    random_lcd& operator =(random_lcd&&) = default;

    int next_int(const int max) {
        seed_ = a * seed_ + c;
        seed_ %= m;
        return static_cast<int>(static_cast<double>(seed_) / m * max);
    }

    int next_int(const int min, const int max) {
        return min + next_int(max - min);
    }

    int next_int() {
        return next_int(0, numeric_limits<int32_t>::max());
    }

    double next_double() {
        seed_ = a * seed_ + c;
        seed_ %= m;
        return static_cast<double>(seed_) / m;
    }

    double next_double(const double min, const double max) {
        return min + (max - min) * next_double();
    }

    double next_double(const double max)  {
        return next_double(0, max);
    }
};


// based on Mersenne Twister algorithm to generate pseudorandom number
class MSTL_API random_mt {
public:
    using seed_type = uint32_t;

private:
    static constexpr size_t n = 624;
    static constexpr size_t m = 397;
    static constexpr seed_type a = 0x9908b0df;
    static constexpr seed_type u = 11;
    static constexpr seed_type s = 7;
    static constexpr seed_type b = 0x9d2c5680;
    static constexpr seed_type t = 15;
    static constexpr seed_type c = 0xefc60000;
    static constexpr seed_type l = 18;

    seed_type state_[n] = {};
    size_t index_ = n;

    void twist();

public:
    random_mt();
    explicit random_mt(const seed_type seed) { set_seed(seed); }

    random_mt(const random_mt& other) = default;
    random_mt& operator =(const random_mt& other) = default;
    random_mt(random_mt&& other) noexcept = default;
    random_mt& operator =(random_mt&& other) noexcept = default;

    void set_seed(seed_type seed);

    int next_int(int max);

    int next_int(const int min, const int max) {
        if (min >= max) return min;
        return min + next_int(max - min);
    }

    int next_int() {
        return next_int(0, numeric_limits<int32_t>::max());
    }

    double next_double();

    double next_double(const double min, const double max) {
        if (min >= max) return min;
        return min + (max - min) * next_double();
    }

    double next_double(const double max) {
        return next_double(0.0, max);
    }
};


// based on hardware noise to generate true random number
class MSTL_API secret {
public:
    static int32_t next_int(int32_t min, int32_t max);
    static int32_t next_int(const int32_t max) { return next_int(0, max); }
    static double next_double();

    static bool is_supported();

private:
    static void get_random_bytes(byte_t* buffer, size_t length);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_NUMERIC_RANDOM_HPP__
