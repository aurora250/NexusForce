#ifndef NEFORCE_TUI_STYLE_HPP__
#define NEFORCE_TUI_STYLE_HPP__

/**
 * @file style.hpp
 * @brief TUI样式与主题系统
 *
 * 定义了样式、边框、对齐方式、尺寸约束和主题等纯数据聚合类型。
 */

#include "NeForce/core/utility/color.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @defgroup TUI TUI
 * @brief NexusForce TUI 框架
 * @{
 */

/**
 * @brief 内边距
 */
struct Padding {
    int top = 0;    ///< 上内边距
    int right = 0;  ///< 右内边距
    int bottom = 0; ///< 下内边距
    int left = 0;   ///< 左内边距
};

/**
 * @brief 外边距
 */
struct Margin {
    int top = 0;    ///< 上外边距
    int right = 0;  ///< 右外边距
    int bottom = 0; ///< 下外边距
    int left = 0;   ///< 左外边距
};

/**
 * @brief 边框样式
 */
enum class Border : uint8_t {
    None,    ///< 无边框
    Single,  ///< 单线边框 ┌─┐│└─┘
    Double,  ///< 双线边框 ╔═╗║╚═╝
    Rounded, ///< 圆角边框 ╭─╮│╰─╯
};

/**
 * @brief 对齐方式
 */
enum class Align : uint8_t {
    Start,   ///< 起始对齐（左/上）
    Center,  ///< 居中对齐
    End,     ///< 末尾对齐（右/下）
    Stretch, ///< 拉伸填充
};

/**
 * @brief 主轴对齐方式
 */
enum class Justify : uint8_t {
    Start,        ///< 起始对齐
    Center,       ///< 居中对齐
    End,          ///< 末尾对齐
    SpaceBetween, ///< 两端对齐，元素之间均匀分布
    SpaceAround,  ///< 环绕对齐，每个元素两侧有相等间距
};

/**
 * @brief 布局方向
 */
enum class Direction : uint8_t {
    Row,    ///< 水平排列
    Column, ///< 垂直排列
};

/**
 * @brief 组件语义变体
 */
enum class Variant : uint8_t {
    Default,   ///< 默认样式
    Primary,   ///< 主要操作
    Secondary, ///< 次要操作
    Danger,    ///< 危险操作
    Success,   ///< 成功/确认操作
};

/**
 * @brief 尺寸约束
 *
 * 描述元素在某个轴上的尺寸策略。
 */
struct SizeHint {
    /**
     * @brief 尺寸模式
     */
    enum Mode : uint8_t {
        Auto,    ///< 由内容决定尺寸
        Fixed,   ///< 固定字符数，由 value 指定
        Fill,    ///< 占满剩余空间
        Percent, ///< 父容器的百分比，value 为 0-100
    };

    Mode mode = Auto; ///< 尺寸模式
    int value = 0;    ///< Fixed 时的字符数 / Percent 时的百分比（0-100） / Fill 时的弹性权重
};

/**
 * @brief 样式
 *
 * 描述元素的视觉属性。支持合并操作（右优先覆盖）。
 * 所有字段为 optional，nullopt 表示"不设置"，合并时被覆盖侧覆盖。
 */
struct Style {
    optional<color> fg;       ///< 前景色
    optional<color> bg;       ///< 背景色
    optional<bool> bold;      ///< 粗体
    optional<bool> underline; ///< 下划线
    optional<bool> italic;    ///< 斜体
    optional<bool> reverse;   ///< 反色

    optional<Padding> padding;   ///< 内边距
    optional<Margin> margin;     ///< 外边距
    optional<Border> border;     ///< 边框样式
    optional<color> borderColor; ///< 边框颜色
    optional<SizeHint> width;    ///< 宽度约束
    optional<SizeHint> height;   ///< 高度约束
    optional<Align> align;       ///< 默认对齐方式

    /**
     * @brief 合并两个样式
     * @param base 基础样式
     * @param over 覆盖样式
     * @return 合并后的样式
     */
    static Style merge(const Style& base, const Style& over);
};

/**
 * @brief 布局容器属性
 *
 * vbox / hbox 使用的布局控制参数。
 */
struct BoxProps {
    Direction dir = Direction::Column; ///< 排列方向
    Justify justify = Justify::Start;  ///< 主轴对齐
    Align align = Align::Start;        ///< 交叉轴对齐
    int gap = 0;                       ///< 子元素间距
    SizeHint width{};                  ///< 宽度约束
    SizeHint height{};                 ///< 高度约束
    Padding padding{};                 ///< 容器内边距
    Margin margin{};                   ///< 容器外边距
};

/**
 * @brief 主题
 *
 * 定义了 TUI 应用的全局色彩方案。
 * 组件样式方法返回适用于特定场景的 Style 对象。
 */
struct Theme {
    color bg;        ///< 默认背景色
    color fg;        ///< 默认前景色
    color primary;   ///< 主色调
    color secondary; ///< 次要色调
    color danger;    ///< 危险/错误色
    color success;   ///< 成功色
    color warning;   ///< 警告色
    color muted;     ///< 弱化/禁用色
    color border;    ///< 边框色

    /**
     * @brief 获取按钮样式
     * @param v 语义变体
     * @return 对应变体的样式
     */
    NEFORCE_NODISCARD Style buttonStyle(Variant v) const;

    /**
     * @brief 获取默认文本样式
     * @return 文本样式
     */
    NEFORCE_NODISCARD Style textStyle() const;

    /**
     * @brief 获取输入框样式
     * @param focused 是否处于焦点状态
     * @return 输入框样式
     */
    NEFORCE_NODISCARD Style inputStyle(bool focused) const;
};

/**
 * @brief 预置暗色主题
 */
NEFORCE_INLINE17 constexpr Theme dark_theme = {
        /* .bg = */ .bg = color{16, 16, 16},
        /* .fg = */ .fg = color{220, 220, 220},
        /* .primary = */ .primary = color::cyan(),
        /* .secondary = */ .secondary = color{100, 100, 220},
        /* .danger = */ .danger = color::red(),
        /* .success = */ .success = color::green(),
        /* .warning = */ .warning = color::yellow(),
        /* .muted = */ .muted = color{128, 128, 128},
        /* .border = */ .border = color{64, 64, 64},
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_STYLE_HPP__
