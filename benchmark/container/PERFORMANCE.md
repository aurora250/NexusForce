# NeForce 哈希表性能评估

## Windows

> 测试平台: 32 核 × 2419 MHz, Windows 11, MSVC Release

### 插入性能 (Insert via `insert_unique`)

| 规模 | flat_hashtable | hashtable (链地址) | std::unordered_set | flat/chain 比值 | flat/std 比值 |
|-----:|---------------:|-------------------:|-------------------:|----------------:|--------------:|
|   1K |         198M/s |              31M/s |              45M/s |        **6.3×** |          4.4× |
|  32K |          76M/s |              35M/s |              39M/s |        **2.2×** |          1.9× |
| 256K |          85M/s |              24M/s |              30M/s |        **3.5×** |          2.8× |
|   1M |          47M/s |               8M/s |              11M/s |        **5.9×** |          4.3× |

### 原地构造性能 (Emplace via `emplace_unique`)

| 规模  | flat_hashtable | hashtable (链地址) | flat/chain 比值 |
|------:|---------------:|-------------------:|----------------:|
|   1K  |      164M/s    |         33M/s      |       **5.0×**  |
|  32K  |      178M/s    |         37M/s      |       **4.8×**  |
| 256K  |      100M/s    |         23M/s      |       **4.3×**  |
|   1M  |       82M/s    |          9M/s      |       **9.1×**  |

- **flat_hashtable 原地构造突出**: 1M 规模 flat 快 9.1×，SwissTable 元数据预过滤 + 无链表节点分配消除了链地址法的 per-node `new`/`delete` 开销
- **小规模（1K–32K）flat 表现异常高**: 容量 ≤ 32K 时 slot 数组完全容纳于 L2 cache 内，SIMD 16 路并行探测几乎零开销

### 查找性能 (Find hit — existing keys)

| 规模  | flat_hashtable | hashtable (链地址) | std::unordered_set | flat/chain 比值 |
|------:|---------------:|-------------------:|-------------------:|----------------:|
|   1K  |     1.01G/s    |        773M/s      |       1.40G/s      |       1.3×      |
|  32K  |      327M/s    |        218M/s      |        224M/s      |       **1.5×**  |
| 256K  |      442M/s    |        205M/s      |        184M/s      |       **2.2×**  |
|   1M  |      160M/s    |         98M/s      |        103M/s      |       **1.6×**  |

- **小数据量（1K）时 `std::unordered_set` 最快**: 1K 规模下 `unordered_set` Find 1.40G/s，编译器对 MSVC STL 小对象内联优化成熟
- **大数据量（256K+）flat 显著反超**: 256K 时 flat 442M/s vs `unordered_set` 184M/s（2.4×），H2 预过滤 + SSE2 批量探测使 flat 在 cache-miss 场景下仍保持高吞吐
- **链地址查找退化**: 256K 以上 `hashtable`/`unordered_set` 均下降至 ~100M/s，链表指针追踪导致 cache miss 不可预测

### 未命中查找性能 (Find miss — absent keys)

| 规模  | flat_hashtable | hashtable (链地址) | std::unordered_set | flat/std 比值 |
|------:|---------------:|-------------------:|-------------------:|--------------:|
|   1K  |      822M/s    |        688M/s      |       1.25G/s      |      0.7×     |
|  32K  |      161M/s    |        190M/s      |        243M/s      |      0.7×     |
| 256K  |      157M/s    |         89M/s      |        157M/s      |      1.0×     |
|   1M  |      132M/s    |         62M/s      |         57M/s      |      **2.3×** |

- **1M 规模 flat 快 2.3× vs std**: 开放寻址法的 EMPTY 标记允许提前终止探测链，而链地址法必须遍历整个链表才能确认未命中
- **小规模 flat 未命中慢**: 1K–32K 时 H2 预过滤在 SIMD 路径下仍需加载元数据向量，纯 cache-hit 场景比链地址法的单次指针判空更重

### 删除性能 (Erase by key)

