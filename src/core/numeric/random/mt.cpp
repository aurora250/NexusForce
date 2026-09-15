#include <NeForce/core/numeric/random/mt.hpp>
#include <NeForce/core/time/datetime.hpp>
NEFORCE_BEGIN_NAMESPACE__

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

random_mt::result_type random_mt::generate_word() noexcept {
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

random_mt::random_mt() noexcept { set_seed(static_cast<seed_type>(timestamp::now().value())); }

void random_mt::set_seed(const seed_type seed) noexcept {
    state_[0] = seed;
    for (size_t i = 1; i < n; ++i) {
        state_[i] = static_cast<seed_type>(1812433253ULL * (state_[i - 1] ^ (state_[i - 1] >> 30)) + i);
    }
    index_ = n;
}

NEFORCE_END_NAMESPACE__
