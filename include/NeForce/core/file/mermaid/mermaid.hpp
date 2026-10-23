#ifndef NEFORCE_CORE_FILE_MERMAID_HPP__
#define NEFORCE_CORE_FILE_MERMAID_HPP__

/**
 * @file mermaid.hpp
 * @brief Mermaid 源码到字符图的渲染
 *
 * 把 Mermaid 的 flowchart、sequenceDiagram、stateDiagram-v2、classDiagram、erDiagram、gitGraph、pie、
 * mindmap、timeline、gantt、journey、quadrantChart 与 requirementDiagram 子集渲染为终端友好的 Unicode字符图。
 * 结果同时提供每行的角色分段，便于调用方按 border / text / edge / edge_label / title 着色。
 */

#include "NeForce/core/string/string.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Core Core
 * @{
 */

/**
 * @brief 图形角色
 */
enum class mermaid_role : uint8_t {
    plain = 0,  ///< 无样式
    border,     ///< 边框
    text,       ///< 节点文字
    edge,       ///< 连线
    edge_label, ///< 连线标签
    title,      ///< 标题
};

/**
 * @brief 同角色的一段文字
 */
struct mermaid_run {
    size_t begin = 0;                        ///< 起始字节偏移
    size_t end = 0;                          ///< 结束字节偏移（不含）
    mermaid_role role = mermaid_role::plain; ///< 角色
};

/**
 * @brief 渲染结果
 */
struct mermaid_art {
    vector<string> lines;               ///< 每行文本
    vector<vector<mermaid_run>> styled; ///< 与 lines 等长，按行给出角色段
    int width = 0;                      ///< 最宽行的显示宽度
};

/**
 * @brief 把 Mermaid 源码渲染成字符图
 * @param source Mermaid 源码
 * @return 结果；不支持的图类型或解析失败返回 none
 *
 * 支持 flowchart（TD/TB/LR）、sequenceDiagram、stateDiagram-v2 的常见语法，另支持 classDiagram、
 * erDiagram、gitGraph、pie、mindmap、timeline、gantt、journey、quadrantChart、requirementDiagram。
 * 类图/ER 图/需求图按纵向堆叠布局，甘特图按任务时长归一化到固定宽度，gitGraph 仅绘制纵向泳道。
 * 空输入、仅包含空白或注释的输入、超过 20000 字节的输入均返回 none。
 * 每行都会用空格补齐到 width，`styled` 中所有段按顺序拼接后与对应行完全一致。
 */
NEFORCE_API optional<mermaid_art> render_mermaid(string_view source);

/** @} */ // TUI

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_MERMAID_HPP__
