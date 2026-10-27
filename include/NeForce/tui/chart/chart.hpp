#ifndef NEFORCE_TUI_CHART_CHART_HPP__
#define NEFORCE_TUI_CHART_CHART_HPP__

/**
 * @file chart.hpp
 * @brief 通用图表元素与交互组件
 *
 * 提供柱状图、折线图、面积图、饼图、环形图、散点图、直方图、热力图与雷达图。
 * 图表直接绘制到画布单元格，使用主题角色色、Unicode 半块与盲文点阵，
 * 支持基于进度的确定性揭示动画、图例、坐标轴与可交互的选中点。
 *
 * @code
 * tui::chart_spec spec;
 * spec.kind = tui::chart_kind::bar;
 * spec.title = "Requests";
 * spec.labels = {"Mon", "Tue", "Wed"};
 * spec.series.push_back({"GET", {12, 30, 18}});
 * auto view = tui::chart(spec);
 * @endcode
 */

#include "NeForce/core/container/deque.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/element.hpp"
#include "NeForce/tui/dom/theme.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 图表类型
 */
enum class chart_kind : uint8_t {
    bar,       ///< 分组柱状图
    line,      ///< 盲文折线图
    area,      ///< 半块填充面积图
    pie,       ///< 饼图
    donut,     ///< 环形图
    scatter,   ///< 散点图
    histogram, ///< 连续分箱直方图
    heatmap,   ///< 行列表格热力图
    radar,     ///< 雷达图
};

/**
 * @brief 图例位置
 */
enum class chart_legend_position : uint8_t {
    none,  ///< 不显示图例
    right, ///< 显示在图表右侧
    bottom ///< 显示在图表底部
};

/**
 * @brief 揭示动画缓动函数
 *
 * `progress` 先经过缓动映射，再作用于数据揭示；固定输入始终得到固定输出。
 */
enum class chart_easing : uint8_t {
    linear,      ///< 线性
    ease_in,     ///< 三次缓入
    ease_out,    ///< 三次缓出
    ease_in_out, ///< 三次缓入缓出
    smooth,      ///< 平滑阶跃（两端导数为零）
};

/**
 * @brief 流式数据聚合方式
 *
 * 仅在 `chart_spec::stream_buckets > 0` 时生效。
 */
enum class chart_stream_aggregate : uint8_t {
    last, ///< 每个桶取最后一个有限样本
    high, ///< 每个桶取最大值
    low,  ///< 每个桶取最小值
};

/**
 * @brief 图表数据系列
 *
 * 每个系列包含一个名称与一组数值。折线图、柱状图等按索引共享 `chart_spec::labels`；
 * 热力图把每个系列当作一行；饼图与环形图把每个系列当作一个扇区。
 */
struct chart_series {
    string name;                           ///< 系列名称，用于图例与提示
    vector<double> values;                 ///< 数值列表，非有限值会被忽略
    const deque<double>* stream = nullptr; ///< 流式数据源；设置后优先于 values
    optional<_NEFORCE color> color;        ///< 显式颜色，优先于角色色
    optional<theme_role> role;             ///< 主题角色色，未设置时使用内置序列调色板
};

/**
 * @brief 坐标轴与刻度选项
 */
struct chart_axis_options {
    bool show = true;                ///< 是否绘制坐标轴
    bool show_ticks = true;          ///< 是否绘制刻度线
    bool show_labels = true;         ///< 是否绘制刻度值标签
    bool zero_baseline = true;       ///< 柱状图、面积图等是否强制包含零基线
    int tick_count = 5;              ///< 期望的刻度数量（至少 2）
    optional<double> min;            ///< 强制最小值，非有限值忽略
    optional<double> max;            ///< 强制最大值，非有限值忽略
    function<string(double)> format; ///< 自定义数值格式化函数，为空时使用内置格式
};

/**
 * @brief 网格线选项
 */
struct chart_grid_options {
    bool show = true;               ///< 是否绘制网格线
    bool vertical = false;          ///< 是否绘制纵向网格线
    optional<_NEFORCE color> color; ///< 显式网格颜色
    optional<theme_role> role;      ///< 网格主题角色色
};

/**
 * @brief 图例选项
 */
struct chart_legend_options {
    bool show = true;                                              ///< 是否显示图例
    chart_legend_position position = chart_legend_position::right; ///< 图例位置
    optional<_NEFORCE color> color;                                ///< 图例文本显式颜色
    optional<theme_role> role;                                     ///< 图例文本主题角色色
};

