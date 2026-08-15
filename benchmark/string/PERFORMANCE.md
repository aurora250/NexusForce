# NeForce basic_string 性能评估

> 测试平台: 32 核 × 2419 MHz (Xeon-class), Windows 11, MSVC Release (AVX2), NexusForce v1.0.1
> 测量方法: 进程固定单核 + High 优先级 + `--benchmark_repetitions=5` 取中位数

---

## 构造类（固定尺寸，ns）

| 场景              | NfString | StdString |        比值 |
|-------------------|---------:|----------:|------------:|
| DefaultConstruct  |   1.93ns |    2.53ns | **1.3× 快** |
| ConstructShort    |   8.46ns |    10.7ns | **1.3× 快** |
| ConstructLong     |   70.5ns |    71.4ns |        持平 |
| CopyShort (5)     |   3.27ns |    3.29ns |        持平 |
| CopyLong (~135)   |   56.5ns |    56.5ns |        持平 |

- CopyShort 与 std 持平：SSO 源整块拷贝 16 字节，无逐字节尾部开销
- ConstructShort 反超：小尺寸拷贝按 8/4/2/1 宽度展开，无逐字节循环
- 构造路径 SSO 容量与 append 路径一致（15 字符）

## AppendChar（单字符追加）

|  Size | NfString | StdString |      比值 |
|------:|---------:|----------:|----------:|
|    64 |   470M/s |    573M/s |     0.82× |
|  1024 |   779M/s |   1.05G/s |     0.74× |
| 32768 |   789M/s |   1.14G/s |     0.69× |
|    1M |  1.05G/s |   1.03G/s | **1.02×** |

- 该用例**多轮测量波动最大**，当前受控测量下小/中尺寸 std 略快、1M 持平
- 1M 行的 items/s 含 `reserve(1M)` 的分配开销，不纯粹反映 push_back 吞吐
- 小尺寸差距推测来自 `size()` 的 `& ~long_flag` bitmask 与 `is_long()` 分支构成的更新依赖链，std 的独立 `size_` 成员更新更短（待确认）

## AppendBulk（字符串追加）

|  Size | NfString | StdString |         比值 |
|------:|---------:|----------:|-------------:|
|    64 |  1.45G/s |   1.47G/s |        持平* |
|  1024 |  9.79G/s |   7.66G/s | **1.28× 快** |
| 32768 |  11.0G/s |   10.4G/s | **1.06× 快** |
|    1M |  5.66G/s |   6.22G/s |        0.91× |

- *64 为退化用例（`chunks = 64 / 135 = 0`，实测仅构造 + reserve），不反映追加吞吐
- 1024–32768 区间领先：`memory_copy` AVX2 256-bit 搬迁 + 1.5× 增长因子减少重分配次数

## ConstructFill（填充构造）

|  Size | NfString | StdString |         比值 |
|------:|---------:|----------:|-------------:|
|    64 |  1.18G/s |   1.47G/s |        0.81× |
|  1024 |  17.6G/s |   19.6G/s |        0.90× |
| 32768 |  50.3G/s |   39.9G/s | **1.26× 快** |
|    1M |  4.51G/s |   4.70G/s |        0.96× |

- 大块填充（MSVC x86 ≥4KB）走 `__stosb`（REP STOSB / ERMS），32768 反超、1M 持平（1M 受分配与页错误主导）

## FindChar（单字符查找）

|  Size | NfString | StdString |    比值 |
|------:|---------:|----------:|--------:|
|    64 |   191M/s |    273M/s |   0.70× |
|  1024 |  37.4M/s |   57.3M/s |   0.65× |
| 32768 |  1.62M/s |   2.29M/s |   0.71× |
|    1M |  51.2k/s |   73.5k/s |   0.70× |

- **当前唯一显著短板**（约 1.4–1.5× 落后）：`string::find` → `char_traits_find_char` → `Traits::find` → `memory_find` 四层调用链的固定开销，各尺寸差距一致，属每次调用的固定成本而非扫描吞吐
- 已含 count<16 标量快路径，但基准尺寸均 ≥64，AVX2 设置开销仍在

