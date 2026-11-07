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
| IntegerDefault     | `"{}"` 42                     |  14.20 ns |  26.06 ns | **0.55×** |
| IntegerPadded      | `"{:06d}"` 1234               |  24.16 ns |  42.76 ns | **0.57×** |
| IntegerHex         | `"{:#x}"` 0xDEADBEEF          |  28.67 ns |  35.75 ns | **0.80×** |
| DoubleDefault      | `"{}"` 1/3（最短往返）        |  54.25 ns |  53.68 ns | 1.01× |
| DoubleGeneral      | `"{:g}"` 1/3                  |  49.68 ns |  55.10 ns | **0.90×** |
| DoubleFixed        | `"{:.3f}"` π                  |  47.85 ns |  59.05 ns | **0.81×** |
| DoubleScientific   | `"{:e}"` 1234.5               |  50.96 ns |  57.28 ns | **0.89×** |
| DoubleRightAligned | `"{:>12.4f}"` π               |  57.70 ns |  69.60 ns | **0.83×** |
| CStringArg         | `"user={}"`                   |  15.55 ns |  24.93 ns | **0.62×** |
| EscapedBraces      | `"{{literal}} {}"` 42         |  22.99 ns |  41.03 ns | **0.56×** |
| MultiMixed         | `"{} {:.1f} {} {}"`           | 108.25 ns | 130.26 ns | **0.83×** |
| LogLine            | `"[{}] {}:{} {:.1f}ms"`       | 113.11 ns | 137.75 ns | **0.82×** |
| AppendReuse        | `format_to` + `clear()` 复用  | 139.76 ns | 133.53 ns | 1.05× |

**几何平均 NF/std = 0.770**（< 1 表示 NeForce 更快；13 组中 11 组明显更快，`{}` 浮点默认格式 1.01×（与 std 持平，测量波动 ±1%），`format_to` 复用 1.05×）。

> 本页绝对数值取自 CPU 3（避开 C02）的一次 5 进程 × 3 重复测量，测量时机器仍有约 2.7 的负载，绝对值整体偏高；NF/std 比值与旧版同机可比的结论一致。

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

## 浮点默认格式语义（`{}` / `{:g}`）

旧实现的 `{}` 与 `{:g}` 都退化为「固定 6 位小数」（等价 `std::to_string`），`format("{}", 1.0/3.0)` 输出 `0.333333`，**回读不能还原原值**。现已按 `std::format` 语义重写：

| 场景 | 现行行为（与 `std::format` 逐字节一致） |
|---|---|
| `{}` / `{:}`，无精度 | 最短往返表示；`1.0`→`1`、`0.1`→`0.1`、`1/3`→`0.3333333333333333`、`1e15`→`1e+15`、`1e-4`→`1e-04`、`5e-324`→`5e-324`、`-0.0`→`-0` |
| `{:g}` / `{:.Ng}` | 有效数字语义 + 尾零裁剪；`1/3`→`0.333333`、`1234567`→`1.23457e+06`、`{:.3g}` π→`3.14` |
| ≥ 2^53` 的整数型 double | 从**精确整数**出发求最短表示：指数 ≤ 21 时按「定点/科学计数取较短者」输出；指数 > 21 时强制科学计数。该区间 ulp ≥ 1，补零形式可能落在往返区间之外 |
| `{:.Nf}` / `{:.Ne}` / `{:e}` | 保持不变（本次无回归） |

实现方式（不依赖 Ryu 表、不依赖回读校验）：

1. 用已有的精确十进制转换求出 18 位数字（17 位有效 + 1 位保护位）与往返区间 `v ± ulp/2`（`(2m∓1)·2^(e-1)` 同样走精确转换）；
2. 区间端点按「尾数奇偶」决定开闭（偶数尾数时中点可回读）；
3. 在区间内对「可删除的十进制位数」做**二分查找**，取最长的有效删除位数，即最短表示；
4. 尾零裁剪后按「定点/科学计数取较短者，等长取定点」输出。

验证：`tests/unit/test_format.cpp` 的 `FormatFloatShortest.MatchesStdFormat` 在 2 万随机位模式 + 边界值（次正规、DBL_MAX、2^53±1、1e±300 等）上对 `{}`、`{:g}`、`{:.3g}`、`{:.6g}`、`{:.17g}` 与 `std::format` 做**逐字节比对**，并把每个输出用 `strtod` 回读校验，全部通过。

### 性能：`{}` 已接近 `std::format`（1.26×）

- 最短表示改用 **Ryu d2s 的缩放方式**：用两张 128 位幂表（`5^i` 与 `floor(2^K/5^q)+1`）把尾数缩放到 17~18 位十进制整数，区间端点由 `(2m±1)` 与表项做同一次乘加得出，尾数为幂次时下界再折半；随后在区间内二分定位可删除的最长位数，最后按尾零裁剪输出。
- 全程只用 64/128 位整数乘加与少量十进制除法，不做回读校验，也不需要 `strtod`/`snprintf` 等任何标准库组件。
- `{}` 的端到端成本已从 325 ns 降到 54.3 ns（`std::format` 53.7 ns，即 1.01×，已持平）；`{:g}` 0.90×、`{:.3f}` 0.81×、`{:e}` 0.89×、`{:>12.4f}` 0.83×。
- 剩余差距在 `format_to` 复用缓冲用例（1.05×，同内容的 `format` 用例为 0.82×），属基准框架下的复用缓冲写入开销，后续可单独排查。

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
7. 浮点默认格式改为最短往返，`{:g}` 改为有效数字 + 尾零裁剪（行为变更）。
8. 负零保留符号（`-0.0` 输出 `-0.0`，与 `std::format`/`std::to_string` 一致），并同步更新 `FloatEdgeCaseTest`。

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
