# AI Agent Quick Start

## 索引文件

`api_index.jsonl` — 每行一个 JSON 格式 API。

## 字段说明

| 字段     | 含义                               | 示例                              |
|----------|------------------------------------|-----------------------------------|
| `name`   | 类型简称                           | `AES256`                          |
| `kind`   | 类型种类: class / struct / concept | `struct`                          |
| `ns`     | 命名空间                           | `neforce`                         |
| `header` | `#include` 路径                    | `NeForce/core/encrypt/aes256.hpp` |
| `brief`  | 一句话中文描述                     | `"AES-256加密算法结构体"`         |

## Agent 推荐工作流

1. **搜索**: Grep 中文关键词或类型名在 `api_index.jsonl`
2. **定位**: 从命中行获取 `header` 字段
3. **读取**: Read 对应的 `.hpp` 头文件获取完整签名和 doxygen 注释
4. **编写**: 按照项目 `CLAUDE.md` 中的编码规范编写代码

## 示例

```bash
# 找到所有加密相关的 API
grep "加密" docs/ai/api_index.jsonl

# 找到 TCP socket
grep "tcp_socket" docs/ai/api_index.jsonl
```

寻找组件之后注意：如果组件名称与标准库一致，大概率是与标准库组件功能一致或提供了额外功能的超集组件。
如果你使用的功能标准库同名组件会提供，默认本库组件也会提供即可，如标准库的组件未提供你需求的功能，你可以再决定具体到本库的组件中搜寻。
