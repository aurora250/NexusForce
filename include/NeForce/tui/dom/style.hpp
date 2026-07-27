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
 * @brief 样式
 */
struct NEFORCE_API style {
    /**
     * @brief 内边距
     */
    struct padding {
        int top = 0;    ///< 上内边距
        int right = 0;  ///< 右内边距
        int bottom = 0; ///< 下内边距
        int left = 0;   ///< 左内边距
    };

    /**
     * @brief 外边距
     */
    struct margin {
        int top = 0;    ///< 上外边距
        int right = 0;  ///< 右外边距
        int bottom = 0; ///< 下外边距
        int left = 0;   ///< 左外边距
    };

    /**
     * @brief 边框样式
     */
    enum class border : uint8_t {
        none,    ///< 无边框
        single,  ///< 单线边框 ┌─┐│└─┘
        double_, ///< 双线边框 ╔═╗║╚═╝
        rounded, ///< 圆角边框 ╭─╮│╰─╯
    };

    /**
     * @brief 对齐方式
     */
    enum class align : uint8_t {
        start,   ///< 起始对齐
        center,  ///< 居中对齐
        end,     ///< 末尾对齐
        stretch, ///< 拉伸填充
    };

    /**
     * @brief 主轴对齐方式
     */
    enum class justify : uint8_t {
        start,         ///< 起始对齐
        center,        ///< 居中对齐
        end,           ///< 末尾对齐
        space_between, ///< 两端对齐，元素之间均匀分布
        space_around,  ///< 环绕对齐，每个元素两侧有相等间距
    };

    /**
     * @brief 布局方向
     */
    enum class direction : uint8_t {
        row,    ///< 水平排列
        column, ///< 垂直排列
    };

    /**
     * @brief 组件语义变体
     */
    enum class variant : uint8_t {
        default_,  ///< 默认样式
        primary,   ///< 主要操作
        secondary, ///< 次要操作
        danger,    ///< 危险操作
        success,   ///< 成功/确认操作
    };

    /**
     * @brief 文本换行模式
     */
    enum class wrap_mode : uint8_t {
        none,      ///< 不换行，超出截断
        word,      ///< 按单词边界换行
        character, ///< 按字符边界换行
    };

    /**
     * @brief 尺寸约束
     *
     * 描述元素在某个轴上的尺寸策略。
     */
    struct size_hint {
        /**
         * @brief 尺寸模式
         */
        enum size_mode : uint8_t {
            auto_,   ///< 由内容决定尺寸
            fixed,   ///< 固定字符数，由 value 指定
            fill,    ///< 占满剩余空间
            percent, ///< 父容器的百分比，value 为 0-100
        };

        size_mode mode = auto_; ///< 尺寸模式
        int value = 0;          ///< Fixed 字符数 / Percent 百分比 / Fill 弹性权重
    };

    optional<color> fg; ///< 前景色
    optional<color> bg; ///< 背景色

    optional<bool> bold;              ///< 粗体
    optional<bool> dim;               ///< 暗色
    optional<bool> underline;         ///< 单下划线
    optional<bool> underlined_double; ///< 双下划线
    optional<bool> italic;            ///< 斜体
    optional<bool> reverse;           ///< 反色
    optional<bool> blink;             ///< 闪烁
    optional<bool> strikethrough;     ///< 删除线

    optional<padding> padding;     ///< 内边距
    optional<margin> margin;       ///< 外边距
    optional<border> border;       ///< 边框样式
    optional<color> borderColor;   ///< 边框颜色
    optional<size_hint> width;     ///< 宽度约束
    optional<size_hint> height;    ///< 高度约束
    optional<align> align;         ///< 默认对齐方式
    optional<float> flex_grow;     ///< 弹性增长权重
    optional<float> flex_shrink;   ///< 弹性收缩权重
    optional<wrap_mode> text_wrap; ///< 文本换行模式

    /**
     * @brief 合并两个样式
     * @param base 基础样式
     * @param over 覆盖样式
     * @return 合并后的样式
     */
    static style merge(const style& base, const style& over);
};

/**
 * @brief 布局容器属性
 */
struct box_props {
    style::direction dir = style::direction::column;       ///< 排列方向
    style::justify justify = style::justify::start;        ///< 主轴对齐
    enum style::align align = style::align::start;         ///< 交叉轴对齐
    enum style::align align_content = style::align::start; ///< 多行/列时交叉轴内容对齐

    int gap = 0;                     ///< 子元素间距
    int cross_gap = 0;               ///< 换行时交叉轴间距
    style::size_hint width{};        ///< 宽度约束
    style::size_hint height{};       ///< 高度约束
    struct style::padding padding{}; ///< 容器内边距
    struct style::margin margin{};   ///< 容器外边距
    float flex_grow = 0.0F;          ///< 弹性增长因子
    float flex_shrink = 0.0F;        ///< 弹性收缩因子
    bool wrap = false;               ///< 是否允许换行
};

/**
 * @brief 主题
 */
struct NEFORCE_API theme {
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
    NEFORCE_NODISCARD style button_style(style::variant v) const;

    /**
     * @brief 获取默认文本样式
     * @return 文本样式
     */
    NEFORCE_NODISCARD style text_style() const;

    /**
     * @brief 获取输入框样式
     * @param focused 是否处于焦点状态
     * @return 输入框样式
     */
    NEFORCE_NODISCARD style input_style(bool focused) const;
};

/**
 * @brief 预置暗色主题
 */
NEFORCE_INLINE17 constexpr theme dark_theme = {
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
