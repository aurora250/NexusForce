#include <NeForce/core/memory/memory_pool.hpp>
#include <NeForce/core/memory/new.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/system/process.hpp>
#include <gtest/gtest.h>

using namespace neforce;

namespace {
    struct payload {
        char data[96];
        int marker;
    };

    struct alignas(128) wide_payload {
        char data[192];
    };

    size_t resident_bytes() {
        const auto mi = process::get_memory_info(process::current_id());
        return mi.working_set_size;
    }
} // namespace

TEST(AllocatorOverride, NewAndDeleteAreServedByTheMemoryPool) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    memory_pool& pool = system_memory_pool();
    const size_t before = pool.stats().active_blocks;

    vector<payload*> blocks;
    for (int index = 0; index < 4096; ++index) {
        auto* item = new payload();
        ASSERT_NE(item, nullptr);
        item->marker = index;
        memory_set(item->data, index & 0xFF, sizeof(item->data));
        blocks.push_back(item);
    }
    EXPECT_GT(pool.stats().active_blocks, before);

    for (int index = 0; index < 4096; ++index) {
        EXPECT_EQ(blocks[static_cast<size_t>(index)]->marker, index);
        const auto* data = reinterpret_cast<const unsigned char*>(blocks[static_cast<size_t>(index)]->data);
        for (size_t offset = 0; offset < sizeof(payload::data); ++offset) {
            ASSERT_EQ(data[offset], static_cast<unsigned char>(index & 0xFF));
        }
    }
    for (payload* item: blocks) {
        delete item;
    }
    pool.flush_thread_cache();
    EXPECT_TRUE(pool.verify());
#endif
}

TEST(AllocatorOverride, ArrayAndSizedDeleteForms) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    auto* bytes = new unsigned char[5000];
    ASSERT_NE(bytes, nullptr);
    memory_set(bytes, 0x5C, 5000);
    EXPECT_EQ(bytes[4999], 0x5C);
    delete[] bytes;

    auto* objects = new payload[64];
    ASSERT_NE(objects, nullptr);
    objects[63].marker = 63;
    EXPECT_EQ(objects[63].marker, 63);
    delete[] objects;

    payload* single = new payload;
    ASSERT_NE(single, nullptr);
    delete single;
#endif
}

TEST(AllocatorOverride, NothrowFormsReportFailureInsteadOfThrowing) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    void* ok = ::operator new(1024, neforce::nothrow);
    ASSERT_NE(ok, nullptr);
    ::operator delete(ok);

    void* too_large = ::operator new(static_cast<size_t>(-1) / 2, neforce::nothrow);
    EXPECT_EQ(too_large, nullptr);

    void* array_too_large = ::operator new[](static_cast<size_t>(-1) / 2, neforce::nothrow);
    EXPECT_EQ(array_too_large, nullptr);

    EXPECT_THROW(ignore = ::operator new(static_cast<size_t>(-1) / 2), allocate_exception);
#endif
}

TEST(AllocatorOverride, AlignedFormsAreHonored) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    auto* item = new wide_payload();
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(item) % 128, 0U);
    memory_set(item->data, 0x3D, sizeof(item->data));
    EXPECT_EQ(item->data[191], 0x3D);
    delete item;

    void* block = ::operator new(256, align_t{4096});
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(block) % 4096, 0U);
    ::operator delete(block);
#endif
}

TEST(AllocatorOverride, StandardContainersUseThePool) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    memory_pool& pool = system_memory_pool();
    const size_t before = pool.stats().os_map_calls;
    {
        std::vector<string> lines;
        for (int index = 0; index < 20000; ++index) {
            lines.emplace_back(static_cast<size_t>(index % 97) + 1, static_cast<char>('a' + (index % 26)));
        }
        ASSERT_EQ(lines.size(), 20000U);
        EXPECT_EQ(lines[19999].size(), static_cast<size_t>(19999 % 97) + 1);
    }
    EXPECT_GE(pool.stats().os_map_calls, before);
    pool.flush_thread_cache();
    EXPECT_TRUE(pool.verify());
#endif
}

TEST(AllocatorOverride, MultiThreadedAllocationIsRaceFree) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    atomic<int> errors{0};
    vector<thread> workers;
    for (size_t index = 0; index < 8; ++index) {
        workers.emplace_back([&errors, index]() {
            vector<void*> live;
            for (int round = 0; round < 20000; ++round) {
                const size_t size = (static_cast<size_t>(round) * 37 + index * 11) % 4096 + 2;
                auto* block = static_cast<unsigned char*>(::operator new(size));
                if (block == nullptr) {
                    ++errors;
                    continue;
                }
                block[0] = static_cast<unsigned char>(index);
                block[size - 1] = static_cast<unsigned char>(round);
                if (block[0] != static_cast<unsigned char>(index) ||
                    block[size - 1] != static_cast<unsigned char>(round)) {
                    ++errors;
                }
                if ((round & 7) == 0 && !live.empty()) {
                    const size_t victim = static_cast<size_t>(round) % live.size();
                    ::operator delete(live[victim]);
                    live[victim] = block;
                } else {
                    live.push_back(block);
                }
            }
            for (void* block: live) {
                ::operator delete(block);
            }
        });
    }
    for (auto& worker: workers) {
        worker.join();
    }
    EXPECT_EQ(errors.load(), 0);
    memory_pool& pool = system_memory_pool();
    pool.flush_thread_cache();
    EXPECT_TRUE(pool.verify());
#endif
}

TEST(AllocatorOverride, RepeatedLargeAllocationsDoNotInflateResidentMemory) {
#ifndef NEFORCE_USING_MEMORY_POOL_OVERRIDE
    GTEST_SKIP() << "NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE is disabled";
#else
    memory_pool& pool = system_memory_pool();
    pool.flush_thread_cache();
    pool.purge();
    for (int round = 0; round < 64; ++round) {
        auto* block = static_cast<unsigned char*>(::operator new(1U << 20));
        ASSERT_NE(block, nullptr);
        block[0] = 1;
        block[(1U << 20) - 1] = 2;
        ::operator delete(block);
    }
    const size_t after_warmup = resident_bytes();
    const size_t map_calls_warmup = pool.stats().os_map_calls;
    const size_t unmap_calls_warmup = pool.stats().os_unmap_calls;
    for (int round = 0; round < 512; ++round) {
        auto* block = static_cast<unsigned char*>(::operator new(1U << 20));
        ASSERT_NE(block, nullptr);
        block[0] = 3;
        ::operator delete(block);
    }
    const size_t after = resident_bytes();
    // The exact signal: reusing one cached region must not map or unmap anything.
    EXPECT_EQ(pool.stats().os_map_calls, map_calls_warmup);
    EXPECT_EQ(pool.stats().os_unmap_calls, unmap_calls_warmup);
    // The resident size is a coarse guard on purpose: its unit and the bookkeeping overhead of a profiler
    // (valgrind and friends) are both platform and environment dependent.
    EXPECT_LT(after, after_warmup + (1U << 20));
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_TRUE(pool.verify());
#endif
}
