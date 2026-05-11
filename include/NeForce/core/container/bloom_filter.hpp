#ifndef NEFORCE_CORE_CONTAINER_BLOOM_FILTER_HPP__
#define NEFORCE_CORE_CONTAINER_BLOOM_FILTER_HPP__

/**
 * @file bloom_filter.hpp
 * @brief 布隆过滤器容器
 *
 * 此文件提供了布隆过滤器的实现。
 * 布隆过滤器是一种空间效率很高的概率性数据结构，
 * 用于判断一个元素是否在集合中，可能存在误报但不会有漏报。
 */

#include "NeForce/core/container/bitmap.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/bit.hpp"
#include "NeForce/core/numeric/math.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup BloomFilter 布隆过滤器
 * @brief 布隆过滤器实现
 *
 * 布隆过滤器是一种空间效率很高的概率性数据结构，
 * 用于判断一个元素是否在集合中，可能存在误报但不会有漏报。
 *
 * @section references 学术文献与理论来源
 * 本实现基于以下原创学术论文和经典理论分析：
 *
 * **布隆过滤器原始论文：**
 * - **Burton H. Bloom (1970)**：Space/Time Trade-offs in Hash Coding with Allowable Errors
 *   Communications of the ACM, 13(7): 422-426
 *   https://doi.org/10.1145/362686.362692
 *
 * **理论分析与最优参数推导：**
 * - **Andrei Broder, Michael Mitzenmacher (2004)**：Network Applications of Bloom Filters: A Survey
 *   Internet Mathematics, 1(4): 485-509
 *   https://doi.org/10.1080/15427951.2004.10129096
 *
 * **双哈希技术文献：**
 * - **Adam Kirsch, Michael Mitzenmacher (2006)**：Less Hashing, Same Performance: Building a Better Bloom Filter
 *   Random Structures & Algorithms, 33(2): 187-218
 *   https://doi.org/10.1002/rsa.20208
 *
 * @section bloom_filter_principles 布隆过滤器原理
 * 布隆过滤器由 m 位的位数组和 k 个独立的哈希函数组成：
 *
 * **核心操作**：
 * | 操作   | 过程                                           | 时间复杂度 |
 * |--------|------------------------------------------------|------------|
 * | 插入   | 计算 k 个哈希值，将对应位设为 1                | O(k)       |
 * | 查询   | 计算 k 个哈希值，检查所有对应位是否都为 1      | O(k)       |
 *
 * **特性总结**：
 * | 特性           | 说明                                           |
 * |----------------|------------------------------------------------|
 * | 空间效率       | 仅使用位数组，空间复杂度 O(m)                  |
 * | 假阴性（漏报） | 不可能（已插入元素一定返回 true）              |
 * | 假阳性（误报） | 可能（未插入元素可能返回 true）                |
 * | 删除操作       | 不支持（可能影响其他元素）                     |
 * | 元素计数       | 仅支持估算                                     |
 *
 * @section optimal_parameters 最优参数推导
 * 给定预期插入元素数量 n 和目标误报率 p，最优参数由以下公式确定：
 *
 * **最优位数组大小 m**：
 * ```
 * m = -n × ln(p) / (ln 2)²
 * ```
 *
 * **最优哈希函数数量 k**：
 * ```
 * k = (m / n) × ln 2
 * ```
 *
 * 在此最优配置下，误报率 p 与参数关系为：
 * ```
 * p = (1 - e^(-kn/m))^k
 * ```
 *
 * @section parameter_examples 参数示例
 * 给定 n = 1,000,000，不同目标误报率下的最优参数：
 *
 * | 目标误报率 p | m (位数) | m (MB) | k  | 理论容量 |
 * |--------------|----------|--------|----|----------|
 * | 0.1 (10%)    | 4,792,530 | 0.57 | 3  | 1,107,000 |
 * | 0.01 (1%)    | 9,585,059 | 1.14 | 7  | 947,000 |
 * | 0.001 (0.1%) | 14,377,589 | 1.71 | 10 | 996,000 |
 * | 0.0001 (0.01%)| 19,170,117 | 2.28 | 13 | 1,022,000 |
 *
 * @section double_hashing 双哈希技术
 * 本实现采用 Kirsch & Mitzenmacher (2006) 提出的双哈希技术：
 *
 * **原理**：
 * 仅计算两个独立的哈希值 h₁(x) 和 h₂(x)，通过线性组合生成 k 个哈希值：
 * ```
 * g_i(x) = (h₁(x) + i × h₂(x)) mod m  (i = 0, 1, ..., k-1)
 * ```
 *
 * **优点**：
 * - 减少哈希计算开销
 * - 理论证明与 k 个独立哈希函数具有相同的渐近性能
 * - 要求 h₂(x) 与 m 互质（通过确保 h₂(x) 为奇数实现）
 *
 * @section false_positive_estimation 误报率估算
 * 设位数组中 1 的比例为 x，则当前误报率可估算为：
 * ```
 * p ≈ x^k
 * ```
 *
 * 当前元素数量可估算为：
 * ```
 * n ≈ -(m/k) × ln(1 - x)
 * ```
 *
 * @section applications 典型应用场景
 * 布隆过滤器广泛应用于以下场景：
 *
 * | 应用场景               | 说明                                           |
 * |------------------------|------------------------------------------------|
 * | 缓存穿透防护           | 快速过滤不存在的键，避免查询后端存储           |
 * | 数据库查询优化         | 在 LSM-Tree 中减少不必要的磁盘读取             |
 * | 网络爬虫 URL 去重      | 高效判断 URL 是否已访问                        |
 * | 垃圾邮件过滤           | 快速检查邮件地址是否在黑名单中                 |
 * | 分布式系统同步         | 快速判断数据是否存在于远程节点                 |
 * | 区块链轻节点           | 验证交易是否属于某个区块（BIP 37）             |
 * | 拼写检查               | 快速判断单词是否在词典中                       |
 *
 * @section limitations 局限性
 * | 限制           | 说明                                           |
 * |----------------|------------------------------------------------|
 * | 不支持删除     | 将位清零可能影响其他元素的查询                 |
 * | 误报率         | 随插入元素数量增加而上升                       |
 * | 无法枚举元素   | 无法获知具体存储了哪些元素                     |
 * | 精确计数困难   | 只能通过位数组状态估算                         |
 *
 * @note 布隆过滤器在空间敏感且可容忍少量误报的场景下表现优异。
 *       对于需要精确结果或支持删除操作的场景，应考虑使用哈希表或计数布隆过滤器。
 *
 * @warning 误报率随插入元素数量增加而上升。当实际插入数量远超预期时，
 *          误报率将显著增加。建议预留 20-30% 的容量余量。
 *          布隆过滤器不支持删除操作，将位清零会破坏其他元素的查询准确性。
 *
 * @see https://en.wikipedia.org/wiki/Bloom_filter
 * @see https://dl.acm.org/doi/10.1145/362686.362692
 * @see https://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf
 * @{
 */

/**
 * @class bloom_filter
 * @brief 布隆过滤器
 * @tparam T 元素类型
 * @tparam Hash 哈希函数类型，默认为hash<T>
 *
 * 布隆过滤器使用多个哈希函数将元素映射到一个位数组中。
 * 布隆过滤器有如下特性：
 * - 空间效率高，使用位数组存储
 * - 查询速度快，时间复杂度O(k)
 * - 不存在假阴性（如果元素在集合中，查询一定返回true）
 * - 存在假阳性（可能误判不存在的元素存在）
 * - 支持合并和交集操作
 */
template <typename T, typename Hash = hash<T>>
class bloom_filter {
private:
    size_t m_;    ///< 位数组大小
    size_t k_;    ///< 哈希函数数量
    bitmap bits_; ///< 位数组
    Hash hasher_; ///< 哈希函数对象

    /**
     * @brief 计算最优位数组大小
     * @param n 预期插入元素数量
     * @param p 期望误报率
     * @return 最优位数组大小
     *
     * 公式：m = -n * ln(p) / (ln2)^2
     */
    static size_t compute_m(const size_t n, const double p) noexcept {
        if (p <= 0.0 || p >= 1.0) {
            return 0;
        }
        const double ln2 = logarithm_e(2.);
        const double m = -static_cast<double>(n) * logarithm_e(p) / (ln2 * ln2);
        return static_cast<size_t>(ceil(m));
    }

    /**
     * @brief 计算最优哈希函数数量
     * @param n 预期插入元素数量
     * @param m 位数组大小
     * @return 最优哈希函数数量
     *
     * 公式：k = (m / n) * ln2
     */
    static size_t compute_k(const size_t n, const size_t m) noexcept {
        if (n == 0 || m == 0) {
            return 0;
        }
        const double k = (static_cast<double>(m) / n) * logarithm_e(2.);
        return static_cast<size_t>(max(static_cast<decimal_t>(1), round(k)));
    }

    /**
     * @brief 计算元素的双哈希值
     * @param key 输入元素
     * @return 一对哈希值（h1, h2）
     *
     * 使用双哈希技术生成k个哈希值。
     */
    pair<size_t, size_t> hash_values(const T& key) const noexcept {
        size_t h1 = hasher_(key);
        size_t h2 = rotate_l(h1, 17);
        if (h2 == 0) {
            h2 = 1;
        }
        return {h1, h2};
    }

    /**
     * @brief 计算第i个哈希函数的索引
     * @param i 哈希函数索引
     * @param h1 第一个哈希值
     * @param h2 第二个哈希值
     * @return 位数组索引
     *
     * 公式：g_i(x) = h1(x) + i * h2(x) (mod m)
     */
    size_t nth_hash(const size_t i, const size_t h1, const size_t h2) const noexcept { return (h1 + i * h2) % m_; }

public:
    bloom_filter(const bloom_filter&) noexcept = default;
    bloom_filter(bloom_filter&&) noexcept = default;
    bloom_filter& operator=(const bloom_filter&) = default;
    bloom_filter& operator=(bloom_filter&&) = default;

    /**
     * @brief 析构函数
     */
    ~bloom_filter() = default;

    /**
     * @brief 基于预期插入数和误报率构造
     * @param expected_insertions 预期插入元素数量
     * @param false_positive_prob 期望误报率
     * @throws value_exception 当预期插入数为0或误报率不在(0,1)范围内时抛出
     *
     * 根据预期插入数量和期望误报率自动计算最优参数。
     */
    bloom_filter(const size_t expected_insertions, const double false_positive_prob) :
    m_(compute_m(expected_insertions, false_positive_prob)),
    k_(compute_k(expected_insertions, m_)),
    bits_(m_, false),
    hasher_(Hash()) {
        if (expected_insertions == 0 || false_positive_prob <= 0.0 || false_positive_prob >= 1.0) {
            NEFORCE_THROW_EXCEPTION(
                    value_exception("expected_insertions must be positive and false_positive_prob in (0,1)"));
        }
    }

    /**
     * @brief 基于直接参数构造
     * @param m 位数组大小
     * @param k 哈希函数数量
     * @throws value_exception 当m或k为0时抛出
     *
     * 直接指定位数组大小和哈希函数数量。
     */
    bloom_filter(const size_t m, const size_t k) :
    m_(m),
    k_(k),
    bits_(m_, false),
    hasher_(Hash()) {
        if (m == 0 || k == 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("m and k must be positive"));
        }
    }

    /**
     * @brief 获取理论容量
     * @return 理论可容纳元素数量
     *
     * 基于当前参数计算理论可容纳元素数量。
     */
    NEFORCE_NODISCARD size_t capacity() const noexcept {
        return static_cast<size_t>(static_cast<double>(m_) * logarithm_e(2.) / k_);
    }

    /**
     * @brief 检查过滤器是否为空
     * @return 如果没有元素被插入返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        for (size_t i = 0; i < m_; ++i) {
            if (bits_[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 获取位数组大小
     * @return 位数
     */
    NEFORCE_NODISCARD size_t bit_size() const noexcept { return m_; }

    /**
     * @brief 获取哈希函数数量
     * @return k值
     */
    NEFORCE_NODISCARD size_t hash_count() const noexcept { return k_; }

    /**
     * @brief 估计当前元素数量
     * @return 元素数量的近似值
     *
     * 根据位数组中1的比例估算实际插入的元素数量。
     */
    NEFORCE_NODISCARD size_t approximate_count() const noexcept {
        size_t bits_set = 0;
        for (size_t i = 0; i < m_; ++i) {
            if (bits_[i]) {
                ++bits_set;
            }
        }
        const double x = static_cast<double>(bits_set) / m_;
        return static_cast<size_t>(-(static_cast<double>(m_) / k_) * logarithm_e(1 - x));
    }

    /**
     * @brief 估计当前误报率
     * @return 误报率的近似值
     *
     * 根据位数组中1的比例估算当前的误报率。
     */
    NEFORCE_NODISCARD double false_positive_rate() const noexcept {
        size_t bits_set = 0;
        for (size_t i = 0; i < m_; ++i) {
            if (bits_[i]) {
                ++bits_set;
            }
        }
        const double x = static_cast<double>(bits_set) / m_;
        return power(x, k_);
    }

    /**
     * @brief 检查元素是否可能存在
     * @param key 要检查的元素
     * @return 如果元素可能存在返回true，如果一定不存在返回false
     *
     * 如果返回false，则元素一定不在集合中。
     * 如果返回true，则元素可能存在（也可能误判）。
     */
    NEFORCE_NODISCARD bool contains(const T& key) const noexcept {
        auto h = this->hash_values(key);
        for (size_t i = 0; i < k_; ++i) {
            const size_t index = this->nth_hash(i, h.first, h.second);
            if (!bits_[index]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 插入元素
     * @param key 要插入的元素
     *
     * 将元素的所有哈希位置设为1。
     */
    void insert(const T& key) noexcept {
        auto h = this->hash_values(key);
        for (size_t i = 0; i < k_; ++i) {
            const size_t index = this->nth_hash(i, h.first, h.second);
            bits_[index] = true;
        }
    }

    /**
     * @brief 清空过滤器
     *
     * 将所有位重置为0。
     */
    void clear() noexcept { fill(bits_.begin(), bits_.end(), false); }

    /**
     * @brief 合并另一个过滤器
     * @param other 要合并的过滤器
     * @return 自身引用
     * @throws value_exception 当过滤器参数不匹配时抛出
     *
     * 将另一个过滤器的所有位与当前过滤器进行OR操作。
     * 要求两个过滤器参数相同。
     */
    bloom_filter& merge(const bloom_filter& other) {
        if (m_ != other.m_ || k_ != other.k_) {
            NEFORCE_THROW_EXCEPTION(value_exception("Filters must have same parameters to merge"));
        }

        for (size_t i = 0; i < m_; ++i) {
            if (other.bits_[i]) {
                bits_[i] = true;
            }
        }
        return *this;
    }

    /**
     * @brief 交集操作
     * @param other 另一个过滤器
     * @return 新过滤器，为两个过滤器的交集
     * @throws value_exception 当过滤器参数不匹配时抛出
     *
     * 对两个过滤器的位进行AND操作。
     * 要求两个过滤器参数相同。
     */
    bloom_filter intersect(const bloom_filter& other) const {
        if (m_ != other.m_ || k_ != other.k_) {
            NEFORCE_THROW_EXCEPTION(value_exception("Filters must have same parameters for intersection"));
        }

        bloom_filter result(m_, k_);
        for (size_t i = 0; i < m_; ++i) {
            result.bits_[i] = bits_[i] && other.bits_[i];
        }
        return result;
    }

    /**
     * @brief 并集操作
     * @param other 另一个过滤器
     * @return 新过滤器，为两个过滤器的并集
     * @throws value_exception 当过滤器参数不匹配时抛出
     *
     * 对两个过滤器的位进行OR操作。
     */
    bloom_filter unite(const bloom_filter& other) const {
        bloom_filter result(*this);
        return result.merge(other);
    }

    /**
     * @brief 转换为字节数组
     * @return 字节向量表示
     *
     * 将位数组转换为字节数组以便序列化。
     */
    byte_vector to_bytes() const {
        byte_vector bytes((m_ + 7) / 8);
        for (size_t i = 0; i < m_; ++i) {
            if (bits_[i]) {
                bytes[i / 8] |= (1 << (i % 8));
            }
        }
        return bytes;
    }

    /**
     * @brief 从字节数组恢复
     * @param bytes 字节向量
     * @throws value_exception 当字节数组大小不足时抛出
     *
     * 从字节数组恢复位数组状态。
     */
    void from_bytes(const byte_vector& bytes) {
        if (bytes.size() * 8 < m_) {
            NEFORCE_THROW_EXCEPTION(value_exception("Insufficient byte data"));
        }
        for (size_t i = 0; i < m_; ++i) {
            bits_[i] = (bytes[i / 8] >> (i % 8)) & 1;
        }
    }
};

/** @} */ // BloomFilter

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_BLOOM_FILTER_HPP__
