# NeForce format 性能评估（对比 C++20 `std::format`）

> 测试平台: Intel i9-13980HX（8 P-core + 16 E-core = 32 逻辑 CPU），Linux 7.0，clang 20.1.2 Release（`-O2 -DNDEBUG -std=gnu++20 -mavx2`）
> 测量方法: `taskset -c 2` 固定单个 P-core，5 个独立进程 × 每进程 `--benchmark_repetitions=3`，对 15 个均值再取平均；测量期间机器无其他负载
> 基准文件: `benchmark/string/format/format_benchmark.cpp`；构建目标 `NexusForceFormatBenchmark`

## 对比前提

- `std::format` 侧仅在 **C++20 且 `<format>` 可用**（`__cpp_lib_format`）时编译；C++14/17 只编译 NeForce 侧，因此基准目标在低标准下同样可构建。
- 11 组用例两侧输出**逐字节一致**（整数、浮点、字符串、转义、多参数、复用缓冲），比较的是等价工作量而非不同输出。
- 两侧各自返回自身的字符串类型（`neforce::string` / `std::string`，SSO 容量均为 15 字符）。
- 格式串全部为字面量：NeForce 走 `const char (&)[N]` 重载（编译期括号校验），`std::format` 走编译期格式串校验，两者都不含运行时的格式串解析/校验成本。
- 结果均被 `benchmark::DoNotOptimize` 拦截，避免整段被优化掉。
- 浮点输出与 `std::format` 做过 216936 组随机/边界值的差分验证（`{:.0f}`~`{:.17f}`、`{:.0e}`~`{:.8e}`、`{:e}`），全部一致。

## 总览（ns/次，越小越好）

| 用例 | 模板 | NeForce | std::format | NF/std |
|------|------|--------:|------------:|-------:|
| IntegerDefault     | `"{}"` 42                     |  16.44 ns |  29.03 ns | **0.57×** |
| IntegerPadded      | `"{:06d}"` 1234               |  27.90 ns |  49.27 ns | **0.57×** |
| IntegerHex         | `"{:#x}"` 0xDEADBEEF          |  32.95 ns |  41.36 ns | **0.80×** |
| DoubleFixed        | `"{:.3f}"` π                  |  53.41 ns |  69.23 ns | **0.77×** |
| DoubleScientific   | `"{:e}"` 1234.5               |  57.21 ns |  70.26 ns | **0.81×** |
| DoubleRightAligned | `"{:>12.4f}"` π               |  78.48 ns |  82.65 ns | **0.95×** |
| CStringArg         | `"user={}"`                   |  16.83 ns |  30.18 ns | **0.56×** |
| EscapedBraces      | `"{{literal}} {}"` 42         |  44.05 ns |  49.96 ns | **0.88×** |
| MultiMixed         | `"{} {:.1f} {} {}"`           | 122.14 ns | 155.30 ns | **0.79×** |
| LogLine            | `"[{}] {}:{} {:.1f}ms"`       | 134.81 ns | 163.03 ns | **0.83×** |
| AppendReuse        | `format_to` + `clear()` 复用  | 167.68 ns | 166.10 ns | 1.01× |

**几何平均 NF/std = 0.760**（< 1 表示 NeForce 更快；11 组中 10 组更快，AppendReuse 与 std 持平）。

吞吐换算（百万次/秒）：

| 用例 | NeForce | std::format |
|------|--------:|------------:|
| IntegerDefault     | 60.8 | 34.4 |
| IntegerPadded      | 35.8 | 20.3 |
| IntegerHex         | 30.3 | 24.2 |
| DoubleFixed        | 18.7 | 14.4 |
| DoubleScientific   | 17.5 | 14.2 |
| DoubleRightAligned | 12.7 | 12.1 |
| CStringArg         | 59.4 | 33.1 |
| EscapedBraces      | 22.7 | 20.0 |
| MultiMixed         |  8.2 |  6.4 |
| LogLine            |  7.4 |  6.1 |
| AppendReuse        |  6.0 |  6.0 |

## 分项分析

### 1. 整数：全面领先（0.57× ~ 0.80×）

- `uint_to_string_base` 改为**栈缓冲区正序生成 + 十进制两位一组**（`divmod 100` 由常量除法优化为乘法），最后一次性 `string(buffer, len)` 构造，不再 `reserve` + 逐位 `push_back` + `reverse`。
- 整数格式化不再先构造中间 `string raw`：数字、符号直接写进 32 字节栈缓冲区，再交给填充逻辑；`{}` 路径只做一次 `append`。
- `apply_format_options` 重构为“追加到输出”的 `apply_format_options_into`，去掉了按值接收的 `raw` 副本、`prefix`/`sign_str`/`left_pad`/`right_pad` 等临时字符串。
- 结果：`{}` 16.44 ns（std 29.03），`{:06d}` 27.90 ns（std 49.27）。

