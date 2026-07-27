#ifndef NEFORCE_TUI_DOM_TABLE_HPP__
#define NEFORCE_TUI_DOM_TABLE_HPP__

/**
 * @file table.hpp
 * @brief 声明式表格构建器
 *
 * 提供 table 类，从二维数据声明式构建带边框和装饰的表格。
 */

#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 声明式表格构建器
 *
 * @code
 * table t({
 *   {"Name", "Age", "City"},
 *   {"Alice", "30", "NYC"},
 *   {"Bob",   "25", "LA"},
 * });
 * t.select_all().border(enum border::single).separator();
 * element table_el = t.render();
 * @endcode
 */
class NEFORCE_API table {
public:
    /**
     * @brief 从二维字符串构造
     * @param data 表格数据 [row][col]
     */
    explicit table(vector<vector<string>> data);

    /**
     * @brief 从二维 element 构造
     * @param data 表格数据
     */
    explicit table(vector<vector<element>> data);

    /**
     * @brief 全选所有单元格
     * @return 自身引用
     */
    table& select_all();

    /**
     * @brief 选择指定行
     * @param row 行索引
     * @return 自身引用
     */
    table& select_row(int row);

    /**
     * @brief 选择指定列
     * @param column 列索引
     * @return 自身引用
     */
    table& select_column(int column);

    /**
     * @brief 选择矩形区域
     * @param col_min 起始列
     * @param col_max 结束列
     * @param row_min 起始行
     * @param row_max 结束行
     * @return 自身引用
     */
    table& select_rectangle(int col_min, int col_max, int row_min, int row_max);

    /**
     * @brief 对当前选择应用边框
     * @param border 边框样式
     * @return 自身引用
     */
    table& border(enum class style::border border);

    /**
     * @brief 对当前选择添加分隔线（行之间）
     * @return 自身引用
     */
    table& separator();

    /**
     * @brief 对当前选择应用装饰器
     * @param decorator 装饰器
     * @return 自身引用
     */
    table& decorate(const decorator& decorator);

    /**
     * @brief 隔行应用装饰器
     * @param even 偶数行装饰器
     * @param odd 奇数行装饰器
     * @return 自身引用
     */
    table& decorate_alternate_row(const decorator& even, const decorator& odd);

    /**
     * @brief 构建表格元素
     * @return element
     */
    NEFORCE_NODISCARD element render() const;

private:
    vector<vector<element>> cells_;
    int rows_ = 0;
    int cols_ = 0;
    vector<vector<bool>> selected_;
    enum class style::border selection_border_ = style::border::none;
    bool separator_ = false;
    decorator decorator_;
    decorator even_decorator_;
    decorator odd_decorator_;
    bool alt_row_ = false;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_TABLE_HPP__
