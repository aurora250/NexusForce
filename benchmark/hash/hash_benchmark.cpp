#include <benchmark/benchmark.h>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/encoding/base32.hpp>
#include <NeForce/core/hash/blake2.hpp>
#include <NeForce/core/hash/blake3.hpp>
#include <NeForce/core/hash/sha256.hpp>
#include <NeForce/core/hash/sha512.hpp>

using namespace neforce;

namespace {
    vector<byte_t> make_data(const size_t size) {
        vector<byte_t> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<byte_t>((i * 131U + 17U) & 0xFFU);
        }
        return data;
    }

    template <typename Function>
    void run_hash(benchmark::State& state, Function&& function) {
        const vector<byte_t> data = make_data(static_cast<size_t>(state.range(0)));
        for (auto _: state) {
            const byte_vector digest = function(data.view());
            benchmark::DoNotOptimize(digest);
        }
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * state.range(0));
    }
} // namespace

static void bm_sha256(benchmark::State& state) {
    run_hash(state, [](const cbyte_view data) { return SHA256::hash(data); });
}

static void bm_sha512(benchmark::State& state) {
    run_hash(state, [](const cbyte_view data) { return SHA512::hash(data); });
}

static void bm_blake2b(benchmark::State& state) {
    run_hash(state, [](const cbyte_view data) { return BLAKE2b::hash(data); });
}

static void bm_blake2s(benchmark::State& state) {
    run_hash(state, [](const cbyte_view data) { return BLAKE2s::hash(data); });
}

static void bm_blake3(benchmark::State& state) {
    run_hash(state, [](const cbyte_view data) { return BLAKE3::hash(data); });
}

static void bm_base32_encode(benchmark::State& state) {
    const vector<byte_t> data = make_data(static_cast<size_t>(state.range(0)));
    for (auto _: state) {
        const string encoded = base32::encode(data.view());
        benchmark::DoNotOptimize(encoded);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * state.range(0));
}

static void bm_base32_decode(benchmark::State& state) {
    const vector<byte_t> data = make_data(static_cast<size_t>(state.range(0)));
    const string encoded = base32::encode(data.view());
    for (auto _: state) {
        const byte_vector decoded = base32::decode(encoded.view());
        benchmark::DoNotOptimize(decoded);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * state.range(0));
}

BENCHMARK(bm_sha256)->RangeMultiplier(4)->Range(64, 1 << 20);
BENCHMARK(bm_sha512)->RangeMultiplier(4)->Range(64, 1 << 20);
BENCHMARK(bm_blake2b)->RangeMultiplier(4)->Range(64, 1 << 20);
BENCHMARK(bm_blake2s)->RangeMultiplier(4)->Range(64, 1 << 20);
BENCHMARK(bm_blake3)->RangeMultiplier(4)->Range(64, 1 << 20);
BENCHMARK(bm_base32_encode)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK(bm_base32_decode)->RangeMultiplier(4)->Range(64, 1 << 16);
BENCHMARK_MAIN();
