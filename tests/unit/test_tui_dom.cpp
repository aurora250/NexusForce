#include <NeForce/tui/dom/element.hpp>
#include <NeForce/tui/dom/graph.hpp>
#include <NeForce/tui/dom/layout_types.hpp>
#include <NeForce/tui/dom/linear_gradient.hpp>
#include <NeForce/tui/dom/scroll_indicator.hpp>
#include <NeForce/tui/dom/style.hpp>
#include <NeForce/tui/dom/table.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;


TEST(TuiLayoutRectTest, DefaultsToZero) {
    const layout_rect rect;

    EXPECT_EQ(rect.x, 0);
    EXPECT_EQ(rect.y, 0);
    EXPECT_EQ(rect.w, 0);
    EXPECT_EQ(rect.h, 0);
}

TEST(TuiLayoutRectTest, AggregateInitialization) {
    const layout_rect rect{3, 4, 10, 6};

    EXPECT_EQ(rect.x, 3);
    EXPECT_EQ(rect.y, 4);
    EXPECT_EQ(rect.w, 10);
    EXPECT_EQ(rect.h, 6);
}

TEST(TuiLayoutRectTest, DimensionsAreMutable) {
    layout_rect rect;
    rect.w = 12;
    rect.h = 5;

    EXPECT_EQ(rect.w, 12);
    EXPECT_EQ(rect.h, 5);
}


TEST(TuiLinearGradientTest, DefaultGradientRunsTransparentToBlack) {
    const linear_gradient gradient;

    EXPECT_TRUE(gradient.sample(0.0F).is_transparent());

    const neforce::color end = gradient.sample(1.0F);
    EXPECT_EQ(end.R(), 0);
    EXPECT_EQ(end.G(), 0);
    EXPECT_EQ(end.B(), 0);
}

TEST(TuiLinearGradientTest, EndpointsMatchConfiguredColors) {
    const linear_gradient gradient(neforce::color(10, 20, 30), neforce::color(200, 100, 50));

    const neforce::color begin = gradient.sample(0.0F);
    EXPECT_EQ(begin.R(), 10);
    EXPECT_EQ(begin.G(), 20);
    EXPECT_EQ(begin.B(), 30);

    const neforce::color end = gradient.sample(1.0F);
    EXPECT_EQ(end.R(), 200);
    EXPECT_EQ(end.G(), 100);
    EXPECT_EQ(end.B(), 50);
}

TEST(TuiLinearGradientTest, MidpointIsInterpolated) {
    const linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(100, 200, 50));

    const neforce::color mid = gradient.sample(0.5F);
    EXPECT_EQ(mid.R(), 50);
    EXPECT_EQ(mid.G(), 100);
    EXPECT_EQ(mid.B(), 25);
}

TEST(TuiLinearGradientTest, SampleBelowRangeClampsToStart) {
    const linear_gradient gradient(neforce::color(10, 20, 30), neforce::color(200, 100, 50));

    const neforce::color below = gradient.sample(-1.0F);
    EXPECT_EQ(below.R(), 10);
    EXPECT_EQ(below.G(), 20);
    EXPECT_EQ(below.B(), 30);
}

TEST(TuiLinearGradientTest, SampleAboveRangeClampsToEnd) {
    const linear_gradient gradient(neforce::color(10, 20, 30), neforce::color(200, 100, 50));

    const neforce::color above = gradient.sample(2.0F);
    EXPECT_EQ(above.R(), 200);
    EXPECT_EQ(above.G(), 100);
    EXPECT_EQ(above.B(), 50);
}

TEST(TuiLinearGradientTest, AngleIsChainable) {
    linear_gradient gradient;
    linear_gradient& returned = gradient.angle(45.0F);

    EXPECT_EQ(&returned, &gradient);
}

TEST(TuiLinearGradientTest, AddStopIsChainable) {
    linear_gradient gradient;
    linear_gradient& returned = gradient.add_stop(neforce::color(1, 2, 3), 0.5F);

    EXPECT_EQ(&returned, &gradient);
}

TEST(TuiLinearGradientTest, TwoStopGradientInterpolatesAcrossTheWholeRange) {
    linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(255, 255, 255));

    const neforce::color quarter = gradient.sample(0.25F);
    EXPECT_EQ(quarter.R(), 64);
    EXPECT_EQ(quarter.G(), 64);
    EXPECT_EQ(quarter.B(), 64);

    const neforce::color three_quarters = gradient.sample(0.75F);
    EXPECT_EQ(three_quarters.R(), 191);
}