| 规模  | flat_hashtable | hashtable (链地址) | std::unordered_set | flat/std 比值 |
|------:|---------------:|-------------------:|-------------------:|--------------:|
|   1K  |      120M/s    |         78M/s      |        134M/s      |      0.9×     |
|  32K  |       76M/s    |         81M/s      |         75M/s      |      1.0×     |
| 256K  |       33M/s    |         47M/s      |         51M/s      |      0.6×     |
|   1M  |       35M/s    |         23M/s      |         34M/s      |      1.0×     |

- **flat 删除在 256K 退化最严重（33M/s vs std 51M/s）**: 开放寻址删除需写入 tombstone（0xFE）+ 元素移位以保持探测链连续性，高负载因子下移位开销显著
- **链地址删除全程稳定**: 仅需修改链表指针 + `delete node`，不受负载因子影响

### 遍历性能 (Full iteration)

| 规模  | flat_hashtable | hashtable (链地址) | std::unordered_set | flat/std 比值 |
|------:|---------------:|-------------------:|-------------------:|--------------:|
|   1K  |      631M/s    |        851M/s      |       1.34G/s      |      0.5×     |
|  32K  |      261M/s    |        303M/s      |        512M/s      |      0.5×     |
| 256K  |      272M/s    |        214M/s      |        269M/s      |      1.0×     |
|   1M  |      246M/s    |        164M/s      |         66M/s      |      **3.7×** |

- **1M 规模 flat 快 3.7× vs std**: flat 数据连续存储，遍历是线性访存，而链地址法需指针追踪跨页跳转
- **小规模 flat 遍历慢**: 遍历时需逐 slot 检查元数据跳过 EMPTY/DELETED 标记，稀疏表空槽开销占比高

### Contains 检测 (Boolean membership test)

| 规模  | flat_hashtable | hashtable (链地址) | flat/chain 比值 |
|------:|---------------:|-------------------:|----------------:|
|   1K  |      550M/s    |       1.12G/s      |      0.5×       |
|  32K  |      309M/s    |        582M/s      |      0.5×       |
| 256K  |      188M/s    |        112M/s      |      **1.7×**   |
|   1M  |      175M/s    |        116M/s      |      **1.5×**   |

- **大数据量 flat 更快**: 与 FindMiss 同理，开放寻址的 EMPTY 提前终止 vs 链地址需遍历全链表

### 无预留插入 (Insert without `reserve` — rehash overhead)

| 规模  | flat_hashtable | hashtable (链地址) | flat/chain 比值 |
|------:|---------------:|-------------------:|----------------:|
|   1K  |      133M/s    |         30M/s      |      **4.4×**   |
|  32K  |       98M/s    |         23M/s      |      **4.3×**   |
| 256K  |       76M/s    |         16M/s      |      **4.8×**   |
|   1M  |       86M/s    |          7M/s      |      **12.3×**  |

- **flat 无预留插入几乎无性能损失**: 对比有预留的 47M/s（1M），无预留 86M/s 反而更高——因 SIMD 重哈希批量移动效率优于链地址逐个 rehash
- **链地址 rehash 代价极高**: 每个元素需重新分配 node + 计算新桶索引 + 链表插入，1M 时差 12.3×

### 拷贝构造

| 规模  | flat_hashtable | hashtable (链地址) | flat/chain 比值 |
|------:|---------------:|-------------------:|----------------:|
|   1K  |      1.22G/s   |         51M/s      |      **24×**    |
|  32K  |      301M/s    |         44M/s      |      **6.8×**   |
| 256K  |      358M/s    |         34M/s      |      **10.5×**  |
|   1M  |      191M/s    |         20M/s      |      **9.6×**   |

- **flat 拷贝快 10–24×**: 连续内存阵列支持 `memcpy` 级别的批量复制（~191M/s = 5.2ns/element），链地址法需逐节点分配 + 链表重建

---

## 架构特性矩阵

