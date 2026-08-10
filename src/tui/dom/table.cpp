#include <NeForce/tui/dom/table.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

table::table(vector<vector<string>> data) {
    rows_ = static_cast<int>(data.size());
    cols_ = 0;
    for (const auto& row: data) {
        cols_ = max(static_cast<int>(row.size()), cols_);
    }
    cells_.resize(rows_);
    selected_.resize(rows_);
    for (int r = 0; r < rows_; ++r) {
        cells_[r].resize(cols_);
        selected_[r].resize(cols_, false);
        for (int c = 0; c < cols_; ++c) {
            if (c < static_cast<int>(data[r].size())) {
                cells_[r][c] = element::text(data[r][c]);
            } else {
                cells_[r][c] = element::text("");
            }
        }
    }
}

table::table(vector<vector<element>> data) {
    rows_ = static_cast<int>(data.size());
    cols_ = 0;
    for (const auto& row: data) {
        cols_ = max(static_cast<int>(row.size()), cols_);
    }
    cells_.resize(rows_);
    selected_.resize(rows_);
    for (int r = 0; r < rows_; ++r) {
        cells_[r].resize(cols_);
        selected_[r].resize(cols_, false);
        for (int c = 0; c < cols_; ++c) {
            if (c < static_cast<int>(data[r].size())) {
                cells_[r][c] = data[r][c];
            } else {
                cells_[r][c] = element::text("");
            }
        }
    }
}

table& table::select_all() { return select_rectangle(0, cols_ - 1, 0, rows_ - 1); }

table& table::select_row(const int row) { return select_rectangle(0, cols_ - 1, row, row); }

table& table::select_column(const int column) { return select_rectangle(column, column, 0, rows_ - 1); }

table& table::select_rectangle(const int col_min, const int col_max, const int row_min, const int row_max) {
    const int cmn = (col_min < 0) ? 0 : col_min;
    const int cmx = (col_max >= cols_) ? cols_ - 1 : col_max;
    const int rmn = (row_min < 0) ? 0 : row_min;
    const int rmx = (row_max >= rows_) ? rows_ - 1 : row_max;
    for (int r = rmn; r <= rmx && r < rows_; ++r) {
        for (int c = cmn; c <= cmx && c < cols_; ++c) {
            selected_[r][c] = true;
        }
    }
    return *this;
}

table& table::border(const enum style::border border) {
    selection_border_ = border;
    return *this;
}

table& table::separator() {
    separator_ = true;
    return *this;
}

table& table::decorate(const decorator& decorator) {
    decorator_ = decorator;
    return *this;
}

table& table::decorate_alternate_row(const decorator& even, const decorator& odd) {
    even_decorator_ = even;
    odd_decorator_ = odd;
    alt_row_ = true;
    return *this;
}

element table::render() const {
    const string vbar = (selection_border_ == style::border::double_) ? "\xe2\x95\x91" : "\xe2\x94\x82"; // ║ or │

    elements rows;
    for (int r = 0; r < rows_; ++r) {
        elements row_cells;
        for (int c = 0; c < cols_; ++c) {
            element cell = cells_[r][c];
            if (decorator_) {
                cell = decorator_(cell);
            }
            if (alt_row_) {
                if (r % 2 == 0 && even_decorator_) {
                    cell = even_decorator_(cell);
                } else if (r % 2 != 0 && odd_decorator_) {
                    cell = odd_decorator_(cell);
                }
            }

            if (selected_[r][c] && selection_border_ != style::border::none) {
                if (c == 0) {
                    row_cells.push_back(element::text(vbar + " "));
                }
                row_cells.push_back(cell);
                if (c < cols_ - 1) {
                    if (selected_[r][c + 1]) {
                        row_cells.push_back(element::text(" " + vbar + " "));
                    }
                } else {
                    row_cells.push_back(element::text(" " + vbar));
                }
            } else {
                row_cells.push_back(cell);
            }
        }
        rows.push_back(element::hbox(move(row_cells)));
    }

    if (separator_ && rows_ > 1) {
        elements with_seps;
        for (int r = 0; r < rows_; ++r) {
            with_seps.push_back(rows[r]);
            if (r < rows_ - 1) {
                with_seps.push_back(element::separator());
            }
        }
        return element::vbox(move(with_seps));
    }

    return element::vbox(move(rows));
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