TEST(TuiLinearGradientTest, MidGradientStopIsReached) {
    linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(255, 255, 255));
    gradient.add_stop(neforce::color(255, 0, 0), 0.5F);

    const neforce::color at_stop = gradient.sample(0.5F);
    EXPECT_EQ(at_stop.R(), 255);
    EXPECT_EQ(at_stop.G(), 0);
    EXPECT_EQ(at_stop.B(), 0);
}

TEST(TuiLinearGradientTest, MidGradientStopRescalesBothSegments) {
    linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(255, 255, 255));
    gradient.add_stop(neforce::color(255, 0, 0), 0.5F);

    const neforce::color quarter = gradient.sample(0.25F);
    EXPECT_EQ(quarter.R(), 128);
    EXPECT_EQ(quarter.G(), 0);
    EXPECT_EQ(quarter.B(), 0);

    const neforce::color three_quarters = gradient.sample(0.75F);
    EXPECT_EQ(three_quarters.R(), 255);
    EXPECT_EQ(three_quarters.G(), 128);
    EXPECT_EQ(three_quarters.B(), 128);
}

TEST(TuiLinearGradientTest, StopsAreInsertedInAscendingPositionOrder) {
    linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(255, 255, 255));
    gradient.add_stop(neforce::color(0, 255, 0), 0.75F);
    gradient.add_stop(neforce::color(255, 0, 0), 0.25F);

    // Inserted out of order: red at 0.25 first, green at 0.75 second, so the
    // resulting sequence must be black, red, green, white.
    const neforce::color at_quarter = gradient.sample(0.25F);
    EXPECT_EQ(at_quarter.R(), 255);
    EXPECT_EQ(at_quarter.G(), 0);

    const neforce::color at_three_quarters = gradient.sample(0.75F);
    EXPECT_EQ(at_three_quarters.R(), 0);
    EXPECT_EQ(at_three_quarters.G(), 255);
}

TEST(TuiLinearGradientTest, UnpositionedStopTakesThePreviousPosition) {
    linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(255, 255, 255));
    gradient.add_stop(neforce::color(255, 0, 0));

    EXPECT_EQ(gradient.sample(1.0F).R(), 255);
    EXPECT_EQ(gradient.sample(1.0F).G(), 0);
    EXPECT_EQ(gradient.sample(1.0F).B(), 0);

    EXPECT_EQ(gradient.sample(0.5F).R(), 128);
    EXPECT_EQ(gradient.sample(0.5F).G(), 128);
}

TEST(TuiLinearGradientTest, ZeroLengthSegmentPrefersTheLaterStop) {
    linear_gradient gradient(neforce::color(0, 0, 0), neforce::color(255, 255, 255));
    gradient.add_stop(neforce::color(0, 255, 0), 0.0F);

    const neforce::color at_zero = gradient.sample(0.0F);
    EXPECT_EQ(at_zero.R(), 0);
    EXPECT_EQ(at_zero.G(), 255);
}


TEST(TuiGraphTest, EmptyDataProducesBlankLine) {
    const element el = graph(vector<int>{}, 6, 3);

    EXPECT_EQ(el.kind(), element::kind::text);
    EXPECT_EQ(el.text(), string(6, ' '));
}

TEST(TuiGraphTest, RendersOneRowPerHeightUnit) {
    const element el = graph(vector<int>{1, 2, 3}, 6, 4);

    ASSERT_EQ(el.kind(), element::kind::vbox);
    EXPECT_EQ(el.children().size(), 4U);
}

TEST(TuiGraphTest, ConstantDataDoesNotDivideByZero) {
    const element el = graph(vector<int>{7, 7, 7, 7}, 4, 3);

    ASSERT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 3U);
    // With a clamped range every sample maps to row 0, so only the bottom row is filled.
    EXPECT_NE(el.children()[2].text().find("█"), string::npos);
}

TEST(TuiGraphTest, IntermediateValuesLandOnIntermediateRows) {
    const element el = graph(vector<int>{0, 5, 10}, 3, 5);

    ASSERT_EQ(el.children().size(), 5U);
    EXPECT_EQ(el.children()[2].text(), " ██");
}

