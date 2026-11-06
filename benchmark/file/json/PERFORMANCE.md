# NeForce JSON 性能评估

> 被测组件: `neforce::json_parser` / `neforce::json_value` / `neforce::json_builder`（`include/NeForce/core/file/json`）
> 测试条目: 解析、optional 解析、紧凑序列化、缩进序列化、解析+序列化往返、`json_builder` 构建、宽对象成员访问、成员字符串提取
> 单位: **ns/op 或 µs/op（单次操作耗时，越小越好）**，吞吐为单一核心上的 MB/s（1 MB = 1024×1024 字节）

---

## 测试环境

| 项目 | 配置 |
|------|------|
| CPU | i9-13980HX（8 P-core + 16 E-core = 32 逻辑核） |
| 绑定核心 | `taskset -c 2`（单个 P-core） |
| 操作系统 | Linux 7.0.0-42 |
| 编译器 | Clang 20，Release，`-O3 -DNDEBUG -mavx2` |
| 测量方法 | `--benchmark_repetitions=3`，3 次独立进程，各条目取最小值 |
| 构建目标 | `NexusForceJsonBenchmark`（`benchmark/file/json`） |

## 测试文档

| 文档 | 结构 | 大小（字节） |
|------|------|-------------:|
| `FlatObject_32` | 32 个整数成员的扁平对象 | 339 |
| `FlatObject_256` | 256 个整数成员的扁平对象 | 3,181 |
| `StringObject_64x32` | 64 个 32 字节字符串成员 | 2,807 |
| `ArrayOfObjects_256` | 256 个对象的数组（整数+字符串+小数+布尔） | 14,135 |
| `ArrayOfObjects_2048` | 2048 个对象的数组 | 118,527 |
| `Nested_8x32` | 8 层嵌套，每层 32 个对象的数组 | 771 |

## 解析性能（`json_parser::parse`）

每次迭代重新构造 `json_parser` 并解析整份文档，仅计入解析耗时。

| 文档 | 大小（字节） | 耗时 | 吞吐 |
|------|-------------:|-----:|-----:|
| `FlatObject_32` | 339 | 3.76 µs | 85.9 MB/s |
| `FlatObject_256` | 3,181 | 29.36 µs | 103.3 MB/s |
| `StringObject_64x32` | 2,807 | 8.49 µs | 315.4 MB/s |
| `ArrayOfObjects_256` | 14,135 | 119.21 µs | 113.1 MB/s |
| `ArrayOfObjects_2048` | 118,527 | 967.93 µs | 116.8 MB/s |
| `Nested_8x32` | 771 | 10.74 µs | 68.5 MB/s |

## 解析性能（`json_parser::try_parse`）

`try_parse` 以 `optional` 返回结果，异常路径不抛出，用于对照 `parse` 的额外开销。

| 文档 | 大小（字节） | 耗时 | 吞吐 |
|------|-------------:|-----:|-----:|
| `FlatObject_32` | 339 | 3.69 µs | 87.6 MB/s |
| `FlatObject_256` | 3,181 | 29.33 µs | 103.4 MB/s |
| `StringObject_64x32` | 2,807 | 8.40 µs | 318.7 MB/s |
| `ArrayOfObjects_256` | 14,135 | 118.30 µs | 113.9 MB/s |
| `ArrayOfObjects_2048` | 118,527 | 963.20 µs | 117.4 MB/s |
| `Nested_8x32` | 771 | 10.85 µs | 67.8 MB/s |

## 紧凑序列化（`json_value::to_string`）

文档在测量前解析一次，循环内只做序列化，排除解析成本。

| 文档 | 大小（字节） | 耗时 | 吞吐 |
|------|-------------:|-----:|-----:|
| `FlatObject_32` | 339 | 2.45 µs | 131.7 MB/s |
| `FlatObject_256` | 3,181 | 19.29 µs | 157.3 MB/s |
| `StringObject_64x32` | 2,807 | 10.28 µs | 260.3 MB/s |
| `ArrayOfObjects_256` | 14,135 | 117.76 µs | 114.5 MB/s |
| `ArrayOfObjects_2048` | 118,527 | 952.99 µs | 118.6 MB/s |
| `Nested_8x32` | 771 | 10.10 µs | 72.8 MB/s |

## 缩进序列化（`json_value::to_indent_string`）

缩进输出会额外产生大量空白与拼接，用于衡量格式化开销。

