#ifndef MSTL_RANDOM_HPP__
#define MSTL_RANDOM_HPP__
#include "environment.hpp"
MSTL_BEGIN_NAMESPACE__

// based on LCD algorithm to generate pseudorandom number
class MSTL_API random_lcd {
public:
    using seed_type = uint32_t;

private:
    static constexpr seed_type a = 1103515245;
    static constexpr seed_type c = 12345;
    static constexpr seed_type m = 1u << 31;

    static seed_type& get_seed();

public:
    static void set_seed(seed_type seed = 0);

    static int next_int(int max);
    static int next_int(int min, int max);
    static int next_int();

    static double next_double();
    static double next_double(double min, double max);
    static double next_double(double max);
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

    static seed_type* state();
    static size_t& index();
    static void twist();

    static seed_type* get_state();
    static size_t& get_index();

public:
    static void set_seed(seed_type seed = 0);

    static int next_int(int max);
    static int next_int(int min, int max);
    static int next_int();

    static double next_double();
    static double next_double(double min, double max);
    static double next_double(double max);
};


// based on hardware noise to generate true random number
class MSTL_API secret {
public:
    static int32_t next_int(int32_t min, int32_t max);
    static int32_t next_int(int32_t max);
    static double next_double();

    static bool is_supported();

private:
    static void get_random_bytes(byte_t* buffer, size_t length);
};

MSTL_END_NAMESPACE__
#endif // MSTL_RANDOM_HPP__
