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

    size_t resident_bytes() {
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
    const size_t before = resident_bytes();
    vector<void*> blocks;
    for (size_t index = 0; index < 20000; ++index) {
        void* block = pool.allocate(256);
        ASSERT_NE(block, nullptr);
        memory_set(block, 0x77, 256);
        blocks.push_back(block);
    }
    const size_t inflated = resident_bytes();
    const size_t pool_bytes = pool.stats().peak_mapped_bytes;
    for (void* block: blocks) {
        pool.deallocate(block, 256);
    }
    pool.flush_thread_cache();
    EXPECT_GT(pool.stats().mapped_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().cached_empty_bytes, 0U);
    EXPECT_TRUE(pool.verify());
    const size_t after = resident_bytes();
    if (inflated <= before || inflated - before > pool_bytes * 4) {
        GTEST_SKIP() << "resident size is dominated by an external profiler";
    }
    EXPECT_LT(after, inflated);
    EXPECT_GT(inflated - after, (inflated - before) / 4);
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

TEST(MemoryPool, LibraryContainersAreServedByTheSystemPool) {
#ifdef NEFORCE_USING_MEMORY_POOL
    memory_pool& pool = system_memory_pool();
    {
        string text(96, 'x');
        EXPECT_TRUE(pool.owns(text.data()));
        vector<int> values;
        values.reserve(4096);
        EXPECT_TRUE(pool.owns(values.data()));
        EXPECT_EQ(values.capacity(), 4096U);
    }
    pool.flush_thread_cache();
    EXPECT_TRUE(pool.verify());
#else
    GTEST_SKIP() << "NEXUSFORCE_USING_MEMORY_POOL is disabled";
#endif
}

TEST(MemoryPool, GlobalOperatorNewStaysOnTheCrtWithoutTheExplicitOverride) {
#if defined(NEFORCE_USING_MEMORY_POOL_OVERRIDE)
    GTEST_SKIP() << "the process wide allocator override is enabled";
#else
    memory_pool& pool = system_memory_pool();
    void* block = ::operator new(4096);
    ASSERT_NE(block, nullptr);
    EXPECT_FALSE(pool.owns(block));
    ::operator delete(block);
#endif
}

TEST(MemoryPool, ForeignReleaseIsCountedInReleaseBuilds) {
#if defined(NEFORCE_STATE_DEBUG) || defined(NEFORCE_USING_MEMORY_POOL_OVERRIDE)
    GTEST_SKIP() << "debug builds assert on foreign pointers and the override owns the global operators";
#else
    memory_pool& pool = system_memory_pool();
    const size_t before = pool.foreign_release_count();
    void* block = ::operator new(64);
    ASSERT_NE(block, nullptr);
    pool.deallocate(block);
    EXPECT_EQ(pool.foreign_release_count(), before + 1);
    EXPECT_EQ(pool.stats().foreign_releases, pool.foreign_release_count());
    ::operator delete(block);
#endif
}

TEST(MemoryPoolSizeClassLookup, MatchesSmallestSufficientClassForEveryRequest) {
    for (size_t bytes = 0; bytes <= 40000; ++bytes) {
        const size_t request = bytes == 0 ? 1 : bytes;
        size_t expected = memory_pool::class_count;
        if (request <= memory_pool::small_max) {
            for (size_t index = 0; index < memory_pool::class_count; ++index) {
                if (memory_pool::block_size(index) >= request) {
                    expected = index;
                    break;
                }
            }
        }
        ASSERT_EQ(memory_pool::size_to_class(bytes), expected) << "request = " << bytes;
    }
}

TEST(MemoryPoolSizeClassLookup, BoundaryRequestsMapToTheirOwnClass) {
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t block = memory_pool::block_size(index);
        EXPECT_EQ(memory_pool::size_to_class(block), index);
        if (index + 1 < memory_pool::class_count) {
            EXPECT_EQ(memory_pool::size_to_class(block + 1), index + 1);
        }
    }
    EXPECT_EQ(memory_pool::size_to_class(0), 0U);
    EXPECT_EQ(memory_pool::size_to_class(1), 0U);
    EXPECT_EQ(memory_pool::size_to_class(memory_pool::min_align), 0U);
    EXPECT_EQ(memory_pool::size_to_class(memory_pool::small_max), memory_pool::class_count - 1);
    EXPECT_EQ(memory_pool::size_to_class(memory_pool::small_max + 1), memory_pool::class_count);
    EXPECT_EQ(memory_pool::size_to_class(static_cast<size_t>(-1)), memory_pool::class_count);
}

TEST(MemoryPoolSizeClassLookup, SizesAroundTableBoundariesStayExact) {
    const size_t boundaries[] = {16, 64, 128, 256, 512, 1024, 1025, 2048, 4096, 8192, 16384};
    for (const size_t boundary: boundaries) {
        for (size_t delta = 0; delta < 8; ++delta) {
            const size_t request = boundary + delta;
            const size_t index = memory_pool::size_to_class(request);
            if (request > memory_pool::small_max) {
                EXPECT_EQ(index, memory_pool::class_count) << "request = " << request;
                continue;
            }
            ASSERT_LT(index, memory_pool::class_count) << "request = " << request;
            EXPECT_GE(memory_pool::block_size(index), request);
            if (index != 0) {
                EXPECT_LT(memory_pool::block_size(index - 1), request);
            }
        }
    }
}

TEST(MemoryPoolSizeClassLookup, RequestsBeyondTheLookupTableStayClassExact) {
    for (size_t request = memory_pool::lookup_max + 1; request <= memory_pool::small_max; request += 37) {
        const size_t index = memory_pool::size_to_class(request);
        ASSERT_LT(index, memory_pool::class_count) << "request = " << request;
        EXPECT_GE(memory_pool::block_size(index), request);
        if (index != 0) {
            EXPECT_LT(memory_pool::block_size(index - 1), request);
        }
    }
    EXPECT_EQ(memory_pool::size_to_class(memory_pool::small_max + 1), memory_pool::class_count);
    EXPECT_EQ(memory_pool::size_to_class(1U << 30), memory_pool::class_count);
}

