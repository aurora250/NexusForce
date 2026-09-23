#include <NeForce/core/memory/memory_pool.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/map.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/system/process.hpp>
#include <gtest/gtest.h>

using namespace neforce;

namespace {
    void fill_pattern(void* block, const size_t bytes, const unsigned char seed) {
        auto* data = static_cast<unsigned char*>(block);
        for (size_t index = 0; index < bytes; ++index) {
            data[index] = static_cast<unsigned char>(seed + (index * 31U));
        }
    }

    bool check_pattern(const void* block, const size_t bytes, const unsigned char seed) {
        const auto* data = static_cast<const unsigned char*>(block);
        for (size_t index = 0; index < bytes; ++index) {
            if (data[index] != static_cast<unsigned char>(seed + (index * 31U))) {
                return false;
            }
        }
        return true;
    }

    size_t resident_kb() {
        const auto mi = process::get_memory_info(process::current_id());
        return mi.working_set_size;
    }
} // namespace

TEST(MemoryPoolSizeClass, MappingIsMonotonicAndSufficient) {
    size_t previous = 0;
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t block = memory_pool::block_size(index);
        EXPECT_GT(block, previous);
        EXPECT_EQ(block % memory_pool::min_align, 0U);
        previous = block;
    }
    EXPECT_EQ(memory_pool::block_size(memory_pool::class_count), 0U);

    for (size_t request = 1; request <= memory_pool::small_max; ++request) {
        const size_t index = memory_pool::size_to_class(request);
        ASSERT_LT(index, memory_pool::class_count);
        EXPECT_GE(memory_pool::block_size(index), request);
        if (index != 0) {
            EXPECT_LT(memory_pool::block_size(index - 1), request);
        }
    }
    EXPECT_EQ(memory_pool::size_to_class(0), 0U);
    EXPECT_EQ(memory_pool::size_to_class(memory_pool::small_max + 1), memory_pool::class_count);
    EXPECT_EQ(memory_pool::size_to_class(1U << 20), memory_pool::class_count);
}