| 文档 | 大小（字节） | 耗时 | 吞吐 |
|------|-------------:|-----:|-----:|
| `FlatObject_32` | 339 | 4.90 µs | 65.9 MB/s |
| `FlatObject_256` | 3,181 | 37.89 µs | 80.1 MB/s |
| `StringObject_64x32` | 2,807 | 11.24 µs | 238.3 MB/s |
| `ArrayOfObjects_256` | 14,135 | 162.62 µs | 82.9 MB/s |
| `ArrayOfObjects_2048` | 118,527 | 1339.50 µs | 84.4 MB/s |
| `Nested_8x32` | 771 | 21.26 µs | 34.6 MB/s |

## 解析 + 序列化往返

单次迭代完成「解析 + 紧凑序列化」，代表真实转发/转换场景。

| 文档 | 大小（字节） | 耗时 | 吞吐 |
|------|-------------:|-----:|-----:|
| `FlatObject_32` | 339 | 6.18 µs | 52.3 MB/s |
| `FlatObject_256` | 3,181 | 48.11 µs | 63.1 MB/s |
| `StringObject_64x32` | 2,807 | 18.66 µs | 143.5 MB/s |
| `ArrayOfObjects_256` | 14,135 | 235.50 µs | 57.2 MB/s |
| `ArrayOfObjects_2048` | 118,527 | 1927.25 µs | 58.7 MB/s |
| `Nested_8x32` | 771 | 21.10 µs | 34.8 MB/s |

## 构建性能（`json_builder`）

构建与 `FlatObject_N` 等价的文档，逐成员 `key()/value()` 后 `build()`。

| 成员数 | 耗时 | 每成员 |
|-------:|-----:|-------:|
| 32 | 3.34 µs | 104 ns |
| 256 | 25.24 µs | 99 ns |
| 2048 | 221.41 µs | 108 ns |

## 成员访问性能（宽对象按 key 取成员）

对象在测量前解析一次，循环内只做一次 `get_member`（键位于对象中部，避免首尾成员的缓存偏差）。

| 成员数 | 单次访问 |
|-------:|---------:|
| 32 | 15928 ns |
| 256 | 10748 ns |
| 2048 | 11015 ns |

## 成员字符串提取

解析 `StringObject_Nx32` 后遍历全部成员并取字符串值的长度。

| 成员数 | 耗时 | 每成员 |
|-------:|-----:|-------:|
| 64 | 0.20 µs | 3 ns |
| 512 | 1.56 µs | 3 ns |
| 2048 | 6.57 µs | 3 ns |

## 性能画像

| 维度 | 结论 |
|------|------|
| 解析吞吐 | 68.5 ~ 315.4 MB/s，字符串占比越高越快，嵌套结构最慢 |
| 紧凑序列化吞吐 | 72.8 ~ 260.3 MB/s，整体略高于解析 |
| 缩进序列化 | 吞吐约为紧凑版的 0.38 ~ 0.91 倍，空白字符与拼接是主要开销 |
| 解析 + 序列化往返 | 与「解析」几乎等价，序列化只占往返耗时的 30% 左右 |
| `json_builder` 构建 | 约 100 ns/成员，与解析同量级 |
| 成员访问 | `get_member` 约 11 ~ 16 ns（底层为 `flat_unordered_map` 哈希查找） |
| 成员字符串提取 | 约 3 ns/成员，遍历成本远低于解析 |

## 可优化方向

| 方向 | 预期收益 | 复杂度 |
|------|----------|:------:|
| 值对象内联 / arena 分配：每个 `json_value` 与每个对象成员当前各一次堆分配 | 解析与构建提升 40 ~ 60% | 中 |
| 字符串扫描 SIMD 化（AVX2 一次 32 字节查找 `"` 与 `\\`） | 字符串密集文档解析提升 20 ~ 30% | 低 |
| 数字解析快速路径（对齐 `from_chars` 的批量转换） | 数字密集文档解析提升 15 ~ 25% | 低 |
| `to_string` 预分配输出容量（按输入规模估算） | 序列化提升 10 ~ 20% | 低 |
| `json_object` 成员表换用扁平哈希（已用 `flat_unordered_map`，可继续调优负载因子） | 成员访问再降 10 ~ 20% | 低 |
| 缩进序列化改为一次性写入缩进块，避免逐层字符串拼接 | 缩进序列化提升 15 ~ 25% | 低 |

## 复现方式

```bash
# 构建
cmake --build build --target NexusForceJsonBenchmark

# 单核测量（3 次进程，取各条目最小值）
taskset -c 2 build/bin/Release/NexusForceJsonBenchmark \
    --benchmark_repetitions=3 --benchmark_report_aggregates_only=true

# 只看解析
taskset -c 2 build/bin/Release/NexusForceJsonBenchmark --benchmark_filter='Parse'
```