TEST(MemoryPoolGeometryExtended, SpanGeometryHoldsAtLeastTwoBlocksPerClass) {
    memory_pool pool;
    const memory_pool::statistics stats = pool.stats();
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t block = memory_pool::block_size(index);
        const size_t span = stats.classes[index].span_size;
        EXPECT_EQ(stats.classes[index].block_size, block);
        EXPECT_GT(span, block);
        EXPECT_EQ(span % 4096U, 0U);
        EXPECT_GE(span / block, 2U);
        EXPECT_EQ(stats.classes[index].span_count, 0U);
        EXPECT_EQ(stats.classes[index].active_blocks, 0U);
    }
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().os_map_calls, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolGeometryExtended, EveryClassDeliversItsOwnBlockSize) {
    memory_pool pool;
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t block = memory_pool::block_size(index);
        void* address = pool.allocate(block);
        ASSERT_NE(address, nullptr);
        EXPECT_EQ(pool.usable_size(address), block);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(address) % memory_pool::min_align, 0U);
        EXPECT_TRUE(pool.owns(address));
        pool.deallocate(address, block);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_blocks, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolAllocationExtended, UsableSizeCoversOddRequests) {
    memory_pool pool;
    for (size_t request = 1; request <= 2000; request += 7) {
        void* block = pool.allocate(request);
        ASSERT_NE(block, nullptr);
        const size_t usable = pool.usable_size(block);
        ASSERT_GE(usable, request) << "request = " << request;
        fill_pattern(block, usable, 0x27);
        EXPECT_TRUE(check_pattern(block, usable, 0x27));
        pool.deallocate(block, request);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolAllocationExtended, WholeUsableSizeIsWritableForEveryClass) {
    memory_pool pool;
    vector<pair<void*, size_t>> blocks;
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        const size_t request = memory_pool::block_size(index);
        void* block = pool.allocate(request);
        ASSERT_NE(block, nullptr);
        const size_t usable = pool.usable_size(block);
        EXPECT_EQ(usable, request);
        fill_pattern(block, usable, static_cast<unsigned char>(index));
        blocks.emplace_back(block, usable);
    }
    for (size_t index = 0; index < blocks.size(); ++index) {
        EXPECT_TRUE(check_pattern(blocks[index].first, blocks[index].second, static_cast<unsigned char>(index)));
    }
    while (!blocks.empty()) {
        const auto entry = blocks.back();
        blocks.pop_back();
        pool.deallocate(entry.first, entry.second);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolAllocationExtended, OversizedRequestsAreRejectedWithoutMapping) {
    memory_pool pool;
    const size_t limit = static_cast<size_t>(1) << 46;
    EXPECT_EQ(pool.try_allocate(limit + 1), nullptr);
    EXPECT_EQ(pool.try_allocate(limit + 4096), nullptr);
    EXPECT_EQ(pool.try_allocate(static_cast<size_t>(1) << 47), nullptr);
    EXPECT_EQ(pool.try_allocate(static_cast<size_t>(-1)), nullptr);
    EXPECT_THROW(pool.allocate(limit + 1), allocate_exception);
    EXPECT_THROW(pool.allocate(static_cast<size_t>(-1)), allocate_exception);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().os_map_calls, 0U);
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolAllocationExtended, ZeroAndTinyRequestsAreDistinct) {
    memory_pool pool;
    void* zero = pool.allocate(0);
    void* one = pool.allocate(1);
    void* tiny = pool.allocate(2);
    ASSERT_NE(zero, nullptr);
    ASSERT_NE(one, nullptr);
    ASSERT_NE(tiny, nullptr);
    EXPECT_NE(zero, one);
    EXPECT_NE(one, tiny);
    EXPECT_GE(pool.usable_size(zero), 1U);
    EXPECT_GE(pool.usable_size(one), 1U);
    EXPECT_GE(pool.usable_size(tiny), 2U);
    fill_pattern(zero, pool.usable_size(zero), 0x01);
    fill_pattern(one, pool.usable_size(one), 0x02);
    fill_pattern(tiny, pool.usable_size(tiny), 0x03);
    EXPECT_TRUE(check_pattern(zero, pool.usable_size(zero), 0x01));
    EXPECT_TRUE(check_pattern(one, pool.usable_size(one), 0x02));
    EXPECT_TRUE(check_pattern(tiny, pool.usable_size(tiny), 0x03));
    pool.deallocate(tiny, 2);
    pool.deallocate(one, 1);
    pool.deallocate(zero, 0);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolAllocationExtended, NonStandardAlignmentsAreHonored) {
    memory_pool pool;
    const size_t alignments[] = {32, 64, 128, 256, 1024, 4096, 8192, 65536};
    const size_t requests[] = {1, 64, 1000, 4096, 16000, 20000, 200000};
    for (const size_t alignment: alignments) {
        for (const size_t request: requests) {
            void* block = pool.allocate(request, alignment);
            ASSERT_NE(block, nullptr) << "request = " << request << " alignment = " << alignment;
            EXPECT_EQ(reinterpret_cast<uintptr_t>(block) % alignment, 0U) << "request = " << request;
            EXPECT_GE(pool.usable_size(block), request);
            EXPECT_TRUE(pool.owns(block));
            fill_pattern(block, request, 0x5A);
            EXPECT_TRUE(check_pattern(block, request, 0x5A));
            pool.deallocate(block, request);
        }
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolAllocationExtended, ZeroAlignmentFallsBackToTheMinimum) {
    memory_pool pool;
    void* small = pool.allocate(100, 0);
    void* large = pool.allocate(40000, 0);
    ASSERT_NE(small, nullptr);
    ASSERT_NE(large, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(small) % memory_pool::min_align, 0U);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(large) % memory_pool::min_align, 0U);
    EXPECT_GE(pool.usable_size(small), 100U);
    EXPECT_GE(pool.usable_size(large), 40000U);
    pool.deallocate(large, 40000);
    pool.deallocate(small, 100);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, SizesAcrossTheLargeRangeStayIsolated) {
    memory_pool pool;
    const size_t sizes[] = {memory_pool::small_max + 1, 20000, 65536, 100000, 1U << 20, 4U << 20};
    vector<pair<void*, size_t>> blocks;
    for (const size_t size: sizes) {
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr) << "size = " << size;
        EXPECT_TRUE(pool.owns(block));
        EXPECT_GE(pool.usable_size(block), size);
        fill_pattern(block, size, 0x33);
        blocks.emplace_back(block, size);
    }
    for (const auto& entry: blocks) {
        EXPECT_TRUE(check_pattern(entry.first, entry.second, 0x33));
    }
    for (const auto& entry: blocks) {
        pool.deallocate(entry.first, entry.second);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().large_mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, LargeBlocksDoNotDisturbSmallStatistics) {
    memory_pool pool;
    const memory_pool::statistics before = pool.stats();
    void* block = pool.allocate(1U << 20);
    ASSERT_NE(block, nullptr);
    const memory_pool::statistics during = pool.stats();
    EXPECT_EQ(during.active_blocks, before.active_blocks);
    EXPECT_EQ(during.active_bytes, before.active_bytes);
    EXPECT_GT(during.large_mapped_bytes, before.large_mapped_bytes);
    EXPECT_GT(during.mapped_bytes, before.mapped_bytes);
    pool.deallocate(block);
    pool.flush_thread_cache();
    pool.purge();
    const memory_pool::statistics after = pool.stats();
    EXPECT_EQ(after.large_mapped_bytes, 0U);
    EXPECT_EQ(after.cached_region_bytes, 0U);
    EXPECT_EQ(after.mapped_bytes, 0U);
    EXPECT_EQ(after.active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, RegionCacheAvoidsRepeatedMappings) {
    memory_pool::options opts;
    opts.region_cache_enabled = true;
    opts.max_cached_regions = 4;
    memory_pool pool(opts);
    void* first = pool.allocate(30000);
    ASSERT_NE(first, nullptr);
    pool.deallocate(first);
    const size_t map_calls = pool.stats().os_map_calls;
    const size_t unmap_calls = pool.stats().os_unmap_calls;
    for (int round = 0; round < 64; ++round) {
        void* block = pool.allocate(30000);
        ASSERT_NE(block, nullptr);
        pool.deallocate(block);
    }
    EXPECT_EQ(pool.stats().os_map_calls, map_calls);
    EXPECT_EQ(pool.stats().os_unmap_calls, unmap_calls);
    EXPECT_GT(pool.stats().cached_region_bytes, 0U);
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().cached_region_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, DisabledRegionCacheReturnsMemoryImmediately) {
    memory_pool::options opts;
    opts.region_cache_enabled = false;
    memory_pool pool(opts);
    void* block = pool.allocate(48000);
    ASSERT_NE(block, nullptr);
    const size_t map_calls = pool.stats().os_map_calls;
    pool.deallocate(block);
    EXPECT_GT(pool.stats().os_unmap_calls, 0U);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().cached_region_bytes, 0U);
    void* again = pool.allocate(48000);
    ASSERT_NE(again, nullptr);
    EXPECT_GT(pool.stats().os_map_calls, map_calls);
    pool.deallocate(again);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, SmallThreadRegionCapStillReclaimsEverything) {
    memory_pool::options opts;
    opts.thread_region_bytes = 32U << 10;
    memory_pool pool(opts);
    EXPECT_EQ(pool.config().thread_region_bytes, 32U << 10);
    void* block = pool.allocate(300U << 10);
    ASSERT_NE(block, nullptr);
    fill_pattern(block, 300U << 10, 0x48);
    EXPECT_TRUE(check_pattern(block, 300U << 10, 0x48));
    pool.deallocate(block, 300U << 10);
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().large_mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolThreadCacheExtended, DisabledCacheDeliversExactlyOneBlock) {
    memory_pool::options opts;
    opts.thread_cache_enabled = false;
    memory_pool pool(opts);
    const size_t class_index = memory_pool::size_to_class(80);
    void* block = pool.allocate(80);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(pool.stats().active_blocks, 1U);
    EXPECT_EQ(pool.stats().active_bytes, memory_pool::block_size(class_index));
    pool.deallocate(block, 80);
    EXPECT_EQ(pool.stats().active_blocks, 0U);
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolThreadCacheExtended, ReusesTheMostRecentlyReleasedBlock) {
    memory_pool pool;
    void* first = pool.allocate(256);
    void* second = pool.allocate(256);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    pool.deallocate(first, 256);
    void* reused = pool.allocate(256);
    EXPECT_EQ(reused, first);
    pool.deallocate(reused, 256);
    pool.deallocate(second, 256);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolThreadCacheExtended, LargerByteTargetRefillsLargerBatches) {
    memory_pool::options small_opts;
    small_opts.thread_cache_bytes = 512;
    small_opts.thread_cache_batch = 8;
    small_opts.thread_cache_max_bytes = 4096;
    memory_pool small_pool(small_opts);

    memory_pool::options large_opts;
    large_opts.thread_cache_bytes = 65536;
    large_opts.thread_cache_batch = 32;
    large_opts.thread_cache_max_bytes = 1U << 20;
    memory_pool large_pool(large_opts);

    void* small_block = small_pool.allocate(64);
    ASSERT_NE(small_block, nullptr);
    const size_t small_active = small_pool.stats().active_bytes;
    void* large_block = large_pool.allocate(64);
    ASSERT_NE(large_block, nullptr);
    const size_t large_active = large_pool.stats().active_bytes;
    EXPECT_EQ(small_active, 8U * 64U);
    EXPECT_GT(large_active, small_active);
    EXPECT_LE(large_active, 256U * 64U);
    small_pool.deallocate(small_block, 64);
    large_pool.deallocate(large_block, 64);
    small_pool.flush_thread_cache();
    large_pool.flush_thread_cache();
    EXPECT_EQ(small_pool.stats().active_bytes, 0U);
    EXPECT_EQ(large_pool.stats().active_bytes, 0U);
    EXPECT_TRUE(small_pool.verify());
    EXPECT_TRUE(large_pool.verify());
}

