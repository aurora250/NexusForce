#include <NeForce/core/memory/memory_pool.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/string/string.hpp>
#include <benchmark/benchmark.h>
using namespace neforce;

namespace {
    memory_pool& pool() {
        static memory_pool instance;
        return instance;
    }

    void touch(void* block) noexcept { benchmark::DoNotOptimize(*static_cast<unsigned char*>(block) = 0x5A); }

    struct glibc_backend {
        static void* allocate(const size_t size) { return malloc(size); }
        static void release(void* block) { free(block); }
    };

    struct operator_new_backend {
        static void* allocate(const size_t size) { return ::operator new(size); }
        static void release(void* block) { ::operator delete(block); }
    };

    struct pool_backend {
        static void* allocate(const size_t size) { return pool().allocate(size); }
        static void release(void* block) { pool().deallocate(block); }
    };

    template <typename Backend>
    void pair_loop(benchmark::State& state, const size_t size) {
        for (auto _: state) {
            void* block = Backend::allocate(size);
            if (block == nullptr) {
                state.SkipWithError("allocation failed");
                return;
            }
            touch(block);
            Backend::release(block);
        }
        state.SetItemsProcessed(state.iterations());
    }

    template <typename Backend>
    void batch_loop(benchmark::State& state, const size_t size, const size_t batch) {
        vector<void*> blocks(batch, nullptr);
        for (auto _: state) {
            for (size_t index = 0; index < batch; ++index) {
                blocks[index] = Backend::allocate(size);
                touch(blocks[index]);
            }
            for (size_t index = 0; index < batch; ++index) {
                Backend::release(blocks[index]);
            }
            state.SetItemsProcessed(static_cast<int64_t>(batch) * 2);
            state.SetBytesProcessed(static_cast<int64_t>(batch) * static_cast<int64_t>(size));
        }
    }

    template <typename Backend>
    void threaded_loop(benchmark::State& state, const size_t size) {
        constexpr size_t rounds = 20000;
        for (auto _: state) {
            atomic<int64_t> operations{0};
            vector<thread> workers;
            const size_t count = static_cast<size_t>(state.threads());
            workers.reserve(count);
            for (size_t index = 0; index < count; ++index) {
                workers.emplace_back([&operations, size]() {
                    int64_t local = 0;
                    for (size_t round = 0; round < rounds; ++round) {
                        void* block = Backend::allocate(size);
                        touch(block);
                        Backend::release(block);
                        ++local;
                    }
                    operations.fetch_add(local * 2, memory_order_relaxed);
                });
            }
            for (auto& worker: workers) {
                worker.join();
            }
            state.SetItemsProcessed(operations.load());
            state.SetBytesProcessed(operations.load() / 2 * static_cast<int64_t>(size));
        }
    }

    template <typename Backend>
    void container_loop(benchmark::State& state) {
        int64_t operations = 0;
        for (auto _: state) {
            auto* lines = new vector<string>();
            for (int index = 0; index < 2000; ++index) {
                lines->emplace_back(static_cast<size_t>(index % 97) + 1, static_cast<char>('a' + (index % 26)));
            }
            operations += static_cast<int64_t>(lines->size());
            benchmark::DoNotOptimize(lines->data());
            delete lines;
        }
        state.SetItemsProcessed(operations);
    }
} // namespace

BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 16)->Name("Pair/Glibc/16");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 16)->Name("Pair/OperatorNew/16");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 16)->Name("Pair/Pool/16");
BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 64)->Name("Pair/Glibc/64");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 64)->Name("Pair/OperatorNew/64");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 64)->Name("Pair/Pool/64");
BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 256)->Name("Pair/Glibc/256");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 256)->Name("Pair/OperatorNew/256");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 256)->Name("Pair/Pool/256");
BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 1024)->Name("Pair/Glibc/1024");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 1024)->Name("Pair/OperatorNew/1024");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 1024)->Name("Pair/Pool/1024");
BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 4096)->Name("Pair/Glibc/4096");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 4096)->Name("Pair/OperatorNew/4096");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 4096)->Name("Pair/Pool/4096");
BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 16384)->Name("Pair/Glibc/16384");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 16384)->Name("Pair/OperatorNew/16384");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 16384)->Name("Pair/Pool/16384");
BENCHMARK_CAPTURE(pair_loop<glibc_backend>, glibc_backend, 262144)->Name("Pair/Glibc/262144");
BENCHMARK_CAPTURE(pair_loop<operator_new_backend>, operator_new_backend, 262144)->Name("Pair/OperatorNew/262144");
BENCHMARK_CAPTURE(pair_loop<pool_backend>, pool_backend, 262144)->Name("Pair/Pool/262144");

BENCHMARK_CAPTURE(batch_loop<glibc_backend>, glibc_backend, 64, 512)->Name("Batch/Glibc/64x512");
BENCHMARK_CAPTURE(batch_loop<operator_new_backend>, operator_new_backend, 64, 512)->Name("Batch/OperatorNew/64x512");
BENCHMARK_CAPTURE(batch_loop<pool_backend>, pool_backend, 64, 512)->Name("Batch/Pool/64x512");
BENCHMARK_CAPTURE(batch_loop<glibc_backend>, glibc_backend, 4096, 128)->Name("Batch/Glibc/4096x128");
BENCHMARK_CAPTURE(batch_loop<operator_new_backend>, operator_new_backend, 4096, 128)
        ->Name("Batch/OperatorNew/4096x128");
BENCHMARK_CAPTURE(batch_loop<pool_backend>, pool_backend, 4096, 128)->Name("Batch/Pool/4096x128");

BENCHMARK_CAPTURE(threaded_loop<glibc_backend>, Glibc, 64)
        ->Name("Threaded/Glibc/64")
        ->ThreadRange(1, 24)
        ->UseRealTime();
BENCHMARK_CAPTURE(threaded_loop<operator_new_backend>, OperatorNew, 64)
        ->Name("Threaded/OperatorNew/64")
        ->ThreadRange(1, 24)
        ->UseRealTime();
BENCHMARK_CAPTURE(threaded_loop<pool_backend>, Pool, 64)->Name("Threaded/Pool/64")->ThreadRange(1, 24)->UseRealTime();
BENCHMARK_CAPTURE(threaded_loop<glibc_backend>, Glibc, 4096)
        ->Name("Threaded/Glibc/4096")
        ->ThreadRange(1, 24)
        ->UseRealTime();
BENCHMARK_CAPTURE(threaded_loop<operator_new_backend>, OperatorNew, 4096)
        ->Name("Threaded/OperatorNew/4096")
        ->ThreadRange(1, 24)
        ->UseRealTime();
BENCHMARK_CAPTURE(threaded_loop<pool_backend>, Pool, 4096)
        ->Name("Threaded/Pool/4096")
        ->ThreadRange(1, 24)
        ->UseRealTime();

BENCHMARK(container_loop<operator_new_backend>)->Name("Container/OperatorNew");
BENCHMARK(container_loop<pool_backend>)->Name("Container/Pool");

BENCHMARK_MAIN();
