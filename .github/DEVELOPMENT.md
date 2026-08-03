# NexusForce 开发规范

本文档面向 NexusForce 的所有贡献者，定义项目的**设计方案**与**开发规范**。
关于代码格式与静态审查条例，请遵循项目根目录的 Clang-Format 与 Clang-Tidy 配置。

---

## 一、设计方案

### 1.1 从清晰到性能

- 代码必须清晰可测。

```markdown
自检清单

- [ ] 变量/函数名称是否自解释？
- [ ] 是否有超过 3 层的嵌套循环/条件？
- [ ] 是否可以用现有算法替代手写重复？
- [ ] 是否有超过 7 个参数的函数？（参数过多时考虑结构体封装）
```

- 性能优化必须基于 profiling 数据（如 perf、Valgrind、Google Benchmark）。

性能优化路线：
```text
性能观察
↓
是否在热路径？（通过 perf 采样确认调用频率 > 5%）
↓ 是
是否符合上述量化标准？
↓ 是
优化 + 添加注释说明优化动机 + benchmark 验证
↓
优化后性能提升 < 5% → 回退，保持清晰代码
```

- 热路径、已知瓶颈可设计高性能方案，须在注释中说明。

### 1.2 资源安全

- **尽量不使用裸 `new` / `delete`**。

豁免场景（注释说明）：
1. 自定义内存池/分配器实现
2. 与 C API 交互的胶水代码
3. 性能热点中经 benchmark 验证的优化
4. 涉及无法表达的所有权转移模式（如 launder、placement new）

- 尽量使用 `unique_ptr`、`shared_ptr`、`optional`、`variant`、`memory_view`、`string_view`。
- 资源（锁、文件、内存、socket、数据库连接等）尽量由 RAII 进行管理。

### 1.4 统一异常

NexusForce 错误处理遵循：

- 不可恢复错误：抛出异常。
- 可恢复错误：返回 `optional`、`expected`或 `bool`。
- 性能热点错误：错误码、状态码、吞掉异常。

具体来说：
1. 公共 API 边界：使用异常（错误信息丰富、调用方易处理）
2. 内部实现（性能敏感）：使用 error_code / expected（避免异常栈展开开销）
3. 跨线程/异步回调：使用 error_code + 回调（异常不可跨线程）

所有 noexcept 声明必须满足：
1. 移动构造函数/赋值：应为 noexcept（除非有明确理由）
2. 析构函数：必须 noexcept
3. swap 函数：必须 noexcept
4. 无资源释放的 getter：应为 noexcept
5. 比较运算：应为 noexcept

---

## 二、架构/接口设计规范

### 2.1 模块组织

每个模块应满足：

- 独立头文件目录 `include/NeForce/<Module>`
- 对应源文件目录 `src/<Module>`

### 2.2 接口紧凑

- 类/函数单一职责。
- 优先写命名空间下的自由函数，而非将所有逻辑塞入类成员。

### 2.3 不变式

- 参数传递方式选择：

| 参数类型                   | 推荐方式       | 说明                |
|----------------------------|----------------|---------------------|
| 基本类型 (int, char, bool) | 值传递         | 寄存器传递          |
| 小对象 (≤16字节)           | 值传递         | 拷贝开销 < 引用间接 |
| 大对象 (可拷贝)            | const T&       | 避免拷贝            |
| 大对象 (仅移动)            | T&& 或 move    | 转移所有权          |
| 只读字符串                 | string_view    | 零拷贝 + 兼容 char* |
| 只读容器                   | memory_view<T> | 零拷贝 + 边界安全   |
| 输出参数                   | T* 或 T&       | 明确非空语义        |

- 类必须定义**不变式**（invariant），在构造后成立，并在公有方法前后保持。

### 2.4 生命周期

- 明确区分：
  - 拥有资源的对象（`unique_ptr`）
  - 索引资源的对象（`string_view`）
- 避免循环引用：使用 `weak_ptr` 或重构设计。
- 正确实现移动语义：`= default` 或成员逐个移动。

