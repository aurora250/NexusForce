#ifndef NEFORCE_CORE_NUMERIC_RANDOM_HPP__
#define NEFORCE_CORE_NUMERIC_RANDOM_HPP__

/**
 * @file random.hpp
 * @brief 随机数生成器
 *
 * 此文件是随机数组件的聚合，包含全部随机数引擎、位生成器适配器与随机数分布函数。
 * 组件按类别拆分在 random/ 子目录中：
 * - random/engine.hpp：引擎公共基类、无偏映射、均匀位提取与种子设施
 * - random/lcg.hpp、random/mt.hpp、random/pcg.hpp、random/xoroshiro.hpp、random/secret.hpp：各类随机数引擎
 * - random/bit_gen.hpp：指定位宽的位生成器适配器
 * - random/distribution.hpp：随机数分布函数
 */

#include "NeForce/core/numeric/random/bit_generator.hpp"
#include "NeForce/core/numeric/random/distribution.hpp"
#include "NeForce/core/numeric/random/engine.hpp"
#include "NeForce/core/numeric/random/lcg.hpp"
#include "NeForce/core/numeric/random/mt.hpp"
#include "NeForce/core/numeric/random/pcg.hpp"
#include "NeForce/core/numeric/random/secret.hpp"
#include "NeForce/core/numeric/random/xoroshiro.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup RandomGenerators 随机数生成器
 * @brief 线性同余法、梅森旋转算法、置换同余法、异或移位旋转算法、位宽适配与硬件真随机数生成器实现
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下密码学与随机数生成相关标准规范：
 *
 * **伪随机数生成器 (PRNG) 标准：**
 * - ISO/IEC 18031:2011：信息技术 — 安全技术 — 随机比特生成
 *   https://www.iso.org/standard/54945.html
 * - NIST SP 800-90A Rev. 1：确定性随机比特生成器的建议
 *   https://csrc.nist.gov/pubs/sp/800/90/a/r1/final
 *
 * **真随机数生成器 (TRNG) 标准：**
 * - ISO/IEC 20543:2019：信息技术 — 安全技术 — 熵源评估与验证
 *   https://www.iso.org/standard/68338.html
 * - NIST SP 800-90B：熵源验证建议
 *   https://csrc.nist.gov/pubs/sp/800/90/b/final
 *
 * **算法规范参考：**
 * - 线性同余生成器 (LCG)：遵循 POSIX.1-2001 rand() 规范
 *   https://pubs.opengroup.org/onlinepubs/009695399/functions/rand.html
 * - 梅森旋转算法 (MT19937)：遵循松本真、西村拓士 1998 年原始论文
 *   http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/ARTICLES/mt.pdf
 * - 无偏整数映射算法：Daniel Lemire, 2019
 *   https://arxiv.org/abs/1805.10941
 * - 置换同余生成器 (PCG)：Melissa E. O'Neill, 2014
 *   https://www.pcg-random.org/pdf/hmc-cs-2014-0905.pdf
 * - 异或移位旋转生成器 (xoroshiro/xoshiro)：David Blackman, Sebastiano Vigna, 2018
 *   https://arxiv.org/abs/1805.01407
 * - SplitMix 种子扩展：Sebastiano Vigna, 2014
 *   https://arxiv.org/abs/1404.0390
 * - 泊松分布 PTRS 算法：Wolfgang Hörmann, 1993, Insurance: Mathematics and Economics 12(1)
 *
 * @section implementation_details 生成器实现细节
 * | 生成器              | 算法                     | 周期长度       | 适用场景              |
 * |---------------------|--------------------------|----------------|-----------------------|
 * | random_lcd          | 线性同余法 (glibc 兼容)  | 2^31           | 简单模拟、非安全场景  |
 * | random_mt           | MT19937 梅森旋转         | 2^19937 - 1    | 科学计算、统计分析    |
 * | random_pcg32        | PCG XSH-RR 64/32         | 2^64           | 高性能模拟、并行流    |
 * | random_pcg64        | PCG XSL-RR 128/64        | 2^128          | 大规模蒙特卡洛        |
 * | random_xoroshiro128 | 异或移位旋转 2×64 位状态 | 2^128 - 1      | 高性能模拟、并行流    |
 * | random_xoroshiro256 | 异或移位旋转 4×64 位状态 | 2^256 - 1      | 超长周期模拟          |
 * | bit_gen             | 位宽适配 (1-64 位)       | 取决于底层引擎 | 非 2 的幂次位宽随机数 |
 * | secret              | 操作系统熵源             | 不可预测       | 加密密钥、安全令牌    |
 *
 * @section distribution_details 分布函数
 * | 分布函数     | 分布         | 说明                                   |
 * |--------------|--------------|----------------------------------------|
 * | uniform_int  | 离散均匀分布 | [min, max) 上的均匀整数                |
 * | uniform_real | 连续均匀分布 | [0, 1) 或 [min, max) 上的均匀浮点数     |
 * | bernoulli    | 伯努利分布   | 以给定概率返回 true                    |
 * | normal       | 正态分布     | 可指定均值与标准差，无样本缓存         |
 * | exponential  | 指数分布     | 可指定速率参数                         |
 * | log_uniform  | 对数均匀分布 | 区间端点必须为正数                     |
 * | poisson      | 泊松分布     | 小均值用乘积法，大均值用 PTRS 精确算法 |
 *
 * @section engine_concept 引擎接口约定
 * 所有引擎都提供符合 UniformRandomBitGenerator 的接口以及本库统一的取值接口，
 * 因此既可以配合标准分布函数使用，也可以直接使用 random/distribution.hpp 中的分布函数；
 * bit_gen 可以把任意引擎适配为指定 1-64 位字宽的随机源。
 *
 * @section next_int_algorithm next_int 无偏映射算法
 * 使用 Lemire 算法将均匀分布的 64 位随机数无偏映射到 [0, max)：
 *
 *   设 r 为 64 位随机数，计算 m = r * max（128 位中间结果）
 *   取 m 的高 64 位作为结果候选值
 *   若 m 的低 64 位 < max，则进行拒绝采样消除边界偏差
 *
 * 相比简单取模（% max），此算法：
 *   1. 避免了取模操作引入的分布偏差
 *   2. 期望拒绝次数 < 1 次，性能接近无拒绝情况
 *
 * @see https://www.iso.org/standard/54945.html
 * @see https://csrc.nist.gov/projects/random-bit-generation
 */

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_NUMERIC_RANDOM_HPP__
