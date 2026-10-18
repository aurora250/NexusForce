#include <NeForce/core/numeric/random/pcg.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr random_pcg32::seed_type default_stream = 0xda3e39cb94b95bdbULL;
    constexpr random_pcg64::seed_type default_increment_hi = 0x5851f42d4c957f2dULL;
    constexpr random_pcg64::seed_type default_increment_lo = 0x14057b7ef767814fULL;
} // namespace


random_pcg32::random_pcg32() noexcept :
random_pcg32(random_seed()) {}

random_pcg32::random_pcg32(const seed_type seed) noexcept :
random_pcg32(seed, default_stream) {}

random_pcg32::random_pcg32(const seed_type seed, const seed_type stream) noexcept { set_seed(seed, stream); }

void random_pcg32::set_seed(const seed_type seed) noexcept { set_seed(seed, default_stream); }

void random_pcg32::set_seed(const seed_type seed, const seed_type stream) noexcept {
    state_ = 0;
    increment_ = (stream << 1) | 1;

    generate_word();
    state_ += seed;
    generate_word();
}

random_pcg64::random_pcg64() noexcept :
random_pcg64(random_seed()) {}

random_pcg64::random_pcg64(const seed_type seed) noexcept :
random_pcg64(seed, 0) {}

random_pcg64::random_pcg64(const seed_type seed, const seed_type stream) noexcept { set_seed(seed, stream); }

void random_pcg64::set_seed(const seed_type seed) noexcept { set_seed(seed, 0); }

void random_pcg64::set_seed(const seed_type seed, const seed_type stream) noexcept {
    state_hi_ = 0;
    state_lo_ = 0;
    increment_hi_ = default_increment_hi;
    increment_lo_ = default_increment_lo;

    const seed_type offset = stream << 1;
    increment_lo_ += offset;
    if (increment_lo_ < offset) {
        ++increment_hi_;
    }

    step();
    state_lo_ += seed;
    if (state_lo_ < seed) {
        ++state_hi_;
    }
    step();
}

NEFORCE_END_NAMESPACE__