移动后状态约定：
- 容器：空状态（size() == 0, capacity() == 0）
- 索引类：无索引状态（size() == 0, ptr == nullptr）
- RAII 类：无资源（handle == nullptr）
- 自定义类：处于"有效但未指定"状态（仅可赋值/析构）

---

## 三、维护与演进

### 3.1 版本兼容性策略

NexusForce 遵循**语义化版本**：`MAJOR.MINOR.PATCH`

- **MAJOR**：破坏 API/ABI 兼容性
- **MINOR**：新增功能，向后兼容
- **PATCH**：Bug 修复，向后兼容

以下变更被视为破坏性变更：
- 删除或重命名公有 API
- 修改公有函数签名
- 修改公有类内存布局
- 修改枚举值
- 修改模板特化的行为

以下变更不被视为破坏性变更：
- 修复与文档不一致的行为
- 扩展默认模板参数
- 新增重载
- 放宽 constexpr/noexcept 约束

破坏性变更流程：
1. 使用 `[[deprecated("message, will be removed in vX.Y")]]` 标记废弃。
2. 至少保留 6 个月或两个 MINOR 版本。
3. 更新文档和迁移指南。

### 3.2 内部实现保护

- 所有不对外公开的实现放入 `inner` 命名空间。
- `inner` 中的内容不保证 API 稳定性。

### 3.3 依赖管理

- 依赖通过 **vcpkg** 管理。
- 新增依赖必须在 PR 中说明理由，并更新 README 和 CMakeLists。
- 避免传递不必要的依赖，设置依赖不能被加载的回退机制与检测宏。

```cmake
# 依赖查找 + 回退机制示例
if(NEXUSFORCE_ENABLE_ZLIB)
    find_package(ZLIB QUIET)
    if(TARGET ZLIB::ZLIB)
        target_link_libraries(NexusForce PUBLIC ZLIB::ZLIB)
        target_compile_definitions(NexusForce PUBLIC NEFORCE_SUPPORT_ZLIB)
        message(STATUS "ZLIB linked successfully")
    endif()
else()
    message(STATUS "zlib support disabled")
endif()
```

### 3.4 文档义务

- 每个公有头文件必须有模块级注释。
- 每个公有类、函数、枚举、模板必须有 Doxygen 风格注释，至少包含：
  - `@brief`：简要描述
  - `@param`：参数说明
  - `@return`：返回值说明
- 复杂算法须附论文链接或算法说明注释，网络协议等拥有国际标准的解析需标准链接及内容说明。

```c++
// 局部示例
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
 */
```

---

## 四、质量门禁

### 4.1 测试要求

- 每个新特性或 Bug 修复必须伴随测试。
- 测试框架：**GoogleTest**。
- 测试类型：
  - 单元测试（`test/unit/`）
  - 集成测试（`test/integration/`）
  - 性能测试（`benchmark/`）

测试资源管理：
1. 所有测试资源放在 `tests/resource/` 下
2. 禁止在测试中修改资源文件

### 4.2 目标覆盖率

- 核心模块行覆盖率 ≥ 90%
- 全项目行覆盖率 ≥ 70%

覆盖率豁免（需显式注释）：
- 调试辅助代码（LOG_DEBUG 等）
- 平台特定的异常路径
- 防御性编程分支（预期永不执行）

### 4.3 CI 强制检查

每个 PR 必须通过：

- **Clang-Format**
- **Clang-Tidy**
- **CodeQL**
- **Valgrind**
- **Build-Verify**

---

## 五、开发协作

### 5.1 Pull Request

**标题格式**：`[模块名] 简短描述`  
示例：`[core/container] 修复红黑树旋转时的内存泄漏`

**PR 描述模板**：

```markdown
## 设计决策
（说明为什么这样实现，替代方案有哪些）

## 性能影响
（对性能有提升/下降？附数据）

## 测试
- [ ] 单元测试通过
- [ ] 集成测试通过
- [ ] 测试覆盖变更

## 兼容性
- [ ] 不破坏现有 API
- [ ] 若破坏，已提供迁移指南
```

**PR 大小限制**：≤ 400 行变更（超出必须拆分，除非有充分理由）。