/**
 * @brief 图表描述
 *
 * 纯数据描述，可反复渲染且不会持有终端资源。宽度或高度为 0 时使用默认尺寸。
 */
struct chart_spec {
    chart_kind kind = chart_kind::bar;          ///< 图表类型
    string title;                               ///< 标题，空表示不显示
    vector<string> labels;                      ///< 类目标签（X 轴、热力图表头、雷达轴名、饼图扇区名）
    vector<chart_series> series;                ///< 数据系列
    int width = 0;                              ///< 固定宽度（列），0 使用默认宽度
    int height = 0;                             ///< 固定高度（行），0 使用默认高度
    chart_axis_options axis;                    ///< 坐标轴选项
    chart_grid_options grid;                    ///< 网格选项
    chart_legend_options legend;                ///< 图例选项
    float progress = 1.0F;                      ///< 揭示进度，0 到 1，非有限值按 0 处理
    chart_easing easing = chart_easing::smooth; ///< 揭示缓动函数
    optional<size_t> selected_index;            ///< 初始选中索引，越界时忽略

    size_t stream_samples = 0;                                              ///< 流式物化窗口，0 表示全部
    size_t stream_buckets = 0;                                              ///< 流式桶数；>0 时按桶聚合（opt-in）
    chart_stream_aggregate stream_aggregate = chart_stream_aggregate::last; ///< 桶聚合方式
};

/**
 * @brief 选中数据点
 *
 * 折线图、柱状图、散点图、直方图与雷达图的一个索引会对应多个系列的点，
 * 此时 `series_index` 区分具体系列；饼图、环形图与热力图的一个索引只对应一个点。
 */
struct chart_point {
    size_t series_index = 0; ///< 系列索引
    size_t point_index = 0;  ///< 系列内数据点索引
    string series_name;      ///< 系列名称
    string label;            ///< 类目标签，无标签时为空
    double value = 0.0;      ///< 数值
};

/**
 * @brief 对揭示进度应用缓动函数
 * @param progress 原始进度，超出范围会被钳制，NaN 按 0 处理
 * @param easing 缓动函数
 * @return 缓动后的进度，范围 0 到 1
 */
NEFORCE_API float chart_eased_progress(float progress, chart_easing easing) noexcept;

/**
 * @brief 解析系列颜色
 * @param t 主题
 * @param series 系列
 * @param index 系列索引
 * @return 系列颜色
 *
 * 优先级为显式颜色、主题角色色、内置序列调色板。
 */
NEFORCE_API _NEFORCE color chart_series_color(const theme& t, const chart_series& series, size_t index);

/**
 * @brief 获取图表可选中索引数量
 * @param spec 图表描述
 * @return 可选中索引数量，无数据时为 0
 *
 * 折线图、柱状图、散点图、直方图与雷达图返回类目数量；
 * 饼图与环形图返回系列数量；热力图返回行数乘列数。
 */
NEFORCE_API size_t chart_point_count(const chart_spec& spec);

/**
 * @brief 获取指定索引对应的全部数据点
 * @param spec 图表描述
 * @param index 选中索引
 * @return 数据点列表，索引无效时为空
 */
NEFORCE_API vector<chart_point> chart_points_at(const chart_spec& spec, size_t index);

/**
 * @brief 创建静态图表元素
 * @param spec 图表描述
 * @return 画布元素
 *
 * 使用 `spec.selected_index` 作为选中点；无交互能力但会绘制十字线与提示行。
 * 需要键盘、鼠标交互或动画时使用 `chart_component`。
 *
 * @note 元素尺寸由 `spec.width`、`spec.height` 决定，为 0 时使用 60×16 的默认尺寸。
 */
NEFORCE_API element chart(const chart_spec& spec);

/**
 * @brief 交互式图表组件
 *
 * 拥有真实的选中状态：方向键左右移动选中类别，热力图额外支持上下移动，
 * 鼠标按下时按实际绘制几何做命中测试。选中后绘制十字线与提示行，
 * `selected_point()` 与 `selected_points()` 暴露当前选中的标签与数值。
 *
 * 组件可通过 `animate_progress()` 在真实帧回调中推进揭示进度，
 * 进度值本身保持线性推进，绘制时再应用 `chart_spec::easing`。
 */
class NEFORCE_API chart_component final : public component_base {
public:
    /**
     * @brief 构造交互式图表
     * @param spec 图表描述
     *
     * 越界的 `selected_index` 会被忽略。
     */
    explicit chart_component(chart_spec spec);