TEST(MemoryPoolThreadCacheExtended, WorkerThreadFlushReleasesItsBlocks) {
    memory_pool pool;
    atomic<size_t> failures{0};
    thread worker([&pool, &failures]() {
        for (size_t index = 0; index < 4000; ++index) {
            const size_t size = (index % 900) + 1;
            void* block = pool.allocate(size);
            if (block == nullptr) {
                ++failures;
                continue;
            }
            fill_pattern(block, size, 0x61);
            if (!check_pattern(block, size, 0x61)) {
                ++failures;
            }
            pool.deallocate(block, size);
        }
        pool.flush_thread_cache();
        if (pool.stats().active_bytes != 0) {
            ++failures;
        }
    });
    worker.join();
    EXPECT_EQ(failures.load(), 0U);
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolThreadCacheExtended, RepeatedFlushKeepsAccountingStable) {
    memory_pool pool;
    void* block = pool.allocate(128);
    ASSERT_NE(block, nullptr);
    pool.deallocate(block, 128);
    pool.flush_thread_cache();
    const memory_pool::statistics first = pool.stats();
    pool.flush_thread_cache();
    pool.flush_thread_cache();
    const memory_pool::statistics second = pool.stats();
    EXPECT_EQ(first.active_bytes, second.active_bytes);
    EXPECT_EQ(first.active_blocks, second.active_blocks);
    EXPECT_EQ(first.mapped_bytes, second.mapped_bytes);
    EXPECT_EQ(second.active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolSpanPolicyExtended, PurgeOnEmptyReturnsSpansImmediately) {
    memory_pool::options opts;
    opts.purge_on_empty = true;
    memory_pool pool(opts);
    EXPECT_EQ(pool.config().purge_on_empty, true);
    vector<void*> blocks;
    for (size_t index = 0; index < 4000; ++index) {
        void* block = pool.allocate(96);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
    }
    EXPECT_GT(pool.stats().mapped_bytes, 0U);
    for (void* block: blocks) {
        pool.deallocate(block, 96);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().cached_empty_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolSpanPolicyExtended, DefaultRetainsWarmSpans) {
    memory_pool pool;
    const size_t class_index = memory_pool::size_to_class(96);
    void* block = pool.allocate(96);
    ASSERT_NE(block, nullptr);
    pool.deallocate(block, 96);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_GT(pool.stats().cached_empty_bytes, 0U);
    EXPECT_GT(pool.stats().mapped_bytes, 0U);
    EXPECT_GT(pool.stats().classes[class_index].empty_spans, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().cached_empty_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolSpanPolicyExtended, ZeroByteBudgetBoundsRetainedEmptySpans) {
    memory_pool::options opts;
    opts.max_empty_spans = 1;
    opts.max_empty_span_bytes = 0;
    memory_pool pool(opts);
    const size_t block_size = memory_pool::block_size(0);
    vector<void*> blocks;
    for (size_t index = 0; index < 8000; ++index) {
        void* block = pool.allocate(block_size);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
    }
    for (void* block: blocks) {
        pool.deallocate(block, block_size);
    }
    pool.flush_thread_cache();
    const memory_pool::statistics stats = pool.stats();
    EXPECT_LE(stats.cached_empty_bytes, stats.classes[0].span_size);
    EXPECT_EQ(stats.active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolSpanPolicyExtended, WarmSpansAreReusedWithoutNewMappings) {
    memory_pool pool;
    void* block = pool.allocate(256);
    ASSERT_NE(block, nullptr);
    pool.deallocate(block, 256);
    pool.flush_thread_cache();
    const size_t map_calls = pool.stats().os_map_calls;
    const size_t span_count = pool.stats().classes[memory_pool::size_to_class(256)].span_count;
    for (int round = 0; round < 32; ++round) {
        void* recycled = pool.allocate(256);
        ASSERT_NE(recycled, nullptr);
        fill_pattern(recycled, 256, 0x37);
        EXPECT_TRUE(check_pattern(recycled, 256, 0x37));
        pool.deallocate(recycled, 256);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().os_map_calls, map_calls);
    EXPECT_EQ(pool.stats().classes[memory_pool::size_to_class(256)].span_count, span_count);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolStatisticsExtended, PeakCountersNeverDecrease) {
    memory_pool pool;
    size_t peak_active = pool.stats().peak_active_bytes;
    size_t peak_mapped = pool.stats().peak_mapped_bytes;
    for (size_t round = 0; round < 8; ++round) {
        vector<void*> blocks;
        for (size_t index = 0; index < 512; ++index) {
            void* block = pool.allocate((index % 17) * 64 + 1);
            ASSERT_NE(block, nullptr);
            blocks.push_back(block);
        }
        const memory_pool::statistics during = pool.stats();
        EXPECT_GE(during.peak_active_bytes, peak_active);
        EXPECT_GE(during.peak_mapped_bytes, peak_mapped);
        EXPECT_GE(during.peak_active_bytes, during.active_bytes);
        EXPECT_GE(during.peak_mapped_bytes, during.mapped_bytes);
        peak_active = during.peak_active_bytes;
        peak_mapped = during.peak_mapped_bytes;
        for (void* block: blocks) {
            pool.deallocate(block);
        }
    }
    pool.flush_thread_cache();
    const memory_pool::statistics after = pool.stats();
    EXPECT_EQ(after.active_bytes, 0U);
    EXPECT_EQ(after.peak_active_bytes, peak_active);
    EXPECT_GE(after.peak_mapped_bytes, peak_mapped);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolStatisticsExtended, PerClassCountersSumToTotals) {
    memory_pool pool;
    vector<void*> blocks;
    for (size_t index = 0; index < 3000; ++index) {
        void* block = pool.allocate((index % 61) * 128 + 1);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
    }
    const memory_pool::statistics stats = pool.stats();
    size_t class_blocks = 0;
    size_t class_bytes = 0;
    for (size_t index = 0; index < memory_pool::class_count; ++index) {
        EXPECT_EQ(stats.classes[index].block_size, memory_pool::block_size(index));
        EXPECT_EQ(stats.classes[index].active_bytes,
                  stats.classes[index].active_blocks * stats.classes[index].block_size);
        class_blocks += stats.classes[index].active_blocks;
        class_bytes += stats.classes[index].active_bytes;
    }
    EXPECT_EQ(class_blocks, stats.active_blocks);
    EXPECT_EQ(class_bytes, stats.active_bytes);
    EXPECT_GT(stats.active_blocks, 0U);
    for (void* block: blocks) {
        pool.deallocate(block);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_blocks, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolStatisticsExtended, MapAndUnmapCallsBalanceAfterPurge) {
    memory_pool pool;
    vector<void*> small_blocks;
    for (size_t index = 0; index < 3000; ++index) {
        small_blocks.push_back(pool.allocate(48 + index % 64));
    }
    vector<void*> large_blocks;
    for (size_t index = 0; index < 16; ++index) {
        large_blocks.push_back(pool.allocate(20000 + index * 4096));
    }
    EXPECT_GT(pool.stats().small_mapped_bytes, 0U);
    EXPECT_GT(pool.stats().large_mapped_bytes, 0U);
    for (void* block: large_blocks) {
        pool.deallocate(block);
    }
    for (void* block: small_blocks) {
        pool.deallocate(block);
    }
    pool.flush_thread_cache();
    pool.purge();
    const memory_pool::statistics stats = pool.stats();
    EXPECT_EQ(stats.mapped_bytes, 0U);
    EXPECT_EQ(stats.small_mapped_bytes, 0U);
    EXPECT_EQ(stats.large_mapped_bytes, 0U);
    EXPECT_EQ(stats.os_map_calls, stats.os_unmap_calls);
    EXPECT_GT(stats.os_map_calls, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolStatisticsExtended, ForeignReleaseCounterTracksForeignPointers) {
#if defined(NEFORCE_STATE_DEBUG) || defined(NEFORCE_USING_MEMORY_POOL_OVERRIDE)
    GTEST_SKIP() << "debug builds assert on foreign pointers and the override owns the global operators";
#else
    memory_pool pool;
    const size_t before = pool.foreign_release_count();
    int local = 0;
    pool.deallocate(&local);
    pool.deallocate(nullptr);
    EXPECT_EQ(pool.foreign_release_count(), before + 1);
    EXPECT_EQ(pool.stats().foreign_releases, pool.foreign_release_count());
    EXPECT_EQ(pool.usable_size(&local), 0U);
    EXPECT_FALSE(pool.owns(&local));
    EXPECT_TRUE(pool.verify());
#endif
}

TEST(MemoryPoolStatisticsExtended, EmptySpanCountersFollowPurge) {
    memory_pool pool;
    const size_t class_index = memory_pool::size_to_class(96);
    vector<void*> blocks;
    for (size_t index = 0; index < 6000; ++index) {
        void* block = pool.allocate(96);
        ASSERT_NE(block, nullptr);
        blocks.push_back(block);
    }
    for (void* block: blocks) {
        pool.deallocate(block, 96);
    }
    pool.flush_thread_cache();
    const memory_pool::statistics retained = pool.stats();
    EXPECT_GT(retained.classes[class_index].empty_spans, 0U);
    EXPECT_GE(retained.cached_empty_bytes, retained.classes[class_index].span_size);
    EXPECT_EQ(retained.active_bytes, 0U);
    pool.purge();
    const memory_pool::statistics purged = pool.stats();
    EXPECT_EQ(purged.classes[class_index].empty_spans, 0U);
    EXPECT_EQ(purged.classes[class_index].span_count, 0U);
    EXPECT_EQ(purged.cached_empty_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolReallocateExtended, GrowsAcrossTheSmallLargeBoundary) {
    memory_pool pool;
    void* block = pool.allocate(64);
    ASSERT_NE(block, nullptr);
    fill_pattern(block, 64, 0x11);
    void* grown = pool.reallocate(block, 40000);
    ASSERT_NE(grown, nullptr);
    EXPECT_TRUE(check_pattern(grown, 64, 0x11));
    EXPECT_GE(pool.usable_size(grown), 40000U);
    EXPECT_TRUE(pool.owns(grown));
    void* shrunk = pool.reallocate(grown, 32);
    ASSERT_NE(shrunk, nullptr);
    EXPECT_TRUE(check_pattern(shrunk, 32, 0x11));
    EXPECT_GE(pool.usable_size(shrunk), 32U);
    pool.deallocate(shrunk, 32);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolReallocateExtended, AlignmentIsEnforcedOnReallocation) {
    memory_pool pool;
    void* block = pool.allocate(100);
    ASSERT_NE(block, nullptr);
    fill_pattern(block, 100, 0x22);
    void* aligned = pool.reallocate(block, 100, 256);
    ASSERT_NE(aligned, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % 256U, 0U);
    EXPECT_TRUE(check_pattern(aligned, 100, 0x22));
    EXPECT_GE(pool.usable_size(aligned), 100U);
    pool.deallocate(aligned, 100);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolReallocateExtended, ShrinkingLargeBlocksKeepsThePointer) {
    memory_pool pool;
    void* block = pool.allocate(120000);
    ASSERT_NE(block, nullptr);
    fill_pattern(block, 120000, 0x44);
    void* shrunk = pool.reallocate(block, 60000);
    EXPECT_EQ(shrunk, block);
    EXPECT_TRUE(check_pattern(shrunk, 60000, 0x44));
    EXPECT_GE(pool.usable_size(shrunk), 60000U);
    pool.deallocate(shrunk, 60000);
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolReallocateExtended, SameSizeKeepsTheBlock) {
    memory_pool pool;
    for (size_t size = 1; size <= 20000; size = size * 2 + 1) {
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr) << "size = " << size;
        fill_pattern(block, size, 0x55);
        void* same = pool.reallocate(block, size);
        EXPECT_EQ(same, block) << "size = " << size;
        EXPECT_TRUE(check_pattern(same, size, 0x55));
        pool.deallocate(same, size);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolReallocateExtended, NullPointerAllocatesFresh) {
    memory_pool pool;
    void* block = pool.reallocate(nullptr, 777);
    ASSERT_NE(block, nullptr);
    EXPECT_GE(pool.usable_size(block), 777U);
    fill_pattern(block, 777, 0x66);
    EXPECT_TRUE(check_pattern(block, 777, 0x66));
    void* grown = pool.reallocate(block, 5000);
    ASSERT_NE(grown, nullptr);
    EXPECT_TRUE(check_pattern(grown, 777, 0x66));
    EXPECT_GE(pool.usable_size(grown), 5000U);
    void* shrunk = pool.reallocate(grown, 0);
    EXPECT_EQ(shrunk, grown);
    EXPECT_GE(pool.usable_size(shrunk), 1U);
    pool.deallocate(shrunk, 0);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolOwnershipExtended, CrtPointersAreRejected) {
    memory_pool pool;
    void* block = ::operator new(256);
    ASSERT_NE(block, nullptr);
    EXPECT_FALSE(pool.owns(block));
    EXPECT_EQ(pool.usable_size(block), 0U);
    ::operator delete(block);
    EXPECT_EQ(pool.usable_size(nullptr), 0U);
    EXPECT_FALSE(pool.owns(nullptr));
}

TEST(MemoryPoolOwnershipExtended, CrossPoolBlocksAreOwnedByExactlyOnePool) {
    memory_pool first;
    memory_pool second;
    void* a = first.allocate(300);
    void* b = second.allocate(300);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_NE(a, b);
    EXPECT_TRUE(first.owns(a));
    EXPECT_FALSE(first.owns(b));
    EXPECT_TRUE(second.owns(b));
    EXPECT_FALSE(second.owns(a));
    EXPECT_EQ(first.usable_size(b), 0U);
    EXPECT_EQ(second.usable_size(a), 0U);
    second.deallocate(b, 300);
    first.deallocate(a, 300);
    first.flush_thread_cache();
    second.flush_thread_cache();
    EXPECT_EQ(first.stats().active_bytes, 0U);
    EXPECT_EQ(second.stats().active_bytes, 0U);
    EXPECT_TRUE(first.verify());
    EXPECT_TRUE(second.verify());
}

TEST(MemoryPoolOwnershipExtended, NullAndStackPointersAreNeverOwned) {
    memory_pool pool;
    int local = 0;
    EXPECT_FALSE(pool.owns(nullptr));
    EXPECT_FALSE(pool.owns(&local));
    EXPECT_EQ(pool.usable_size(nullptr), 0U);
    EXPECT_EQ(pool.usable_size(&local), 0U);
    void* block = pool.allocate(64);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(pool.owns(block));
    EXPECT_FALSE(pool.owns(&pool));
    pool.deallocate(block, 64);
    pool.flush_thread_cache();
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolConcurrencyExtended, ThreadsWithDistinctSizeBandsStayIsolated) {
    memory_pool pool;
    constexpr size_t thread_count = 4;
    atomic<size_t> failures{0};
    vector<thread> workers;
    for (size_t index = 0; index < thread_count; ++index) {
        workers.emplace_back([&pool, &failures, index]() {
            const size_t base = index * 4096 + 1;
            vector<pair<void*, size_t>> live;
            for (size_t round = 0; round < 6000; ++round) {
                const size_t size = base + (round % 512);
                const auto seed = static_cast<unsigned char>(index + 1);
                void* block = pool.allocate(size);
                if (block == nullptr || !pool.owns(block)) {
                    ++failures;
                    continue;
                }
                fill_pattern(block, size, seed);
                if (!check_pattern(block, size, seed)) {
                    ++failures;
                }
                if ((round & 3U) == 0U && !live.empty()) {
                    const size_t victim = round % live.size();
                    const auto entry = live[victim];
                    if (!check_pattern(entry.first, entry.second, seed)) {
                        ++failures;
                    }
                    pool.deallocate(entry.first, entry.second);
                    live[victim] = make_pair(block, size);
                } else {
                    live.emplace_back(block, size);
                }
            }
            const auto seed = static_cast<unsigned char>(index + 1);
            for (const auto& entry: live) {
                if (!check_pattern(entry.first, entry.second, seed)) {
                    ++failures;
                }
                pool.deallocate(entry.first, entry.second);
            }
        });
    }
    for (auto& worker: workers) {
        worker.join();
    }
    EXPECT_EQ(failures.load(), 0U);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolConcurrencyExtended, TwoPoolsUsedByDifferentThreadGroups) {
    memory_pool first;
    memory_pool second;
    atomic<size_t> failures{0};
    vector<thread> workers;
    for (size_t group = 0; group < 2; ++group) {
        memory_pool* pool = group == 0 ? &first : &second;
        for (size_t index = 0; index < 2; ++index) {
            workers.emplace_back([pool, &failures]() {
                for (size_t round = 0; round < 3000; ++round) {
                    const size_t size = (round % 700) + 1;
                    void* block = pool->allocate(size);
                    if (block == nullptr || !pool->owns(block)) {
                        ++failures;
                        continue;
                    }
                    fill_pattern(block, size, 0x7E);
                    if (!check_pattern(block, size, 0x7E)) {
                        ++failures;
                    }
                    pool->deallocate(block, size);
                }
            });
        }
    }
    for (auto& worker: workers) {
        worker.join();
    }
    EXPECT_EQ(failures.load(), 0U);
    first.flush_thread_cache();
    second.flush_thread_cache();
    EXPECT_EQ(first.stats().active_bytes, 0U);
    EXPECT_EQ(second.stats().active_bytes, 0U);
    EXPECT_TRUE(first.verify());
    EXPECT_TRUE(second.verify());
}

TEST(MemoryPoolConcurrencyExtended, ThreadExitFlushesCachedLargeRegions) {
    memory_pool pool;
    atomic<size_t> failures{0};
    thread worker([&pool, &failures]() {
        for (size_t round = 0; round < 128; ++round) {
            void* block = pool.allocate(64U << 10);
            if (block == nullptr) {
                ++failures;
                continue;
            }
            fill_pattern(block, 4096, 0x66);
            if (!check_pattern(block, 4096, 0x66)) {
                ++failures;
            }
            pool.deallocate(block, 64U << 10);
        }
    });
    worker.join();
    EXPECT_EQ(failures.load(), 0U);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().large_mapped_bytes, 0U);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolRobustnessExtended, BlocksSpanningSeveralSpansStayIndependent) {
    memory_pool pool;
    const size_t class_index = memory_pool::size_to_class(64);
    constexpr size_t count = 20000;
    vector<void*> blocks;
    for (size_t index = 0; index < count; ++index) {
        void* block = pool.allocate(64);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, 64, static_cast<unsigned char>(index % 251));
        blocks.push_back(block);
    }
    EXPECT_GT(pool.stats().classes[class_index].span_count, 1U);
    for (size_t index = 0; index < count; ++index) {
        ASSERT_TRUE(check_pattern(blocks[index], 64, static_cast<unsigned char>(index % 251)));
    }
    for (size_t index = 0; index < count; index += 2) {
        pool.deallocate(blocks[index], 64);
    }
    for (size_t index = 1; index < count; index += 2) {
        ASSERT_TRUE(check_pattern(blocks[index], 64, static_cast<unsigned char>(index % 251)));
    }
    for (size_t index = 1; index < count; index += 2) {
        pool.deallocate(blocks[index], 64);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolRobustnessExtended, MixedSmallAndLargeChurnStaysConsistent) {
    memory_pool pool;
    vector<pair<void*, size_t>> small_live;
    vector<pair<void*, size_t>> large_live;
    for (size_t round = 0; round < 600; ++round) {
        const size_t small_size = (round % 1500) + 1;
        void* small_block = pool.allocate(small_size);
        ASSERT_NE(small_block, nullptr);
        fill_pattern(small_block, small_size, 0x81);
        const size_t large_size = 20000 + (round % 32) * 4096;
        void* large_block = pool.allocate(large_size);
        ASSERT_NE(large_block, nullptr);
        fill_pattern(large_block, large_size, 0x82);
        if (small_live.size() >= 8) {
            const size_t victim = round % small_live.size();
            pool.deallocate(small_live[victim].first, small_live[victim].second);
            small_live[victim] = make_pair(small_block, small_size);
        } else {
            small_live.emplace_back(small_block, small_size);
        }
        if (large_live.size() >= 4) {
            const size_t victim = round % large_live.size();
            pool.deallocate(large_live[victim].first, large_live[victim].second);
            large_live[victim] = make_pair(large_block, large_size);
        } else {
            large_live.emplace_back(large_block, large_size);
        }
    }
    for (const auto& entry: small_live) {
        ASSERT_TRUE(check_pattern(entry.first, entry.second, 0x81));
        pool.deallocate(entry.first, entry.second);
    }
    for (const auto& entry: large_live) {
        ASSERT_TRUE(check_pattern(entry.first, entry.second, 0x82));
        pool.deallocate(entry.first, entry.second);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolRobustnessExtended, LongLivedBlocksSurviveInterleavedChurn) {
    memory_pool pool;
    vector<pair<void*, size_t>> residents;
    for (size_t index = 0; index < 400; ++index) {
        const size_t size = (index % 400) * 37 + 1;
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, size, 0x91);
        residents.emplace_back(block, size);
    }
    for (size_t round = 0; round < 8000; ++round) {
        const size_t size = (round % 700) + 1;
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, size, 0x92);
        EXPECT_TRUE(check_pattern(block, size, 0x92));
        pool.deallocate(block, size);
    }
    for (const auto& entry: residents) {
        ASSERT_TRUE(check_pattern(entry.first, entry.second, 0x91));
    }
    for (const auto& entry: residents) {
        pool.deallocate(entry.first, entry.second);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolRobustnessExtended, RepeatedPurgeAndFlushAreIdempotent) {
    memory_pool pool;
    for (size_t round = 0; round < 3; ++round) {
        vector<void*> blocks;
        for (size_t index = 0; index < 1500; ++index) {
            void* block = pool.allocate((index % 40) * 100 + 1);
            ASSERT_NE(block, nullptr);
            blocks.push_back(block);
        }
        for (void* block: blocks) {
            pool.deallocate(block);
        }
        pool.flush_thread_cache();
        pool.purge();
        const memory_pool::statistics first = pool.stats();
        pool.flush_thread_cache();
        pool.purge();
        const memory_pool::statistics second = pool.stats();
        EXPECT_EQ(first.mapped_bytes, second.mapped_bytes);
        EXPECT_EQ(first.active_bytes, second.active_bytes);
        EXPECT_EQ(second.mapped_bytes, 0U);
        EXPECT_EQ(second.active_bytes, 0U);
        EXPECT_TRUE(pool.verify());
    }
}

TEST(PoolAllocatorExtended, ServesProjectContainersInEveryShape) {
    memory_pool pool;
    {
        vector<int, pool_allocator<int>> values{pool_allocator<int>(pool)};
        for (int index = 0; index < 5000; ++index) {
            values.push_back(index);
        }
        ASSERT_EQ(values.size(), 5000U);
        EXPECT_EQ(values[4999], 4999);
    }
    {
        list<int, pool_allocator<list_node<int>>> items{pool_allocator<list_node<int>>(pool)};
        for (int index = 0; index < 1500; ++index) {
            items.push_back(index * 2);
        }
        EXPECT_EQ(items.size(), 1500U);
        EXPECT_EQ(items.front(), 0);
        EXPECT_EQ(items.back(), 2998);
    }
    {
        map<int, int, less<int>, pool_allocator<rb_tree_node<pair<const int, int>>>> table{
                less<int>(), pool_allocator<rb_tree_node<pair<const int, int>>>(pool)};
        for (int index = 0; index < 1000; ++index) {
            table[index] = index * 3;
        }
        EXPECT_EQ(table.size(), 1000U);
        EXPECT_EQ(table[999], 2997);
    }
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(PoolAllocatorExtended, ReboundAllocatorKeepsTheSamePool) {
    memory_pool pool;
    pool_allocator<int> allocator(pool);
    pool_allocator<long>::rebind<double>::other rebound(allocator);
    EXPECT_EQ(rebound.pool(), &pool);
    EXPECT_TRUE(allocator == rebound);
    double* block = rebound.allocate(128);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(pool.owns(block));
    for (size_t index = 0; index < 128; ++index) {
        block[index] = static_cast<double>(index);
    }
    EXPECT_DOUBLE_EQ(block[127], 127.0);
    rebound.deallocate(block, 128);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(PoolAllocatorExtended, DefaultAllocatorBindsTheSystemPool) {
    pool_allocator<int> allocator;
    EXPECT_EQ(allocator.pool(), &system_memory_pool());
    EXPECT_EQ(allocator.max_size(), static_cast<size_t>(-1) / sizeof(int));
    int* block = allocator.allocate(64);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(system_memory_pool().owns(block));
    EXPECT_GE(system_memory_pool().usable_size(block), 64U * sizeof(int));
    allocator.deallocate(block, 64);
    system_memory_pool().flush_thread_cache();
    EXPECT_TRUE(system_memory_pool().verify());
}

TEST(MemoryPoolSystemExtended, ServesContainersAndReclaimsMemory) {
    memory_pool& pool = system_memory_pool();
    EXPECT_EQ(&pool, &system_memory_pool());
    EXPECT_NE(pool.id(), 0U);
    {
        vector<int, pool_allocator<int>> values{pool_allocator<int>(pool)};
        for (int index = 0; index < 20000; ++index) {
            values.push_back(index);
        }
        EXPECT_EQ(values.size(), 20000U);
        EXPECT_EQ(values.back(), 19999);
    }
    pool.flush_thread_cache();
    EXPECT_TRUE(pool.verify());
    pool.purge();
    EXPECT_EQ(pool.stats().cached_empty_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolSystemExtended, OptionsAreCopiedIntoTheConfig) {
    memory_pool::options opts;
    opts.thread_cache_max = 16;
    opts.thread_cache_batch = 48;
    opts.thread_cache_bytes = 2048;
    opts.thread_cache_max_bytes = 512U << 10;
    opts.max_empty_spans = 3;
    opts.max_empty_span_bytes = 1U << 20;
    opts.max_cached_regions = 64;
    opts.thread_region_bytes = 128U << 10;
    opts.thread_cache_enabled = false;
    opts.region_cache_enabled = false;
    opts.purge_on_empty = true;
    memory_pool pool(opts);
    const memory_pool::options applied = pool.config();
    EXPECT_EQ(applied.thread_cache_bytes, 2048U);
    EXPECT_EQ(applied.thread_cache_batch, 48U);
    EXPECT_EQ(applied.thread_cache_max, 48U);
    EXPECT_EQ(applied.thread_cache_max_bytes, 512U << 10);
    EXPECT_EQ(applied.max_empty_spans, 3U);
    EXPECT_EQ(applied.max_empty_span_bytes, 1U << 20);
    EXPECT_LE(applied.max_cached_regions, memory_pool::region_cache_slots - 1);
    EXPECT_EQ(applied.thread_region_bytes, 128U << 10);
    EXPECT_EQ(applied.thread_cache_enabled, false);
    EXPECT_EQ(applied.region_cache_enabled, false);
    EXPECT_EQ(applied.purge_on_empty, true);
    EXPECT_NE(pool.id(), 0U);
    void* block = pool.allocate(256);
    ASSERT_NE(block, nullptr);
    pool.deallocate(block, 256);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, PayloadSizedRegionsStayThreadLocal) {
    memory_pool pool;
    void* warmup = pool.allocate(256U << 10);
    ASSERT_NE(warmup, nullptr);
    pool.deallocate(warmup, 256U << 10);
    const size_t map_calls = pool.stats().os_map_calls;
    const size_t unmap_calls = pool.stats().os_unmap_calls;
    for (int round = 0; round < 64; ++round) {
        void* block = pool.allocate(256U << 10);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, 4096, 0x71);
        EXPECT_TRUE(check_pattern(block, 4096, 0x71));
        pool.deallocate(block, 256U << 10);
    }
    EXPECT_EQ(pool.stats().os_map_calls, map_calls);
    EXPECT_EQ(pool.stats().os_unmap_calls, unmap_calls);
    EXPECT_GT(pool.stats().cached_region_bytes, 0U);
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, ThreadRegionBudgetBoundsRetention) {
    memory_pool::options opts;
    opts.thread_region_bytes = 256U << 10;
    opts.thread_region_budget_bytes = 320U << 10;
    opts.max_cached_regions = 0;
    memory_pool pool(opts);
    const size_t sizes[] = {200U << 10, 250U << 10, 220U << 10, 250U << 10};
    for (const size_t size: sizes) {
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, 8192, 0x72);
        EXPECT_TRUE(check_pattern(block, 8192, 0x72));
        EXPECT_GE(pool.usable_size(block), size);
        pool.deallocate(block, size);
        EXPECT_LE(pool.stats().cached_region_bytes, opts.thread_region_budget_bytes);
    }
    pool.flush_thread_cache();
    pool.purge();
    EXPECT_EQ(pool.stats().cached_region_bytes, 0U);
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolLargeAllocationExtended, LargeRegionsOfDistinctSizesCoexistPerThread) {
    memory_pool pool;
    vector<pair<void*, size_t>> live;
    const size_t sizes[] = {20000, 70000, 200000};
    for (const size_t size: sizes) {
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, size, 0x73);
        live.emplace_back(block, size);
    }
    for (const auto& entry: live) {
        EXPECT_TRUE(check_pattern(entry.first, entry.second, 0x73));
    }
    for (const auto& entry: live) {
        pool.deallocate(entry.first, entry.second);
    }
    const size_t map_calls = pool.stats().os_map_calls;
    for (int round = 0; round < 32; ++round) {
        for (const auto& entry: live) {
            void* block = pool.allocate(entry.second);
            ASSERT_NE(block, nullptr);
            EXPECT_TRUE(check_pattern(block, 1024, 0x73));
            pool.deallocate(block, entry.second);
        }
    }
    EXPECT_EQ(pool.stats().os_map_calls, map_calls);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolSystemExtended, PoolIdentifiersStayDistinctAndUsable) {
    const uint32_t system_id = system_memory_pool().id();
    EXPECT_NE(system_id, 0U);
    vector<memory_pool*> live;
    for (size_t index = 0; index < 200; ++index) {
        auto* pool = new memory_pool();
        ASSERT_NE(pool, nullptr);
        EXPECT_NE(pool->id(), 0U);
        EXPECT_NE(pool->id(), system_id);
        for (const memory_pool* other: live) {
            EXPECT_NE(other->id(), pool->id());
        }
        void* block = pool->allocate(48);
        ASSERT_NE(block, nullptr);
        EXPECT_TRUE(pool->owns(block));
        pool->deallocate(block, 48);
        pool->flush_thread_cache();
        live.push_back(pool);
    }
    for (memory_pool* pool: live) {
        pool->purge();
        EXPECT_TRUE(pool->verify());
        delete pool;
    }
    for (int round = 0; round < 500; ++round) {
        memory_pool pool;
        EXPECT_NE(pool.id(), system_id);
        void* block = pool.allocate(128);
        ASSERT_NE(block, nullptr);
        pool.deallocate(block, 128);
        pool.flush_thread_cache();
    }
    EXPECT_TRUE(system_memory_pool().verify());
}

TEST(MemoryPoolLargeAllocationExtended, DistinctLargeSizesStayCachedPerThread) {
    memory_pool pool;
    const size_t sizes[] = {20000, 100000, 200000, 300000};
    for (const size_t size: sizes) {
        void* block = pool.allocate(size);
        ASSERT_NE(block, nullptr);
        fill_pattern(block, 4096, 0x74);
        pool.deallocate(block, size);
    }
    EXPECT_GT(pool.stats().cached_region_bytes, 0U);
    const size_t map_calls = pool.stats().os_map_calls;
    const size_t unmap_calls = pool.stats().os_unmap_calls;
    for (int round = 0; round < 32; ++round) {
        for (const size_t size: sizes) {
            void* block = pool.allocate(size);
            ASSERT_NE(block, nullptr);
            EXPECT_TRUE(check_pattern(block, 4096, 0x74));
            EXPECT_GE(pool.usable_size(block), size);
            pool.deallocate(block, size);
        }
    }
    EXPECT_EQ(pool.stats().os_map_calls, map_calls);
    EXPECT_EQ(pool.stats().os_unmap_calls, unmap_calls);
    pool.flush_thread_cache();
    EXPECT_EQ(pool.stats().active_bytes, 0U);
    pool.purge();
    EXPECT_EQ(pool.stats().mapped_bytes, 0U);
    EXPECT_TRUE(pool.verify());
}

TEST(MemoryPoolOwnershipExtended, CrossPoolReleaseIsNotShortCircuited) {
    memory_pool first;
    memory_pool second;
    void* block = first.allocate(128);
    ASSERT_NE(block, nullptr);
    fill_pattern(block, 128, 0x75);
    const size_t foreign_before = second.foreign_release_count();
    second.deallocate(block, 128);
    EXPECT_EQ(second.foreign_release_count(), foreign_before);
    EXPECT_TRUE(first.owns(block));
    EXPECT_EQ(second.usable_size(block), 0U);
    void* again = first.allocate(128);
    ASSERT_NE(again, nullptr);
    EXPECT_TRUE(first.owns(again));
    fill_pattern(again, 128, 0x76);
    EXPECT_TRUE(check_pattern(again, 128, 0x76));
    first.deallocate(again, 128);
    first.flush_thread_cache();
    second.flush_thread_cache();
    EXPECT_EQ(first.stats().active_bytes, 0U);
    EXPECT_TRUE(first.verify());
    EXPECT_TRUE(second.verify());
}