豁免条件（需在 PR 中说明）：
1. 自动生成的代码
2. 文档批量更新
3. 测试数据批量添加

### 5.2 Review

审查要素：
- 设计是否合理、是否符合规范
- 不变式是否保持
- 错误处理是否完整
- 并发安全性
- 资源泄漏
- 测试覆盖是否充分

### 5.3 Git 提交规范

遵循 **Conventional Commits**：

```
<type>(<scope>): <subject>

[optional body]

[optional footer]
```

**type** 类型：
- `feat`：新功能
- `fix`：Bug 修复
- `docs`：文档变更
- `style`：格式化
- `refactor`：重构
- `perf`：性能优化
- `test`：添加测试
- `chore`：构建/工具变更

示例：  
`feat(network/http): 添加 WebSocket 自动重连机制`  
`fix(core/async): 修复线程池任务窃取时的死锁`

### 5.4 禁止事项

- ❌ 三行以上的注释代码
- ❌ 未使用的变量
- ❌ 任何头文件中的全局命名空间 using
- ❌ 忽略编译器警告（除非有合理理由）

---

## 六、特殊规范

### 6.1 并发模块（core/async）

- 无锁数据结构（如 `无锁队列`）必须附带**形式化正确性论证**或**已知论文引用**。
- 高层次的并发组件（如 `线程池`）须有性能测试证明其优于其他方式的实现。

### 6.2 加密模块（core/encrypt）

- **严禁自己发明或修改加密算法**。所有实现必须基于标准文档。
- 加密函数必须恒定时间执行，防止时序攻击。

### 6.3 安全漏洞

安全漏洞报告：
1. 通过 GitHub Security Advisory 报告
2. 报告者获得 90 天协调披露窗口
3. 修复版本发布后，在 CHANGELOG 中标注 CVE
4. 致谢报告者

---

## 七、社区原则

### 7.1 欢迎新手

- 所有 `TODO` 标记的任务优先分配给首次贡献者。

所有 `TODO` 标记的任务在提交时，应在 PR 中评估并添加相应标签：

| 标签               | 含义                         |
|--------------------|------------------------------|
| `good first issue` | 适合首次贡献者，问题范围明确 |
| `help wanted`      | 需要社区帮助                 |
| `beginner`         | 低难度，适合学习代码库结构   |
| `junior job`       | 中等难度，需要一定 C++ 经验  |

- 代码审查应有**建设性**，指出问题时同时给出改进建议或示例。

### 7.2 决策机制

- 设计争议：提交 **RFC 文档**（`docs/rfc/`）讨论，至少 72 小时反馈期。
RFC 讨论结束后，如果 72 小时后无反对意见，该提案视为默认通过。
项目维护者可在 72 小时窗口内提出反对并说明理由。

RFC 模板：
```markdown
# docs/rfc/YYYY-MM-DD-<标题>.md

**状态**: 草稿 | 讨论中 | 已接受 | 已实现 | 已拒绝
**作者**: @username
**讨论期**: YYYY-MM-DD ~ YYYY-MM-DD

## 摘要
（1-2 段简述提案内容）

## 动机
（当前存在的问题或机会）

## 设计方案
（核心设计决策）

## 替代方案
（被否决的方案及原因）

## 兼容性影响
（对现有 API/ABI 的影响）

## 实现计划
（可选，分阶段实现）
```

- 核心模块变更：需至少一位项目维护者批准。

### 7.3 致谢

所有贡献者将列入 [CONTRIBUTORS](https://github.com/aurora250/NexusForce/blob/dev/CONTRIBUTORS.md)。
重大设计或性能突破将在 [CHANGELOG](https://github.com/aurora250/NexusForce/blob/dev/CHANGELOG.md) 中特别致谢。

---

## 八、规范更新

本规范随项目演进更新。任何贡献者可以：

1. 在 `.github/DEVELOPMENT.md` 上提交 PR 修改建议。
2. 在 PR 描述中说明修改理由和影响范围。
3. 至少两位维护者批准后合并。

---

**NexusForce 感谢您的贡献！**  
让 C++ 更强大、更工程、更可靠。
