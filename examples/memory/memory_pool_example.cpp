/**
 * @brief 内存池示例——高性能分配与统计
 *
 * 演示 memory_pool 的典型用法：
 * - 直接分配与释放，覆盖小对象 span 路径与大对象映射路径
 * - pool_allocator 与标准容器配合使用
 * - 线程本地缓存与 purge 对驻留内存的影响
 * - 统计信息与一致性校验
 */

#include <NeForce/core/memory/memory_pool.hpp>
#include <NeForce/core/system/console.hpp>
using namespace neforce;

namespace {
    void print_statistics(const char* title, memory_pool::statistics& stats) {
        println("{}", title);
        println("  已交付字节 {} / 累计映射 {} / 峰值映射 {}", stats.active_bytes, stats.mapped_bytes,
                stats.peak_mapped_bytes);
        println("  小对象 span {} / 大对象映射 {} / 保留区域 {} / 保留空闲 span {}", stats.small_mapped_bytes,
                stats.large_mapped_bytes, stats.cached_region_bytes, stats.cached_empty_bytes);
        println("  映射次数 {} / 归还次数 {} / 外来释放 {}", stats.os_map_calls, stats.os_unmap_calls,
                stats.foreign_releases);
    }
} // namespace


int main() {
    memory_pool pool;

    println("== 基本分配 ==");
    vector<pair<void*, size_t>> blocks;
    for (size_t size: {16U, 64U, 512U, 4096U, 20000U, 262144U}) {
        void* block = pool.allocate(size);
        memory_set(block, static_cast<int>(size & 0xFF), size);
        println("  请求 {:>7} 字节 -> 可用 {:>7} 字节，{}", size, pool.usable_size(block),
                size > memory_pool::small_max ? "大对象映射" : "尺寸类 span");
        blocks.emplace_back(block, size);
    }
    memory_pool::statistics stats = pool.stats();
    print_statistics("分配后统计：", stats);
    for (auto& entry: blocks) {
        pool.deallocate(entry.first, entry.second);
    }

    println("== 线程缓存与内存回收 ==");
    pool.flush_thread_cache();
    pool.purge();
    stats = pool.stats();
    print_statistics("归还后统计：", stats);

    println("== 与标准容器配合 ==");
    {
        vector<string, pool_allocator<string>> lines{pool_allocator<string>(pool)};
        for (int index = 0; index < 1000; ++index) {
            lines.emplace_back(128, static_cast<char>('a' + (index % 26)));
        }
        println("  容器元素 {} 个，首元素长度 {}", lines.size(), lines.front().size());
    }
    pool.flush_thread_cache();
    println("  容器释放后已交付字节 {}", pool.stats().active_bytes);

    println("== 一致性校验 ==");
    println("  内存池内部一致：{}", pool.verify() ? "是" : "否");
    return 0;
}