TEST(TuiGraphTest, FunctionOverloadUsesProvidedSamples) {
    const element el = graph(
            [](const int width, const int height) {
                ignore = height;
                return vector<int>{0, 0, 0, 0};
            },
            4, 3);

    ASSERT_EQ(el.kind(), element::kind::vbox);
    EXPECT_EQ(el.children().size(), 3U);
    EXPECT_NE(el.children()[2].text().find("█"), string::npos);
}


TEST(TuiScrollIndicatorTest, VerticalIndicatorShowsThumbProportionalToVisibleRows) {
    const element el = vscroll_indicator(100, 50, 0, 10);

    ASSERT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 10U);

    size_t thumb_rows = 0;
    for (const auto& row: el.children()) {
        if (row.text() == "█") {
            ++thumb_rows;
        }
    }
    EXPECT_EQ(thumb_rows, 5U);
}

TEST(TuiScrollIndicatorTest, VerticalThumbMovesWithOffset) {
    const element top = vscroll_indicator(100, 50, 0, 10);
    const element bottom = vscroll_indicator(100, 50, 50, 10);

    EXPECT_EQ(top.children()[0].text(), "█");
    EXPECT_EQ(top.children()[9].text(), "│");

    EXPECT_EQ(bottom.children()[0].text(), "│");
    EXPECT_EQ(bottom.children()[9].text(), "█");
}

TEST(TuiScrollIndicatorTest, VerticalIndicatorIsBlankWhenEverythingFits) {
    const element el = vscroll_indicator(5, 10, 0, 4);

    EXPECT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 4U);
    for (const auto& child: el.children()) {
        EXPECT_EQ(child.kind(), element::kind::text);
        EXPECT_EQ(child.text(), " ");
    }
}

TEST(TuiScrollIndicatorTest, HorizontalIndicatorShowsThumbProportionalToVisibleColumns) {
    const element el = hscroll_indicator(100, 50, 0, 10);

    EXPECT_EQ(el.kind(), element::kind::text);
    EXPECT_EQ(el.text(), "█████─────");
}

TEST(TuiScrollIndicatorTest, HorizontalThumbMovesWithOffset) {
    const element el = hscroll_indicator(100, 50, 50, 10);

    EXPECT_EQ(el.text(), "─────█████");
}

TEST(TuiScrollIndicatorTest, HorizontalIndicatorIsBlankWhenEverythingFits) {
    const element el = hscroll_indicator(4, 10, 0, 6);

    EXPECT_EQ(el.kind(), element::kind::text);
    EXPECT_EQ(el.text(), string(6, ' '));
}

TEST(TuiScrollIndicatorTest, ThumbIsAtLeastOneCellForTinyVisibleRatio) {
    const element el = vscroll_indicator(1000, 7, 0, 10);

    ASSERT_EQ(el.children().size(), 10U);
    size_t thumb_rows = 0;
    for (const auto& row: el.children()) {
        if (row.text() == "█") {
            ++thumb_rows;
        }
    }
    EXPECT_EQ(thumb_rows, 1U);
}


TEST(TuiTableTest, RendersOneRowPerDataRow) {
    const table t(vector<vector<string>>{{"a", "b"}, {"c", "d"}, {"e", "f"}});
    const element el = t.render();

    ASSERT_EQ(el.kind(), element::kind::vbox);
    ASSERT_EQ(el.children().size(), 3U);
    EXPECT_EQ(el.children()[0].kind(), element::kind::hbox);
    EXPECT_EQ(el.children()[0].children().size(), 2U);
}

TEST(TuiTableTest, CellTextIsPreserved) {
    const table t(vector<vector<string>>{{"Name", "Age"}, {"Alice", "30"}});
    const element el = t.render();

    EXPECT_EQ(el.children()[0].children()[0].text(), "Name");
    EXPECT_EQ(el.children()[1].children()[1].text(), "30");
}

TEST(TuiTableTest, ShortRowsArePaddedToTheWidestRow) {
    const table t(vector<vector<string>>{{"a", "b", "c"}, {"d"}});
    const element el = t.render();

    ASSERT_EQ(el.children().size(), 2U);
    ASSERT_EQ(el.children()[1].children().size(), 3U);
    EXPECT_EQ(el.children()[1].children()[0].text(), "d");
    EXPECT_EQ(el.children()[1].children()[1].text(), "");
    EXPECT_EQ(el.children()[1].children()[2].text(), "");
}

