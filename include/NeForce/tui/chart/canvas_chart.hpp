#ifndef NEFORCE_TUI_CANVAS_CHART_HPP__
#define NEFORCE_TUI_CANVAS_CHART_HPP__

/**
 * @file canvas_chart.hpp
 * @brief Canvas 流式图表渲染器
 *
 * 面向监控的趋势图：支持时间/样本窗口、极值聚合、缺口、braille/block/tty/ascii 符号、CPU 镜像填充和主题渐变。
 */

#include "NeForce/core/container/deque.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/utility/color.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/tui/dom/canvas.hpp"
#include "NeForce/tui/chart/chart.hpp"
#include "NeForce/tui/dom/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 单列投影区间
 */
struct canvas_chart_range {
    double low = 0.0;
    double high = 0.0;
    double last = 0.0;
    bool gap = false;
};

/**
 * @brief 时间刻度
 */
struct canvas_chart_time {
    const deque<double>* timestamps = nullptr;
    const deque<double>* periods = nullptr;
    uint64_t seconds = 0;
};

/**
 * @brief 图表窗口
 */
struct canvas_chart_window {
    size_t samples = 0;
    uint64_t end = 0;
    bool has_time = false;
    canvas_chart_time time{};

    NEFORCE_NODISCARD NEFORCE_API size_t stride(size_t width) const;
    NEFORCE_NODISCARD NEFORCE_API size_t capacity(size_t width) const;
    NEFORCE_API void project(const deque<double>& values, size_t width, vector<canvas_chart_range>& out) const;
};

/**
 * @brief 图表样式
 */
struct canvas_chart_style {
    neforce::color panel;
    neforce::color graph_text;
    neforce::color cpu_start;
    neforce::color cpu_mid;
    neforce::color cpu_end;
    string graph_symbol = "braille";
    bool ascii = false;
    bool gradient = true;

    /**
     * @brief 从 DOM chart 主题派生 canvas 样式
     */
    NEFORCE_NODISCARD static canvas_chart_style from_theme(const theme& value, string symbol = "braille",
                                                           bool gradient = true);
};

/**
 * @brief 将上限量化为 1/2/5×10^n
 * @param value 原始上限
 * @return 量化上限
 */
NEFORCE_API double canvas_chart_nice_max(double value);

/**
 * @brief 绘制 canvas 进度条
 * @param target 目标画布
 * @param x 列
 * @param y 行
 * @param width 宽
 * @param value 当前值，无值或非法值绘制空轨
 * @param maximum 上限
 * @param fill 填充色
 * @param track 背景轨道色
 * @param ascii 使用 ASCII 字符
 */
NEFORCE_API void canvas_track(canvas_painter& target, int x, int y, int width, optional<double> value, double maximum,
                              neforce::color fill, neforce::color track, bool ascii = false);

/**
 * @brief 绘制 canvas 垂直滚动条
 * @param target 目标画布
 * @param x 列
 * @param y 行
 * @param height 高度
 * @param content 内容总数
 * @param viewport 视口大小
 * @param position 当前起点
 * @param thumb 滑块色
 * @param track 轨道色
 * @param ascii 使用 ASCII 字符
 */
NEFORCE_API void canvas_scrollbar(canvas_painter& target, int x, int y, int height, size_t content, size_t viewport,
                                  size_t position, neforce::color thumb, neforce::color track, bool ascii = false);

/**
 * @brief Canvas 趋势图渲染器
 */
class NEFORCE_API canvas_chart {
public:
    canvas_chart(canvas_painter& target, canvas_chart_style style);

    /**
     * @brief 绘制纯图表
     * @param x 列
     * @param y 行
     * @param width 宽
     * @param height 高
     * @param series 序列
     * @param maximum 上限
     * @param window 投影窗口
     * @param logarithmic 对数刻度
     * @param direction 0 普通，-1 CPU 上限，1 镜像
     *
     * 无标题/坐标轴。
     */
    void raster(int x, int y, int width, int height, const vector<chart_series>& series, double maximum,
                const canvas_chart_window& window, bool logarithmic = false, int direction = 0);

    /**
     * @brief 使用统一的 chart_spec 绘制流式图
     * @note `spec.series[*].stream` 优先；没有 stream 时使用 `values`。
     */
    void raster(int x, int y, int width, int height, const chart_spec& spec, double maximum,
                const canvas_chart_window& window, bool logarithmic = false, int direction = 0);

private:
    canvas_painter& target_;
    canvas_chart_style style_;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_CANVAS_CHART_HPP__
