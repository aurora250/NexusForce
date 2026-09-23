#include <NeForce/core/numeric/random/mt.hpp>
#include <NeForce/core/time/datetime.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    using seed_type = random_mt::seed_type;

    constexpr size_t m = 397;           ///< 中间偏移量
    constexpr seed_type a = 0x9908b0df; ///< 旋转矩阵常数
    constexpr seed_type u = 11;         ///< 位掩码1
    constexpr seed_type s = 7;          ///< 位移量1
    constexpr seed_type b = 0x9d2c5680; ///< 位掩码2
    constexpr seed_type t = 15;         ///< 位移量2
    constexpr seed_type c = 0xefc60000; ///< 位掩码3
    constexpr seed_type l = 18;         ///< 位移量3
} // namespace

random_mt::result_type random_mt::generate_word() noexcept {
    if (index_ >= n) {
        for (size_t i = 0; i < n; ++i) {
            const seed_type y = (state_[i] & 0x80000000) + (state_[(i + 1) % n] & 0x7fffffff);
            state_[i] = state_[(i + m) % n] ^ (y >> 1);
            if (y % 2 != 0) {
                state_[i] ^= a;
            }
        }
        index_ = 0;
    }

    seed_type y = state_[index_++];
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);
    return y;
}

random_mt::random_mt() noexcept { set_seed(static_cast<seed_type>(timestamp::now().value())); }

void random_mt::set_seed(const seed_type seed) noexcept {
    state_[0] = seed;
    for (size_t i = 1; i < n; ++i) {
        state_[i] = static_cast<seed_type>(1812433253ULL * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i);
    }
    index_ = n;
}

NEFORCE_END_NAMESPACE__