TEST(TuiTableTest, SelectionWithoutBorderAddsNoDecoration) {
    table t(vector<vector<string>>{{"a", "b"}, {"c", "d"}});
    t.select_all();

    const element el = t.render();
    EXPECT_EQ(el.children()[0].children().size(), 2U);
}

TEST(TuiTableTest, BorderDecoratesSelectedCells) {
    table t(vector<vector<string>>{{"a", "b"}, {"c", "d"}});
    t.select_all().border(style::border::single);

    const element el = t.render();
    const auto& row = el.children()[0].children();

    EXPECT_EQ(row[0].text(), "│ ");
    EXPECT_EQ(row[row.size() - 1].text(), " │");
}

TEST(TuiTableTest, DoubleBorderUsesDoubleVerticalBar) {
    table t(vector<vector<string>>{{"a"}});
    t.select_all().border(style::border::double_);

    const element el = t.render();
    EXPECT_EQ(el.children()[0].children()[0].text(), "║ ");
}

TEST(TuiTableTest, SelectRowDecoratesOnlyThatRow) {
    table t(vector<vector<string>>{{"a"}, {"b"}, {"c"}});
    t.select_row(1).border(style::border::single);

    const element el = t.render();

    EXPECT_EQ(el.children()[0].children().size(), 1U);
    EXPECT_GT(el.children()[1].children().size(), 1U);
    EXPECT_EQ(el.children()[2].children().size(), 1U);
}

TEST(TuiTableTest, SelectColumnDecoratesOnlyThatColumn) {
    table t(vector<vector<string>>{{"a", "b"}, {"c", "d"}});
    t.select_column(1).border(style::border::single);

    const element el = t.render();
    EXPECT_EQ(el.children()[0].children().size(), 3U);
    EXPECT_EQ(el.children()[0].children()[2].text(), " │");
}

TEST(TuiTableTest, OutOfRangeSelectionIsClamped) {
    table t(vector<vector<string>>{{"a", "b"}});
    t.select_row(99).select_column(-5).border(style::border::single);

    const element el = t.render();
    EXPECT_GT(el.children()[0].children().size(), 0U);
}

TEST(TuiTableTest, SeparatorInsertsLinesBetweenRows) {
    table t(vector<vector<string>>{{"a"}, {"b"}, {"c"}});
    t.separator();

    const element el = t.render();
    ASSERT_EQ(el.children().size(), 5U);
    EXPECT_EQ(el.children()[1].kind(), element::kind::separator);
    EXPECT_EQ(el.children()[3].kind(), element::kind::separator);
}

TEST(TuiTableTest, SeparatorIsIgnoredForSingleRow) {
    table t(vector<vector<string>>{{"a"}});
    t.separator();

    const element el = t.render();
    EXPECT_EQ(el.children().size(), 1U);
}

TEST(TuiTableTest, DecorateAppliesToEveryCell) {
    table t(vector<vector<string>>{{"a", "b"}, {"c", "d"}});
    t.decorate(bold());

    const element el = t.render();
    for (const auto& row: el.children()) {
        for (const auto& cell: row.children()) {
            EXPECT_TRUE(cell.style().bold.value_or(false));
        }
    }
}

TEST(TuiTableTest, DecorateAlternateRowAppliesEvenAndOddDecorators) {
    table t(vector<vector<string>>{{"even"}, {"odd"}});
    t.decorate_alternate_row(bold(), italic());

    const element el = t.render();

    EXPECT_TRUE(el.children()[0].children()[0].style().bold.value_or(false));
    EXPECT_FALSE(el.children()[0].children()[0].style().italic.value_or(false));

    EXPECT_TRUE(el.children()[1].children()[0].style().italic.value_or(false));
    EXPECT_FALSE(el.children()[1].children()[0].style().bold.value_or(false));
}

TEST(TuiTableTest, ElementDataConstructorKeepsElements) {
    vector<vector<element>> data;
    vector<element> row;
    row.push_back(element::text("custom"));
    data.push_back(move(row));

    const table t(move(data));
    const element el = t.render();

    EXPECT_EQ(el.children()[0].children()[0].text(), "custom");
}

TEST(TuiTableTest, EmptyTableRendersEmptyContainer) {
    const table t(vector<vector<string>>{});
    const element el = t.render();

    EXPECT_EQ(el.kind(), element::kind::vbox);
    EXPECT_TRUE(el.children().empty());
}
