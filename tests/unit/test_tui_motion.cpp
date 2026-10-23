#include <NeForce/tui/component/motion.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;


TEST(TuiFrameClockTest, TickAdvancesTimeAndFrame) {
    auto& clock = frame_clock::instance();
    clock.reset();
    clock.set_paused(false);
    EXPECT_EQ(clock.now_ms(), 0);
    clock.tick(16);
    clock.tick(16);
    EXPECT_EQ(clock.now_ms(), 32);
    EXPECT_EQ(clock.frame_index(), 2);
    clock.reset();
}

TEST(TuiFrameClockTest, PausedClockFreezes) {
    auto& clock = frame_clock::instance();
    clock.reset();
    clock.set_paused(true);
    clock.tick(16);
    EXPECT_EQ(clock.now_ms(), 0);
    EXPECT_EQ(clock.frame_index(), 0);
    clock.set_paused(false);
    clock.tick(16);
    EXPECT_EQ(clock.now_ms(), 16);
    clock.reset();
}

TEST(TuiShimmerTest, ColumnOutsideWindowKeepsBase) {
    const color base{10, 20, 30};
    const color highlight{200, 210, 220};
    const color c = shimmer_color(0, 40, 0, base, highlight);
    EXPECT_EQ(c.to_string(), base.to_string());
}

TEST(TuiShimmerTest, WindowSweepsAcrossColumns) {
    const color base{0, 0, 0};
    const color highlight{255, 255, 255};
    // The window starts ten columns left of the text and reaches column 0 after
    // ten steps of 60 ms.
    EXPECT_EQ(shimmer_color(0, 40, 0, base, highlight).to_string(), base.to_string());
    EXPECT_NE(shimmer_color(0, 40, 60 * 10, base, highlight).to_string(), base.to_string());
    // The window also visits every column again one cycle later.
    const int cycle = 40 + 2 * 10;
    EXPECT_NE(shimmer_color(3, 40, 60 * (10 + cycle), base, highlight).to_string(), base.to_string());
}

TEST(TuiShimmerTest, SweepCoversEveryColumn) {
    const vector<color> palette = shimmer_sweep(20, 1234, color{0, 0, 0}, color{255, 255, 255});
    ASSERT_EQ(palette.size(), 20u);
    bool any_lit = false;
    for (const auto& c: palette) {
        if (c.R() != 0) {
            any_lit = true;
        }
    }
    EXPECT_TRUE(any_lit);
}

TEST(TuiShimmerTest, EmptyWidthProducesNothing) {
    EXPECT_TRUE(shimmer_sweep(0, 10, color{}, color{}).empty());
    EXPECT_TRUE(shimmer_sweep(-3, 10, color{}, color{}).empty());
}

TEST(TuiRevealTest, FirstStepCatchesQuarterOfBacklog) {
    reveal r;
    EXPECT_EQ(r.advance(100), 13u);
    EXPECT_FALSE(r.done(100));
}

TEST(TuiRevealTest, SettlesOnLinearTailThenStops) {
    reveal r;
    size_t shown = 0;
    for (int i = 0; i < 20; ++i) {
        shown = r.advance(2000);
        EXPECT_LE(shown, 2000u);
    }
    // Exponential catch-up removes ~1/8 of the backlog per frame; the minimum
    // step of three then finishes the tail linearly.
    EXPECT_GT(shown, 1800u);
    for (int i = 0; i < 64; ++i) {
        shown = r.advance(2000);
    }
    EXPECT_EQ(shown, 2000u);
    EXPECT_TRUE(r.done(2000));
}

TEST(TuiRevealTest, NeverRunsAheadOfArrived) {
    reveal r;
    EXPECT_EQ(r.advance(2), 2u);
    EXPECT_EQ(r.advance(0), 0u);
    EXPECT_TRUE(r.done(0));
}

TEST(TuiRevealTest, ResetJumpsToArrived) {
    reveal r;
    r.reset(50);
    EXPECT_EQ(r.visible(), 50u);
    EXPECT_TRUE(r.done(50));
}

TEST(TuiRevealTest, SmallBacklogUsesMinimumStep) {
    reveal r;
    EXPECT_EQ(r.advance(1), 1u);
    reveal fresh;
    EXPECT_EQ(fresh.advance(10), 3u);
    EXPECT_EQ(fresh.advance(10), 6u);
}

TEST(TuiSpinnerTest, FindsKnownSets) {
    EXPECT_NE(find_spinner(string_view{"braille"}), nullptr);
    EXPECT_NE(find_spinner(string_view{"dots"}), nullptr);
    EXPECT_EQ(find_spinner(string_view{"nope"}), nullptr);
    EXPECT_EQ(find_spinner(string_view{"braille"})->count, 10u);
    EXPECT_EQ(find_spinner(string_view{"braille"})->interval_ms, 80);
}

TEST(TuiSpinnerTest, FrameAdvancesWithTime) {
    const auto* braille = find_spinner(string_view{"braille"});
    ASSERT_NE(braille, nullptr);
    const string_view first = spinner_frame(*braille, 0);
    const string_view second = spinner_frame(*braille, 80);
    EXPECT_NE(first, second);
    EXPECT_EQ(spinner_frame(*braille, 80 * 10), first);
}