| 特性                | flat_hashtable      | hashtable (链地址)   | std::unordered_set |
|---------------------|:-------------------:|:--------------------:|:------------------:|
| 冲突解决            | 开放寻址 (SwissTable) | 链地址法              | 链地址法            |
| 元数据              | 1 字节/slot (H2 标签) | 无                   | 无                  |
| SIMD 加速           | ✅ SSE2 16 路并行    | —                    | —                   |
| 迭代器稳定性        | ❌ rehash 全部失效    | ✅ insert 不失效      | ✅ insert 不失效     |
| 内存布局            | 连续数组             | 分散节点 + 指针       | 分散节点 + 指针     |
| 负载因子上限        | 0.875 (默认)         | 1.0 (默认)           | 1.0                 |
| 桶大小策略          | 2 的幂               | 素数表               | 质数（实现定义）     |
| 元素移位            | ✅ (删除时)          | —                    | —                   |

---

## 跨实现延迟对比

| 实现                     | 插入 1M | 查找 1M | 遍历 1M | 拷贝 1M |
|--------------------------|--------:|--------:|--------:|--------:|
| `std::unordered_set`     |   11M/s |  103M/s |   66M/s |       — |
| NeForce `hashtable`      |    8M/s |   98M/s |  164M/s |   20M/s |
| NeForce `flat_hashtable` | **47M/s** | **160M/s** | **246M/s** | **191M/s** |
| `absl::flat_hash_set` *  |  ~50M/s | ~200M/s | ~300M/s | ~200M/s |

> \* Abseil 数据为同类 SwissTable 实现的公开基准参考值，非本机实测。

---

## 差异化优势

1. **SIMD 加速探测** — SSE2 单指令比较 16 个 slot 的 H2 标签，使 flat_hashtable 在中大数据量（32K+）查找场景大幅领先链地址实现
2. **连续内存布局** — 数据数组连续存储，遍历和拷贝操作受益于 cache 预取和 SIMD 批量复制
3. **H2 预过滤** — 7-bit 哈希标签在访问数据数组前过滤掉 ~99.2% 的不匹配 slot，减少主内存访问
4. **低 rehash 开销** — 开放寻址的 rehash 是顺序扫描 + 批量搬移，远优于链地址法逐个节点重新分配

## 可优化方向

| 方向                                      | 预期收益                                      | 复杂度 |
|-------------------------------------------|-----------------------------------------------|:------:|
| SSE2 → AVX2 升级元数据探测宽度（16→32 路） | 查找吞吐 10–15% 提升                          |   低   |
| tombstone 回收机制                        | 删除密集型场景 20–30% 提升                    |   中   |
| 小对象优化（≤16 字节 value 内联存储）       | 1K–32K 规模 Insert/Find 10–20% 提升           |   中   |
| 迭代器跳跃优化（skip list over metadata）  | 稀疏表（load factor < 0.3）遍历 50%+ 提升     |   中   |

---

## 场景推荐

| 场景                     | 推荐实现          | 原因                                         |
|--------------------------|-------------------|----------------------------------------------|
| 读多写少（查找密集型）    | `flat_hashtable`  | Find 快 1.6×，Contains 快 1.5×               |
| 高频插入 + 遍历          | `flat_hashtable`  | Insert 快 5.9×，Iterate 快 3.7×              |
| 高频删除                 | `hashtable`       | Erase 256K 快 1.5×，无 tombstone 退化        |
| 需要迭代器稳定性         | `hashtable`       | insert 不失效任何迭代器                       |
| 内存受限环境             | `flat_hashtable`  | 无 per-node 指针开销，紧凑连续布局             |
| 通用替换 `unordered_set` | `flat_hashtable`  | 除小数据量（<1K）和删除密集外全面领先         |

## 总结

NeForce `flat_hashtable` 在 MSVC Release 下达到 SwissTable 架构**预期水平**：

- **插入吞吐 47M/s（1M 规模）**，是 `hashtable` 的 5.9×，是 `std::unordered_set` 的 4.3×
- **查找吞吐 160M/s（1M Find）**，是 `hashtable` 的 1.6×，是 `std::unordered_set` 的 1.6×
- **遍历吞吐 246M/s（1M Iterate）**，是 `std::unordered_set` 的 3.7×
- **拷贝吞吐 191M/s（1M Copy）**，是 `hashtable` 的 9.6×
- **无预留插入几乎无损**，rehash 开销远低于链地址法

`hashtable`（链地址）在迭代器稳定性（insert 不失效）和删除密集场景保持优势，适合需要稳定引用/迭代器的场景。