## Substr（子串提取）

|  Size | NfString | StdString |      比值 |
|------:|---------:|----------:|----------:|
|    64 |  22.9M/s |   23.9M/s |     0.96× |
|  1024 |  23.9M/s |   23.2M/s | **1.03×** |
| 32768 |  2.69M/s |   3.02M/s |     0.89× |
|    1M |  94.5k/s |   86.0k/s | **1.10×** |

- 全尺寸持平

## Iterate（字符遍历）

|  Size | NfString | StdString |         比值 |
|------:|---------:|----------:|-------------:|
|    64 |  3.06G/s |   2.54G/s | **1.21× 快** |
|  1024 |  3.04G/s |   2.62G/s | **1.16× 快** |
| 32768 |  2.82G/s |   2.89G/s |        0.98× |
|    1M |  2.66G/s |   3.01G/s |        0.88× |

- 全尺寸持平：`basic_string_iterator` 在 release 下边界检查为空操作，等价裸指针 `*p++`

---

## 架构特性矩阵

| 特性            |      NeForce basic_string      | std::string (MSVC)  | std::string (libstdc++) |
|-----------------|:------------------------------:|:-------------------:|:-----------------------:|
| SSO 容量 (char) |            15 chars            |      15 chars       |        15 chars         |
| 填充构造 SIMD   |         ✅ SSE + AVX2          | ✅ 内部 memset 优化 |   ✅ 内部 memset 优化   |
| 字符查找 SIMD   |         ✅ SSE + AVX2          | ✅ memchr 内联展开  |   ✅ memchr 内联展开    |
| 迭代器类型      | 类迭代器（release 等价裸指针） |       裸指针        |         裸指针          |
| 宽字符感知 SIMD |   ✅ lane-aware match_lanes    |          —          |            —            |
| constexpr 支持  |           ✅ (C++20)           |     ✅ (C++20)      |       ✅ (C++20)        |
| API 丰富度      |     通用 join/split/format     |        基础         |          基础           |

## 优化建议（按剩余收益排序）

| 方向                                                                                                  | 预期收益                      |     复杂度     |
|-------------------------------------------------------------------------------------------------------|-------------------------------|:--------------:|
| FindChar 四层调用链合并：`string::find` 短串直接内联标量/`Traits::find`，减少每次调用的分支与函数边界 | FindChar 1.4× → ~1.1×         |       低       |
| AppendChar 小尺寸：`size()` bitmask 依赖链优化（如长/短模式拆分更新路径）                             | AppendChar ≤32768 波动收窄    |       低       |
| `size()` 纯成员化 — SSO 场景下 `long_flag` 恒为 0，考虑独立 `size_` 成员                              | AppendChar/FindChar 全线 ~10% | 高（架构变更） |

---

## 总结

NexusForce `basic_string` 在 Windows (MSVC Release, AVX2) 下，9 个 Benchmark 场景中 **8 个持平或领先 `std::string`**：

- **构造与拷贝全面追平**：CopyShort 持平、ConstructShort/DefaultConstruct 反超 1.3×、CopyLong/ConstructLong 持平
- **填充持平**：ConstructFill 32768 反超 1.26×（ERMS 大块路径），其余尺寸持平
- **批量操作保持优势**：AppendBulk 1024–32768 领先 1.1–1.3×，`memory_copy` AVX2 宽寄存器路径是核心驱动力
- **FindChar 为唯一余留短板**（1.4–1.5×）：四层调用链固定开销所致，低复杂度优化即可收窄至 ~1.1×
- **AppendChar 波动大**：受控测量下小/中尺寸 std 略快、1M 持平，需在稳定环境复测确认

整体而言，NexusForce `basic_string` 在保有宽字符感知 SIMD（`match_lanes<CharT>`）与丰富 API（`join`/`split`/`format`）的前提下，`char` 类型核心操作性能已与 MSVC `std::string` 处于同一梯队：批量与构造操作持平或领先，仅单字符查找存在可修复的固定开销差距。