    /**
     * @brief 获取图表描述
     * @return 图表描述常量引用
     */
    NEFORCE_NODISCARD const chart_spec& spec() const noexcept { return spec_; }

    /**
     * @brief 替换图表描述
     * @param spec 新图表描述
     *
     * 选中索引按新数据范围重新钳制，正在进行的进度动画会被取消。
     */
    void set_spec(chart_spec spec);

    /**
     * @brief 设置揭示进度
     * @param progress 进度，超出范围会被钳制，NaN 按 0 处理
     *
     * 会取消正在进行的动画。
     */
    void set_progress(float progress);

    /**
     * @brief 获取当前揭示进度
     * @return 原始进度，范围 0 到 1
     */
    NEFORCE_NODISCARD float progress() const noexcept { return progress_; }

    /**
     * @brief 启动进度动画
     * @param target 目标进度
     * @param duration 持续时间
     *
     * 需要宿主持续调用组件树的帧回调（`on_animation`）。
     * 时间累加使用传入的毫秒增量，因此同一组增量序列产生相同结果。
     */
    void animate_progress(float target = 1.0F, milliseconds duration = 320_ms);

    /**
     * @brief 是否正在播放进度动画
     * @return 正在播放返回 true
     */
    NEFORCE_NODISCARD bool animating() const noexcept { return animating_; }

    /**
     * @brief 获取可选中索引数量
     * @return 数量
     */
    NEFORCE_NODISCARD size_t selection_count() const;

    /**
     * @brief 获取当前选中索引
     * @return 选中索引，未选中返回 none
     */
    NEFORCE_NODISCARD optional<size_t> selected_index() const noexcept { return selected_; }

    /**
     * @brief 选中指定索引
     * @param index 索引
     * @return 选中状态发生变化返回 true
     */
    bool select(size_t index);

    /**
     * @brief 选中下一个索引
     * @return 选中状态发生变化返回 true
     */
    bool select_next();

    /**
     * @brief 选中上一个索引
     * @return 选中状态发生变化返回 true
     */
    bool select_previous();

    /**
     * @brief 行方向移动选中索引（热力图为上一行，其他图表与上一个同义）
     * @return 选中状态发生变化返回 true
     */
    bool select_up();

    /**
     * @brief 行方向移动选中索引
     * @return 选中状态发生变化返回 true
     */
    bool select_down();

    /**
     * @brief 清除选中
     * @return 此前存在选中返回 true
     */
    bool clear_selection();

    /**
     * @brief 获取当前选中的第一个数据点
     * @return 数据点，未选中或无有效数值时返回 none
     */
    NEFORCE_NODISCARD optional<chart_point> selected_point() const;

    /**
     * @brief 获取当前选中索引对应的全部数据点
     * @return 数据点列表
     */
    NEFORCE_NODISCARD vector<chart_point> selected_points() const;

    /**
     * @brief 对最近一次绘制区域做局部坐标命中测试
     * @param x 相对图表左上角的列坐标
     * @param y 相对图表左上角的行坐标
     * @return 命中的选中索引，未命中返回 none
     */
    NEFORCE_NODISCARD optional<size_t> hit_test(int x, int y) const;

    /**
     * @brief 获取最近一次绘制所占的矩形
     * @return 布局矩形，尚未绘制时宽高为 0
     */
    NEFORCE_NODISCARD layout_rect area() const noexcept;

    /**
     * @brief 创建组件状态
     */
    void setup() override;

    /**
     * @brief 构建图表画布树
     * @return 画布元素
     */
    element render() override;

    /**
     * @brief 处理方向键、Home、End 与 Escape
     * @param e 键盘事件
     * @return 事件已消费返回 true
     */
    bool on_key(const key_event& e) override;

    /**
     * @brief 处理鼠标按下并做局部坐标命中测试
     * @param e 鼠标事件
     * @return 事件已消费返回 true
     */
    bool on_mouse(const mouse_event& e) override;

    /**
     * @brief 推进揭示进度动画
     * @param delta 距上一帧的毫秒数
     */
    void on_animation(int64_t delta) override;

    /**
     * @brief 图表可获取焦点
     * @return 恒为 true
     */
    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    chart_spec spec_;
    state<layout_rect>* rect_ = nullptr;
    optional<size_t> selected_;
    float progress_ = 1.0F;
    float animation_from_ = 0.0F;
    float animation_to_ = 1.0F;
    int64_t animation_elapsed_ = 0;
    int64_t animation_duration_ = 0;
    bool animating_ = false;

    void clamp_selection();
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_CHART_CHART_HPP__
