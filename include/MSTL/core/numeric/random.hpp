#ifndef MSTL_RANDOM_HPP__
#define MSTL_RANDOM_HPP__
#include "../config/types.hpp"
MSTL_BEGIN_NAMESPACE__

// based on LCD algorithm to generate pseudorandom number
class MSTL_API random_lcd {
public:
    using seed_type = uint32_t;

private:
    static constexpr seed_type a = 1103515245;
    static constexpr seed_type c = 12345;
    static constexpr seed_type m = 1u << 31;

    static seed_type& get_seed() {
        static seed_type seed = 0;
        return seed;
    }

public:
    static void set_seed(seed_type seed = 0);

    static int next_int(const int max) {
        get_seed() = a * get_seed() + c;
        get_seed() %= m;
        return static_cast<int>(static_cast<double>(get_seed()) / m * max);
    }
    static int next_int(const int min, const int max) { return min + next_int(max - min); }
    static int next_int();

    static double next_double() {
        get_seed() = a * get_seed() + c;
        get_seed() %= m;
        return static_cast<double>(get_seed()) / m;
    }
    static double next_double(const double min, const double max) {
        return min + (max - min) * next_double();
    }
    static double next_double(const double max)  {
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

    static seed_type* state() {
        static seed_type state[n] = {};
        return state;
    }
    static size_t& index() {
        static size_t index = n;
        return index;
    }
    static void twist();

    static seed_type* get_state();

public:
    static void set_seed(seed_type seed = 0);

    static int next_int(const int min, const int max) {
        if (min >= max) return min;
        return min + next_int(max - min);
    }
    static int next_int(int max);
    static int next_int();

    static double next_double(const double max) { return next_double(0.0, max); }
    static double next_double(const double min, const double max) {
        if (min >= max) return min;
        return min + (max - min) * next_double();
    }
    static double next_double();
};


// based on hardware noise to generate true random number
class MSTL_API secret {
public:
    static int32_t next_int(int32_t min, int32_t max);
    static int32_t next_int(int32_t max) { return next_int(0, max); }
    static double next_double();

    static bool is_supported();

private:
    static void get_random_bytes(byte_t* buffer, size_t length);
};

MSTL_END_NAMESPACE__
#endif // MSTL_RANDOM_HPP__