### 2. 十六进制：2.08× → 0.80×

- 进制转换同样走栈缓冲区，`0x` 前缀写进局部 `prefix_buffer`，不再构造前缀字符串。
- 逐位 `push_back` 与 `reverse()` 消失后，`{:#x}` 由 84.62 ns 降到 32.95 ns（std 41.36）。

### 3. 浮点：3.51× → 0.77× ~ 0.95×

浮点曾是最慢的一项，根因有三处，逐一消除：

1. **`logarithm_10` 占掉整个转换时间**：科学计数分支用 `logarithm_10(value)` 估算十进制指数，实测单次调用 **114 ns**（`std::log10` 仅 1.95 ns）。改为由二进制指数直接估算 `floor(log10(v))`：`(exponent + 52) × log10(2)`，误差 ±1 由既有的补位/截断逻辑吸收，**零函数调用**。
2. **大整数缩放过慢**：`floor(mantissa × 10^scale)` 原本一律走 40 肢体的任意精度整数。新增 128 位精确快速路径：`__umul128` 一次算出 `mantissa × 10^scale`（`scale ≤ 19`），再按指数左右移（右移同时给出“精确/不足半/恰好半/多于半”的舍入类别），最后用二位数字表 + `10^9` 分组输出。仅在 `2^exponent` 会溢出 128 位或 `scale > 19` 时回退大整数实现，语义完全等价。
3. **字符级拼接**：结果改为写入 768 字节栈缓冲区，末尾一次 `string(text, len)`；`to_string_*` 模板包装也不再逐字符 `+=`（`char` 特化直接一次 `memcpy`）。

`float_to_string_exact` 单次成本从 144.7 ns 降到约 22 ns；`{:e}` 由 226.44 ns 降到 57.21 ns，`{:.3f}` 由 82.01 ns 降到 53.41 ns。

### 4. 多参数与复用：1.05×/1.57× → 0.79×/1.01×

- 新增 **直接追加** 的格式化通道：`format_impl` 不再为每个占位符构造临时 `string` 再 `+=`，而是按参数类别（整数/浮点/文本）把结果直接写进输出缓冲区；文本参数直接 `append`（并保留 `{:.N}` 截断语义）。
- 文本类参数（`string`/`string_view`/`const char*`）不再先拷贝成 `string` 再填充，少一次堆分配判断与一次拷贝。
- `format_to` 复用缓冲时可稳定在 6.0 M/s，与 `std::format_to` 持平；`LogLine` 由 198.53 ns 降到 134.81 ns。

### 5. 唯一持平项：AppendReuse（1.01×）

- 该用例把结果写入已 `reserve(64)` 的缓冲，两侧都无内存分配，剩余成本集中在 4 个占位符的解析/分发与 25 字节拷贝上，差异落在测量噪声内（多次独立测量在 0.98× ~ 1.03× 间波动）。

## 已实施的优化手段（与上表对应）

1. `float_to_string_exact` 去除 `logarithm_10` 依赖（整数估算十进制指数）。
2. 128 位精确缩放快速路径 + 大整数回退（`__umul128`，必要时 `10^9` 分组长除法）。
3. 浮点结果改栈缓冲区拼装，包装层去掉逐字符转换。
4. `uint_to_string_base` 正序生成 + 两位数字表，去除 `reverse()`。
5. `apply_format_options` → `apply_format_options_into`，消除全部临时字符串。
6. 按参数类别直接追加（`append_arg_impl`），消除每占位符的临时 `string`。
7. 负零保留符号（`-0.0` 输出 `-0.0`，与 `std::format`/`std::to_string` 一致），并同步更新 `FloatEdgeCaseTest`。

## 复现

```bash
# 构建（C++20 才会编译 std 对照侧）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NexusForceFormatBenchmark

# 单次
taskset -c 2 ./build/bin/Release/NexusForceFormatBenchmark \
  --benchmark_filter='BM_(Nf|Std)Format_' \
  --benchmark_repetitions=3 --benchmark_report_aggregates_only

# 5 进程 × 3 次重复，取均值（本页数据）
for i in $(seq 5); do
  taskset -c 2 ./build/bin/Release/NexusForceFormatBenchmark \
    --benchmark_filter='BM_(Nf|Std)Format_' --benchmark_repetitions=3 \
    --benchmark_report_aggregates_only --benchmark_format=json \
    --benchmark_out=/tmp/fmt_$i.json
done
```

## 备注

- 低标准（C++14/17）下只编译 NeForce 侧；`std::format` 对照侧由 `NEFORCE_BENCH_STD_FORMAT` 宏保护，C++17 构建中 `StdFormat` 符号数为 0。
- 同一套测量方法下，机器若并行跑编译等负载，后段用例会被显著拖慢；请在与本页相同的静置条件下复测。
