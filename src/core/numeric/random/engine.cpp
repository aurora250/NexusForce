#include <NeForce/core/numeric/random/engine.hpp>
#include <NeForce/core/time/datetime.hpp>
NEFORCE_BEGIN_NAMESPACE__

uint64_t random_seed() noexcept {
    // Mix the timestamp with a thread-local sequence and the address entropy so that generators
    // created within the same tick still receive distinct seeds.
    thread_local uint64_t sequence = 0;
    ++sequence;

    uint64_t state = static_cast<uint64_t>(timestamp::now().value());
    state ^= sequence * 0x9e3779b97f4a7c15ULL;
    state ^= static_cast<uint64_t>(reinterpret_cast<size_t>(&sequence)) * 0xbf58476d1ce4e5b9ULL;
    return splitmix64(state);
}

NEFORCE_END_NAMESPACE__