TEST(MemoryPool, GeometryIsConsistent) {
    memory_pool pool;
    const memory_pool::statistics stats = pool.stats();
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t span = stats.classes[index].span_size;
        const size_t block = stats.classes[index].block_size;
        EXPECT_EQ(span % 4096, 0U);
        EXPECT_GT(span, block * 2);
        EXPECT_GT(block, 0U);
    }
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, AllocateEveryClassKeepsDataIntact) {
    memory_pool pool;
    vector<pair<void*, size_t>> blocks;
    for (size_t round = 0; round < 3; ++round) {
        for (size_t index = 0; index < memory_pool::class_count; ++index) {
            const size_t request = memory_pool::block_size(index) - round;
            void* block = pool.allocate(request);
            ASSERT_NE(block, nullptr);
            EXPECT_EQ(reinterpret_cast<uintptr_t>(block) % memory_pool::min_align, 0U);
            EXPECT_GE(pool.usable_size(block), request);
            EXPECT_TRUE(pool.owns(block));
            fill_pattern(block, request, static_cast<unsigned char>(index + round * 7 + 1));
            blocks.emplace_back(block, request);
        }
    }
    size_t index = 0;
    for (const auto& entry: blocks) {
        const size_t class_index = index % memory_pool::class_count;
        const size_t round = index / memory_pool::class_count;
        EXPECT_TRUE(check_pattern(entry.first, entry.second, static_cast<unsigned char>(class_index + round * 7 + 1)));
        ++index;
    }
    for (const auto& entry: blocks) {
        pool.deallocate(entry.first, entry.second);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, ZeroSizeAndNullPointer) {
    memory_pool pool;
    void* first = pool.allocate(0);
    void* second = pool.allocate(0);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_GE(pool.usable_size(first), 1U);
    pool.deallocate(first);
    pool.deallocate(second);
    pool.deallocate(nullptr);
    pool.deallocate(nullptr, 0);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, AlignmentRequestsAreHonored) {
    memory_pool pool;
    const size_t alignments[] = {16, 32, 64, 128, 256, 512, 4096, 65536};
    const size_t requests[] = {1, 17, 100, 4096, 16000, 17000, 100000, 1000000};
    for (const size_t alignment: alignments) {
        for (const size_t request: requests) {
            void* block = pool.allocate(request, alignment);
            ASSERT_NE(block, nullptr);
            EXPECT_EQ(reinterpret_cast<uintptr_t>(block) % alignment, 0U);
            EXPECT_GE(pool.usable_size(block), request);
            fill_pattern(block, request, 0x5A);
            EXPECT_TRUE(check_pattern(block, request, 0x5A));
            pool.deallocate(block, request);
        }
    }
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, LargeAllocationsAreIsolated) {
    memory_pool pool;
    vector<pair<void*, size_t>> blocks;
    const size_t sizes[] = {memory_pool::small_max + 1, 20000, 65536, 100000, 1U << 20, 4U << 20};
    for (const size_t size: sizes) {
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr);
        EXPECT_TRUE(pool.owns(block));
        EXPECT_GE(pool.usable_size(block), size);
        fill_pattern(block, size, 0x33);
        blocks.emplace_back(block, size);
    }
    for (const auto& entry: blocks) {
        EXPECT_TRUE(check_pattern(entry.first, entry.second, 0x33));
        pool.deallocate(entry.first, entry.second);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, LargeRegionCacheReusesMappings) {
    memory_pool pool;
    void* block = pool.allocate(30000);
    ASSERT_NE(block, nullptr);
    pool.deallocate(block);
    const size_t map_calls = pool.stats().os_map_calls;
    for (int round = 0; round < 50; ++round) {
        void* again = pool.allocate(30000);
        ASSERT_NE(again, nullptr);
        EXPECT_EQ(again, block);
        pool.deallocate(again);
    }
    EXPECT_EQ(pool.stats().os_map_calls, map_calls);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
}

TEST(MemoryPool, ReallocateKeepsContent) {
    memory_pool pool;
    void* block = pool.allocate(100);
    ASSERT_NE(block, nullptr);
    fill_pattern(block, 100, 0x11);

    void* grown = pool.reallocate(block, 5000);
    ASSERT_NE(grown, nullptr);
    EXPECT_TRUE(check_pattern(grown, 100, 0x11));

    void* shrunk = pool.reallocate(grown, 40);
    ASSERT_NE(shrunk, nullptr);
    EXPECT_TRUE(check_pattern(shrunk, 40, 0x11));

    void* large = pool.reallocate(shrunk, 1U << 20);
    ASSERT_NE(large, nullptr);
    EXPECT_TRUE(check_pattern(large, 40, 0x11));

    void* fresh = pool.reallocate(nullptr, 64);
    ASSERT_NE(fresh, nullptr);
    EXPECT_GE(pool.usable_size(fresh), 64U);

    pool.deallocate(fresh);
    pool.deallocate(large);
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, ReallocateInPlaceWhenCapacityIsSufficient) {
    memory_pool pool;
    void* block = pool.allocate(64);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(pool.reallocate(block, 32), block);
    EXPECT_EQ(pool.reallocate(block, 64), block);
    pool.deallocate(block);
}

TEST(MemoryPool, BlocksDoNotOverlap) {
    memory_pool pool;
    constexpr size_t count = 4096;
    set<uintptr_t> addresses;
    vector<void*> blocks;
    for (size_t index = 0; index < count; ++index) {
        void* block = pool.allocate(64);
        ASSERT_NE(block, nullptr);
        addresses.insert(reinterpret_cast<uintptr_t>(block));
        blocks.push_back(block);
    }
    EXPECT_EQ(addresses.size(), count);
    for (void* block: blocks) {
        memory_set(block, 0xEE, 64);
    }
    for (void* block: blocks) {
        const auto* data = static_cast<const unsigned char*>(block);
        for (size_t index = 0; index < 64; ++index) {
            ASSERT_EQ(data[index], 0xEE);
        }
    }
    for (void* block: blocks) {
        pool.deallocate(block, 64);
    }
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, CrossThreadFreeIsRoutedToTheOwner) {
    memory_pool pool;
    constexpr size_t thread_count = 4;
    constexpr size_t per_thread = 2000;
    vector<vector<void*>> bags(thread_count);

    vector<thread> producers;
    for (size_t index = 0; index < thread_count; ++index) {
        producers.emplace_back([&pool, &bags, index]() {
            for (size_t round = 0; round < per_thread; ++round) {
                const size_t size = (round % 500) + 1;
                void* block = pool.allocate(size);
                if (block == nullptr) {
                    continue;
                }
                memory_set(block, static_cast<int>(index), size);
                bags[index].push_back(block);
            }
        });
    }
    for (auto& worker: producers) {
        worker.join();
    }

    vector<thread> consumers;
    for (size_t index = 0; index < thread_count; ++index) {
        consumers.emplace_back([&pool, &bags, index]() {
            for (void* block: bags[(index + 1) % thread_count]) {
                pool.deallocate(block);
            }
        });
    }
    for (auto& worker: consumers) {
        worker.join();
    }

    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, ThreadExitReturnsCachedBlocks) {
    memory_pool pool;
    {
        vector<thread> workers;
        for (size_t index = 0; index < 4; ++index) {
            workers.emplace_back([&pool]() {
                for (size_t round = 0; round < 5000; ++round) {
                    const size_t size = (round % 700) + 1;
                    void* block = pool.allocate(size);
                    pool.deallocate(block, size);
                }
            });
        }
        for (auto& worker: workers) {
            worker.join();
        }
    }
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, PurgeReleasesResidentMemory) {
    memory_pool pool;
    const size_t before = resident_kb();
    vector<void*> blocks;
    for (size_t index = 0; index < 8000; ++index) {
        void* block = pool.allocate(256);
        ASSERT_NE(block, nullptr);
        memory_set(block, 0x77, 256);
        blocks.push_back(block);
    }
    const size_t inflated = resident_kb();
    for (void* block: blocks) {
        pool.deallocate(block, 256);
    }
    pool.flush_thread_cache();
    EXPECT_GT(pool.stats().mapped_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    const size_t after = resident_kb();
    EXPECT_LT(after, inflated);
    EXPECT_LT(after, before + 262144U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, MultiThreadedChurnKeepsDataIntact) {
    memory_pool pool;
    constexpr size_t thread_count = 8;
    constexpr size_t rounds = 4000;
    atomic<int> errors{0};
    vector<thread> workers;
    for (size_t index = 0; index < thread_count; ++index) {
        workers.emplace_back([&pool, &errors, index]() {
            vector<pair<void*, size_t>> live;
            uint32_t seed = static_cast<uint32_t>(index * 7919 + 13);
            for (size_t round = 0; round < rounds; ++round) {
                seed = seed * 1103515245U + 12345U;
                const size_t size = (seed >> 9) % 20000 + 1;
                void* block = pool.allocate(size);
                if (block == nullptr || (reinterpret_cast<uintptr_t>(block) % memory_pool::min_align) != 0) {
                    ++errors;
                    continue;
                }
                fill_pattern(block, size, static_cast<unsigned char>(index));
                if (!check_pattern(block, size, static_cast<unsigned char>(index))) {
                    ++errors;
                }
                if ((seed & 3U) == 0U && !live.empty()) {
                    const size_t victim = (seed >> 16) % live.size();
                    const auto entry = live[victim];
                    if (!check_pattern(entry.first, entry.second, static_cast<unsigned char>(index))) {
                        ++errors;
                    }
                    pool.deallocate(entry.first, entry.second);
                    live[victim] = make_pair(block, size);
                } else {
                    live.emplace_back(block, size);
                }
            }
            for (const auto& entry: live) {
                if (!check_pattern(entry.first, entry.second, static_cast<unsigned char>(index))) {
                    ++errors;
                }
                pool.deallocate(entry.first, entry.second);
            }
        });
    }
    for (auto& worker: workers) {
        worker.join();
    }
    EXPECT_EQ(errors.load(), 0);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, StatisticsTrackLiveAllocations) {
    memory_pool pool;
    const memory_pool::statistics before = pool.stats();
    EXPECT_EQ(before.active_bytes, 0U);

    vector<void*> blocks;
    size_t expected = 0;
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t request = memory_pool::block_size(index);
        void* block = pool.allocate(request);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
        expected += memory_pool::block_size(index);
    }
    const memory_pool::statistics during = pool.stats();
    EXPECT_GE(during.active_bytes, expected);
    EXPECT_GE(during.peak_active_bytes, during.active_bytes);
    EXPECT_GT(during.mapped_bytes, 0U);
    EXPECT_GT(during.os_map_calls, 0U);

    for (size_t index = 0; index < blocks.size(); ++index) {
        pool.deallocate(blocks[index], memory_pool::block_size(index));
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().os_map_calls, pool.stats().os_unmap_calls);
}

TEST(MemoryPool, MultiplePoolsAreIndependent) {
    memory_pool first;
    memory_pool second;
    EXPECT_NE(first.id(), second.id());

    void* from_first = first.allocate(128);
    void* from_second = second.allocate(128);
    ASSERT_NE(from_first, nullptr);
    ASSERT_NE(from_second, nullptr);
    EXPECT_TRUE(first.owns(from_first));
    EXPECT_FALSE(first.owns(from_second));
    EXPECT_TRUE(second.owns(from_second));
    EXPECT_FALSE(second.owns(from_first));

    first.deallocate(from_second, 128);
    second.deallocate(from_first, 128);

    first.flush_thread_cache();
    second.flush_thread_cache();
    EXPECT_EQ(second.stats().active_bytes, 0U);
    EXPECT_TRUE(first.verify());
    EXPECT_TRUE(second.verify());
}

TEST(MemoryPool, ThreadCacheRebindsBetweenPools) {
    memory_pool first;
    memory_pool second;
    for (int round = 0; round < 64; ++round) {
        void* a = first.allocate(96);
        void* b = second.allocate(96);
        ASSERT_NE(a, nullptr);
        ASSERT_NE(b, nullptr);
        memory_set(a, 0xA1, 96);
        memory_set(b, 0xB2, 96);
        first.deallocate(a, 96);
        second.deallocate(b, 96);
    }
    first.flush_thread_cache();
    second.flush_thread_cache();
    EXPECT_EQ(first.stats().active_bytes, 0U);
    EXPECT_EQ(second.stats().active_bytes, 0U);
    EXPECT_TRUE(first.verify());
    EXPECT_TRUE(second.verify());
}

TEST(MemoryPool, DestroyedPoolKeepsOtherPoolsUsable) {
    memory_pool* temporary = new memory_pool();
    void* block = temporary->allocate(512);
    ASSERT_NE(block, nullptr);
    temporary->deallocate(block, 512);
    temporary->flush_thread_cache();
    EXPECT_TRUE(temporary->verify());
    delete temporary;

    memory_pool survivor;
    void* fresh = survivor.allocate(512);
    ASSERT_NE(fresh, nullptr);
    memory_set(fresh, 0x42, 512);
    survivor.deallocate(fresh, 512);
    survivor.flush_thread_cache();
    EXPECT_TRUE(survivor.verify());
    EXPECT_EQ(survivor.stats().active_bytes, 0U);
}

TEST(MemoryPool, OwnershipQueriesRejectForeignPointers) {
    memory_pool pool;
    int stack_value = 0;
    EXPECT_FALSE(pool.owns(&stack_value));
    EXPECT_EQ(pool.usable_size(&stack_value), 0U);
    void* heap = malloc(64);
    ASSERT_NE(heap, nullptr);
    EXPECT_FALSE(pool.owns(heap));
    free(heap);
    EXPECT_EQ(pool.usable_size(nullptr), 0U);
}

TEST(MemoryPool, ThreadCacheCanBeDisabled) {
    memory_pool::options opts;
    opts.thread_cache_enabled = false;
    opts.purge_on_empty = true;
    memory_pool pool(opts);

    vector<void*> blocks;
    for (size_t index = 0; index < 500; ++index) {
        void* block = pool.allocate((index % 900) + 1);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
    }
    for (void* block: blocks) {
        pool.deallocate(block);
    }
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, ManySpansAreAllocatedAndReleased) {
    memory_pool pool;
    memory_pool::options opts;
    opts.thread_cache_max = 8;
    opts.thread_cache_batch = 8;
    opts.max_empty_spans = 1;
    memory_pool small(opts);

    vector<void*> blocks;
    for (size_t index = 0; index < 20000; ++index) {
        void* block = small.allocate(16);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
    }
    set<uintptr_t> addresses;
    for (void* block: blocks) {
        addresses.insert(reinterpret_cast<uintptr_t>(block));
    }
    EXPECT_EQ(addresses.size(), blocks.size());
    EXPECT_TRUE(small.verify());
    for (void* block: blocks) {
        small.deallocate(block, 16);
    }
    small.flush_thread_cache();
    small.purge();
    EXPECT_EQ(small.stats().active_bytes, 0U);
    EXPECT_EQ(small.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPool, SystemPoolIsStable) {
    memory_pool& first = system_memory_pool();
    memory_pool& second = system_memory_pool();
    EXPECT_EQ(&first, &second);
    void* block = first.allocate(200);
    ASSERT_NE(block, nullptr);
    memory_set(block, 0x19, 200);
    EXPECT_TRUE(first.owns(block));
    first.deallocate(block, 200);
    EXPECT_TRUE(first.verify());
}

TEST(PoolAllocator, WorksWithStandardContainers) {
    memory_pool pool;
    {
        vector<int, pool_allocator<int>> values{pool_allocator<int>(pool)};
        for (int index = 0; index < 10000; ++index) {
            values.push_back(index);
        }
        for (int index = 0; index < 10000; ++index) {
            ASSERT_EQ(values[static_cast<size_t>(index)], index);
        }
    }
    {
        list<string, pool_allocator<list_node<string>>> items{pool_allocator<list_node<string>>(pool)};
        for (int index = 0; index < 2000; ++index) {
            items.emplace_back(64, static_cast<char>('a' + (index % 26)));
        }
        EXPECT_EQ(items.size(), 2000U);
        EXPECT_EQ(items.front().size(), 64U);
    }
    {
        map<int, int, less<int>, pool_allocator<rb_tree_node<pair<const int, int>>>> table{
                less<int>(), pool_allocator<rb_tree_node<pair<const int, int>>>(pool)};
        for (int index = 0; index < 2000; ++index) {
            table[index] = index * 3;
        }
        EXPECT_EQ(table.size(), 2000U);
        EXPECT_EQ(table[1999], 5997);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(PoolAllocator, EqualityFollowsTheBoundPool) {
    memory_pool first;
    memory_pool second;
    pool_allocator<int> default_allocator;
    pool_allocator<int> first_allocator(first);
    pool_allocator<int> second_allocator(second);
    pool_allocator<long> rebound(first_allocator);

    EXPECT_EQ(default_allocator.pool(), &system_memory_pool());
    EXPECT_TRUE(first_allocator == rebound);
    EXPECT_FALSE(first_allocator == second_allocator);
    EXPECT_TRUE(first_allocator != second_allocator);
    EXPECT_EQ(first_allocator.max_size(), static_cast<size_t>(-1) / sizeof(int));
}

TEST(PoolAllocator, AllocateAndDeallocateRoundTrip) {
    memory_pool pool;
    pool_allocator<int> allocator(pool);
    int* block = allocator.allocate(1000);
    ASSERT_NE(block, nullptr);
    for (int index = 0; index < 1000; ++index) {
        block[index] = index;
    }
    EXPECT_EQ(block[999], 999);
    allocator.deallocate(block, 1000);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}
