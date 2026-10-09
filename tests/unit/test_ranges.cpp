#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/iterator/ranges.hpp>
#include <NeForce/core/functional/function.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/pair.hpp>
#include <NeForce/core/utility/tuple.hpp>
#include <gtest/gtest.h>
using namespace neforce;
#ifdef NEFORCE_STANDARD_20

TEST(RangesTest, AllLvalue) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto v = ranges::all(vec);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[4], 5);
}

TEST(RangesTest, AllRvalue) {
    namespace rv = ranges::views;
    auto v = ranges::all(vector<int>{1, 2, 3});
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[2], 3);
}

TEST(RangesTest, AllViewPassthrough) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto fv = vec | rv::filter([](int x) { return true; });
    auto v = ranges::all(fv);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, FilterViewCopy) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto fv1 = vec | rv::filter([](int x) { return x % 2 == 0; });
    auto fv2 = fv1;
    vector<int> result;
    for (auto x: fv2) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[2], 6);
}

TEST(RangesTest, FilterViewMove) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4};
    auto fv1 = vec | rv::filter([](int x) { return x > 2; });
    auto fv2 = move(fv1);
    vector<int> result;
    for (auto x: fv2) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[1], 4);
}

TEST(RangesTest, FilterViewAssign) {
    namespace rv = ranges::views;
    vector<int> vec1 = {1, 2, 3, 4};
    vector<int> vec2 = {10, 20, 30, 40};
    auto pred = [](int x) { return x % 2 == 0; };
    auto fv1 = vec1 | rv::filter(pred);
    auto fv2 = vec2 | rv::filter(pred);
    fv2 = fv1;
    vector<int> result;
    for (auto x: fv2) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[1], 4);
}

TEST(RangesTest, FilterViewBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4};
    auto fv = vec | rv::filter([](int x) { return x > 1; });
    auto base = fv.base().base();
    ASSERT_EQ(base.size(), 4);
    ASSERT_EQ(base[0], 1);
    ASSERT_EQ(base[3], 4);
}

TEST(RangesTest, FilterViewEmpty) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto fv = vec | rv::filter([](int x) { return x > 10; });
    vector<int> result;
    for (auto x: fv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, FilterIteratorPostIncrement) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto fv = vec | rv::filter([](int x) { return x % 2 == 0; });
    auto it = fv.begin();
    auto old = it++;
    ASSERT_EQ(*old, 2);
    ASSERT_EQ(*it, 4);
}

TEST(RangesTest, FilterBidirectionalBackward) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto fv = vec | rv::filter([](int x) { return x % 2 == 0; });
    auto it = fv.end();
    --it;
    ASSERT_EQ(*it, 6);
    --it;
    ASSERT_EQ(*it, 4);
    --it;
    ASSERT_EQ(*it, 2);
}

TEST(RangesTest, FilterCommonRangeEnd) {
    namespace rv = ranges::views;
    vector<int> vec = {2, 4, 6, 8};
    auto fv = vec | rv::filter([](int x) { return x > 0; });
    auto it = fv.end();
    auto begin = fv.begin();
    ASSERT_NE(it, begin);
}

TEST(RangesTest, FilterConstIteration) {
    namespace rv = ranges::views;
    const vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto fv = vec | rv::filter([](int x) { return x % 3 == 0; });
    vector<int> result;
    for (auto x: fv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[1], 6);
}

TEST(RangesTest, TransformViewCopy) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto tv1 = vec | rv::transform([](int x) { return x * 2; });
    auto tv2 = tv1;
    vector<int> result;
    for (auto x: tv2) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[2], 6);
}

TEST(RangesTest, TransformViewBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto tv = vec | rv::transform([](int x) { return x * 10; });
    auto base = tv.base().base();
    ASSERT_EQ(base.size(), 3);
    ASSERT_EQ(base[2], 3);
}

TEST(RangesTest, TransformChained) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto tv = vec | rv::transform([](int x) { return x + 1; }) | rv::transform([](int x) { return x * 10; });
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 20);
    ASSERT_EQ(result[1], 30);
    ASSERT_EQ(result[2], 40);
}

TEST(RangesTest, TransformConstIteration) {
    namespace rv = ranges::views;
    const vector<int> vec = {1, 2, 3};
    auto tv = vec | rv::transform([](int x) { return x * 5; });
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 5);
    ASSERT_EQ(result[2], 15);
}

TEST(RangesTest, TakeViewAllElements) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto tv = vec | rv::take(10);
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
}

TEST(RangesTest, TakeViewZero) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto tv = vec | rv::take(0);
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, TakeViewBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto tv = vec | rv::take(3);
    auto base = tv.base().base();
    ASSERT_EQ(base.size(), 5);
}

TEST(RangesTest, TakeViewConst) {
    namespace rv = ranges::views;
    const vector<int> vec = {1, 2, 3, 4, 5, 6, 7};
    auto tv = vec | rv::take(4);
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[3], 4);
}

TEST(RangesTest, TakeWhileEmptyPred) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto twv = vec | rv::take_while([](int x) { return false; });
    vector<int> result;
    for (auto x: twv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, TakeWhileBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto twv = vec | rv::take_while([](int x) { return x < 4; });
    auto base = twv.base().base();
    ASSERT_EQ(base.size(), 5);
}

TEST(RangesTest, TakeWhileConst) {
    namespace rv = ranges::views;
    const vector<int> vec = {2, 4, 6, 8, 10};
    auto twv = vec | rv::take_while([](int x) { return x < 7; });
    vector<int> result;
    for (auto x: twv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[2], 6);
}

TEST(RangesTest, DropViewAll) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto dv = vec | rv::drop(10);
    vector<int> result;
    for (auto x: dv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, DropViewNone) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto dv = vec | rv::drop(0);
    vector<int> result;
    for (auto x: dv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, DropViewBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto dv = vec | rv::drop(2);
    auto base = dv.base().base();
    ASSERT_EQ(base.size(), 5);
}

TEST(RangesTest, DropViewConst) {
    namespace rv = ranges::views;
    const vector<int> vec = {10, 20, 30, 40, 50};
    auto dv = vec | rv::drop(3);
    vector<int> result;
    for (auto x: dv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 40);
    ASSERT_EQ(result[1], 50);
}

TEST(RangesTest, DropWhileAllMatch) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto dwv = vec | rv::drop_while([](int x) { return x < 10; });
    vector<int> result;
    for (auto x: dwv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, DropWhileNoneMatch) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto dwv = vec | rv::drop_while([](int x) { return x < 0; });
    vector<int> result;
    for (auto x: dwv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
}

TEST(RangesTest, DropWhileBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto dwv = vec | rv::drop_while([](int x) { return x < 3; });
    auto base = dwv.base().base();
    ASSERT_EQ(base.size(), 5);
}

TEST(RangesTest, DropWhileConst) {
    namespace rv = ranges::views;
    const vector<int> vec = {5, 10, 15, 20, 25};
    auto dwv = vec | rv::drop_while([](int x) { return x < 12; });
    vector<int> result;
    for (auto x: dwv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 15);
}

TEST(RangesTest, ReverseViewEmpty) {
    namespace rv = ranges::views;
    vector<int> vec;
    auto rv_view = vec | rv::reverse();
    vector<int> result;
    for (auto x: rv_view) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, ReverseViewBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4};
    auto rv_view = vec | rv::reverse();
    auto base = rv_view.base().base();
    ASSERT_EQ(base.size(), 4);
}

TEST(RangesTest, ReverseConst) {
    namespace rv = ranges::views;
    const vector<int> vec = {10, 20, 30};
    auto rv_view = vec | rv::reverse();
    vector<int> result;
    for (auto x: rv_view) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 30);
    ASSERT_EQ(result[2], 10);
}

TEST(RangesTest, IotaViewIncrement) {
    auto iv = ranges::views::iota(0, 5);
    vector<int> result;
    for (auto x: iv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 0);
    ASSERT_EQ(result[4], 4);
}

TEST(RangesTest, IotaViewUnboundedTake) {
    auto iv = ranges::views::iota(100);
    auto tv = iv | ranges::views::take(3);
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 100);
    ASSERT_EQ(result[2], 102);
}

TEST(RangesTest, IotaViewPostIncrement) {
    auto iv = ranges::views::iota(1, 4);
    auto it = iv.begin();
    auto old = it++;
    ASSERT_EQ(*old, 1);
    ASSERT_EQ(*it, 2);
}

TEST(RangesTest, RepeatViewBounded) {
    auto rv = ranges::views::repeat(42, 4);
    vector<int> result;
    for (auto x: rv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    for (int v: result) {
        ASSERT_EQ(v, 42);
    }
}

TEST(RangesTest, RepeatViewUnboundedTake) {
    auto rv = ranges::views::repeat(7);
    auto tv = rv | ranges::views::take(3);
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 7);
    ASSERT_EQ(result[2], 7);
}

TEST(RangesTest, RepeatViewPostIncrement) {
    auto rv = ranges::views::repeat(99, 3);
    auto it = rv.begin();
    auto old = it++;
    ASSERT_EQ(*old, 99);
    ASSERT_EQ(*it, 99);
}

TEST(RangesTest, JoinViewNested) {
    namespace rv = ranges::views;
    vector<vector<int>> nested = {{1, 2}, {3, 4, 5}, {6}};
    auto jv = nested | rv::join();
    vector<int> result;
    for (auto x: jv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 6);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[5], 6);
}

TEST(RangesTest, JoinViewEmptyInner) {
    namespace rv = ranges::views;
    vector<vector<int>> nested = {{}, {1, 2}, {}, {3}};
    auto jv = nested | rv::join();
    vector<int> result;
    for (auto x: jv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[2], 3);
}

TEST(RangesTest, JoinViewAllEmpty) {
    namespace rv = ranges::views;
    vector<vector<int>> nested = {{}, {}};
    auto jv = nested | rv::join();
    vector<int> result;
    for (auto x: jv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, JoinViewBase) {
    namespace rv = ranges::views;
    vector<vector<int>> nested = {{1}, {2, 3}};
    auto jv = nested | rv::join();
    auto base = jv.base().base();
    ASSERT_EQ(base.size(), 2);
}

TEST(RangesTest, JoinConst) {
    namespace rv = ranges::views;
    const vector<vector<int>> nested = {{10, 20}, {30}};
    auto jv = nested | rv::join();
    vector<int> result;
    for (auto x: jv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[2], 30);
}

TEST(RangesTest, ElementsKeysValues) {
    namespace rv = ranges::views;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto keys_view = vec | rv::keys();
    vector<int> keys;
    for (auto k: keys_view) {
        keys.push_back(k);
    }
    ASSERT_EQ(keys.size(), 3);
    ASSERT_EQ(keys[0], 1);
    ASSERT_EQ(keys[2], 3);

    auto values_view = vec | rv::values();
    vector<string> values;
    for (auto v: values_view) {
        values.push_back(v);
    }
    ASSERT_EQ(values.size(), 3);
    ASSERT_EQ(values[0], "one");
    ASSERT_EQ(values[2], "three");
}

TEST(RangesTest, ElementsConst) {
    namespace rv = ranges::views;
    const vector<tuple<int, double>> vec = {{1, 1.5}, {2, 2.5}};
    auto e0 = vec | rv::elements<0>();
    vector<int> result;
    for (auto x: e0) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[1], 2);
}

TEST(RangesTest, CommonViewNonCommon) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto fv = vec | rv::filter([](int x) { return x % 2 == 0; });
    auto cv = fv | rv::common();
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[1], 4);
}

TEST(RangesTest, CommonViewAlreadyCommon) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto cv = vec | rv::common();
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, CommonViewBase) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto fv = vec | rv::filter([](int x) { return x > 1; });
    auto cv = fv | rv::common();
    auto base = cv.base().base();
    ASSERT_EQ(base.size(), 3);
}

TEST(RangesTest, CountedViewBasic) {
    namespace rv = ranges::views;
    vector<int> vec = {10, 20, 30, 40, 50};
    auto cv = rv::counted(vec.begin(), 3);
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[2], 30);
}

TEST(RangesTest, CountedViewZero) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto cv = rv::counted(vec.begin(), 0);
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, CountedViewPostIncrement) {
    namespace rv = ranges::views;
    vector<int> vec = {5, 6, 7};
    auto cv = rv::counted(vec.begin(), 3);
    auto it = cv.begin();
    auto old = it++;
    ASSERT_EQ(*old, 5);
    ASSERT_EQ(*it, 6);
    ASSERT_EQ(it.count(), 2);
}

TEST(RangesTest, SplitViewChar) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 0, 2, 0, 3};
    auto sv = vec | rv::split(0);
    vector<vector<int>> result;
    for (auto subrange: sv) {
        vector<int> chunk;
        for (auto x: subrange) {
            chunk.push_back(x);
        }
        result.push_back(chunk);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].size(), 1);
    ASSERT_EQ(result[0][0], 1);
    ASSERT_EQ(result[1].size(), 1);
    ASSERT_EQ(result[1][0], 2);
    ASSERT_EQ(result[2].size(), 1);
    ASSERT_EQ(result[2][0], 3);
}

TEST(RangesTest, SplitViewNoDelimiter) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto sv = vec | rv::split(0);
    vector<vector<int>> result;
    for (auto subrange: sv) {
        vector<int> chunk;
        for (auto x: subrange) {
            chunk.push_back(x);
        }
        result.push_back(chunk);
    }
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].size(), 3);
}

TEST(RangesTest, SplitViewLeadingDelimiter) {
    namespace rv = ranges::views;
    vector<int> vec = {0, 1, 2, 0, 3};
    auto sv = vec | rv::split(0);
    vector<vector<int>> result;
    for (auto subrange: sv) {
        vector<int> chunk;
        for (auto x: subrange) {
            chunk.push_back(x);
        }
        result.push_back(chunk);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_TRUE(result[0].empty());
    ASSERT_EQ(result[1].size(), 2);
    ASSERT_EQ(result[2].size(), 1);
}

TEST(RangesTest, SplitViewTrailingDelimiter) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 0, 2, 0};
    auto sv = vec | rv::split(0);
    vector<vector<int>> result;
    for (auto subrange: sv) {
        vector<int> chunk;
        for (auto x: subrange) {
            chunk.push_back(x);
        }
        result.push_back(chunk);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].size(), 1);
    ASSERT_EQ(result[1].size(), 1);
    ASSERT_TRUE(result[2].empty());
}

TEST(RangesTest, SliceViewBasic) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    auto sv = vec | rv::slice(2, 4);
    vector<int> result;
    for (auto x: sv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[1], 4);
}

TEST(RangesTest, SliceViewZeroLength) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto sv = vec | rv::slice(1, 0);
    vector<int> result;
    for (auto x: sv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, SliceViewExceedsRange) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto sv = vec | rv::slice(1, 10);
    vector<int> result;
    for (auto x: sv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[1], 3);
}

TEST(RangesTest, SliceConst) {
    namespace rv = ranges::views;
    const vector<int> vec = {10, 20, 30, 40, 50};
    auto sv = vec | rv::slice(1, 4);
    vector<int> result;
    for (auto x: sv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 20);
    ASSERT_EQ(result[2], 40);
}

TEST(RangesTest, ConcatViewEmpty) {
    namespace rv = ranges::views;
    vector<int> a;
    vector<int> b;
    auto cv = a | rv::concat(b);
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_TRUE(result.empty());
}

TEST(RangesTest, ConcatViewFirstEmpty) {
    namespace rv = ranges::views;
    vector<int> a;
    vector<int> b = {1, 2, 3};
    auto cv = a | rv::concat(b);
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 1);
}

TEST(RangesTest, ConcatViewSecondEmpty) {
    namespace rv = ranges::views;
    vector<int> a = {1, 2, 3};
    vector<int> b;
    auto cv = a | rv::concat(b);
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[2], 3);
}

TEST(RangesTest, ConcatViewChainedWithTransform) {
    namespace rv = ranges::views;
    vector<int> a = {1, 2};
    vector<int> b = {3, 4};
    auto cv = a | rv::concat(b) | rv::transform([](int x) { return x * 2; });
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[3], 8);
}

TEST(RangesTest, AdaptorClosureComposition) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto composed = rv::filter([](int x) { return x % 2 == 0; }) | rv::transform([](int x) { return x * 10; });
    auto v = vec | composed;
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 20);
    ASSERT_EQ(result[1], 40);
    ASSERT_EQ(result[2], 60);
}

TEST(RangesTest, AdaptorDefaultConstructed) {
    namespace rv = ranges::views;
    auto adaptor = rv::reverse();
    vector<int> vec = {1, 2, 3};
    auto rv_view = vec | adaptor;
    vector<int> result;
    for (auto x: rv_view) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[2], 1);
}

TEST(RangesTest, PipeSyntaxMultipleAdaptors) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    auto v = vec | rv::filter([](int x) { return x > 3; }) | rv::take(3) | rv::transform([](int x) { return x * 2; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 8);
    ASSERT_EQ(result[2], 12);
}

TEST(RangesTest, ViewDefaultConstruction) {
    using ref_t = ranges::ref_view<vector<int>>;
    ranges::filter_view<ref_t, function<bool(int)>> fv;
    ranges::transform_view<ref_t, function<int(int)>> tv;
    ranges::take_view<ref_t> tkv;
    ranges::drop_view<ref_t> dv;
    ranges::reverse_view<ref_t> rv;
    ranges::join_view<ranges::ref_view<vector<vector<int>>>> jv;
    SUCCEED();
}

TEST(RangesTest, ContainersAreNotViews) {
    static_assert(!ranges::View<vector<int>>);
    static_assert(ranges::View<ranges::ref_view<vector<int>>>);
    static_assert(ranges::View<ranges::owning_view<vector<int>>>);
}

TEST(RangesTest, FilterIteratorEqualityWithDifferentSentinel) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto fv = vec | rv::filter([](int x) { return x % 2 == 0; });
    auto it = fv.begin();
    auto end = fv.end();
    ASSERT_NE(it, end);
    ++it;
    ++it;
    ++it;
    ASSERT_EQ(it, end);
}

TEST(RangesTest, TransformIteratorEqualityWithSentinel) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto tv = vec | rv::transform([](int x) { return x * 2; });
    auto it = tv.begin();
    auto end = tv.end();
    ASSERT_NE(it, end);
    ++it;
    ++it;
    ++it;
    ASSERT_EQ(it, end);
}

TEST(RangesTest, DropWhileIteratorEqualityWithSentinel) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto dwv = vec | rv::drop_while([](int x) { return x < 3; });
    auto it = dwv.begin();
    auto end = dwv.end();
    ASSERT_NE(it, end);
}

TEST(RangesTest, FilterViewMutableAdaptor) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    int sum = 0;
    auto fv = vec | rv::filter([&sum](int x) {
                  sum += x;
                  return x % 2 == 0;
              });
    vector<int> result;
    for (auto x: fv) {
        result.push_back(x);
    }
    ASSERT_GT(sum, 0);
    ASSERT_EQ(result.size(), 2);
}

TEST(RangesTest, DropViewChain) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto v = vec | rv::drop(2) | rv::drop(3) | rv::take(3);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 6);
    ASSERT_EQ(result[2], 8);
}

TEST(RangesTest, TakeWhileDropWhileChain) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto v = vec | rv::take_while([](int x) { return x < 8; }) | rv::drop_while([](int x) { return x < 3; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[4], 7);
}

TEST(RangesTest, ReverseAndFilter) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    auto v = vec | rv::reverse() | rv::filter([](int x) { return x % 2 == 0; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 8);
    ASSERT_EQ(result[3], 2);
}

TEST(RangesTest, JoinTransformChain) {
    namespace rv = ranges::views;
    vector<vector<int>> nested = {{1, 2}, {3, 4, 5}};
    auto v = nested | rv::join() | rv::transform([](int x) { return x * 10; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[4], 50);
}

TEST(RangesTest, ElementsFilterChain) {
    namespace rv = ranges::views;
    vector<pair<int, int>> vec = {{1, 10}, {2, 20}, {3, 30}, {4, 40}};
    auto v = vec | rv::elements<0>() | rv::filter([](int x) { return x % 2 == 0; }) |
             rv::transform([](int x) { return x * 5; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[1], 20);
}

TEST(RangesTest, CountedWithTransform) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto cv = rv::counted(vec.begin() + 1, 4);
    auto tv = cv | rv::transform([](int x) { return x * 3; });
    vector<int> result;
    for (auto x: tv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 6);
    ASSERT_EQ(result[3], 15);
}

TEST(RangesTest, SplitWithTransform) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 0, 2, 0, 3};
    auto sv = vec | rv::split(0);
    int sum = 0;
    for (auto subrange: sv) {
        for (auto x: subrange) {
            sum += x;
        }
    }
    ASSERT_EQ(sum, 6);
}

TEST(RangesTest, IotaWithTransformAndFilter) {
    namespace rv = ranges::views;
    auto v = rv::iota(1, 20) | rv::filter([](int x) { return x % 3 == 0; }) |
             rv::transform([](int x) { return x * 2; }) | rv::take(4);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 6);
    ASSERT_EQ(result[3], 24);
}

TEST(RangesTest, RepeatWithDropAndTake) {
    namespace rv = ranges::views;
    auto v = rv::repeat(5, 10) | rv::drop(3) | rv::take(4);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    for (int val: result) {
        ASSERT_EQ(val, 5);
    }
}

TEST(RangesTest, CommonViewWithTransform) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto fv = vec | rv::filter([](int x) { return x > 2; });
    auto cv = fv | rv::common() | rv::transform([](int x) { return x * 10; });
    vector<int> result;
    for (auto x: cv) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 30);
    ASSERT_EQ(result[2], 50);
}

TEST(RangesTest, SliceWithDropAndReverse) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto v = vec | rv::slice(1, 8) | rv::drop(2) | rv::reverse() | rv::take(4);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 8);
    ASSERT_EQ(result[3], 5);
}

TEST(RangesTest, FilterOnFilteredView) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto v = vec | rv::filter([](int x) { return x % 2 == 1; }) | rv::filter([](int x) { return x > 5; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 7);
    ASSERT_EQ(result[1], 9);
}

TEST(RangesTest, ComplexChain) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    auto v = vec | rv::filter([](int x) { return x % 2 == 0; }) | rv::reverse() | rv::drop(2) |
             rv::take_while([](int x) { return x < 18; }) | rv::transform([](int x) { return x / 2; }) | rv::take(5);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
}

TEST(RangesTest, AllAdaptorLvalue) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto v = rv::all(vec);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, AllAdaptorRvalue) {
    namespace rv = ranges::views;
    auto v = rv::all(vector<int>{4, 5, 6});
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, FilterAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto v = rv::filter(vec, [](int x) { return x % 3 == 0; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[1], 6);
}

TEST(RangesTest, TransformAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto v = rv::transform(vec, [](int x) { return x * 100; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 100);
    ASSERT_EQ(result[2], 300);
}

TEST(RangesTest, TakeAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto v = rv::take(vec, 3);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, TakeWhileAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto v = rv::take_while(vec, [](int x) { return x < 4; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, DropAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto v = rv::drop(vec, 2);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 3);
}

TEST(RangesTest, DropWhileAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto v = rv::drop_while(vec, [](int x) { return x < 3; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 3);
}

TEST(RangesTest, ReverseAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto v = rv::reverse(vec);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[2], 1);
}

TEST(RangesTest, IotaAdaptorTwoArgs) {
    namespace rv = ranges::views;
    auto v = rv::iota(10, 15);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[4], 14);
}

TEST(RangesTest, RepeatAdaptorTwoArgs) {
    namespace rv = ranges::views;
    auto v = rv::repeat(3, 5);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    for (int val: result) {
        ASSERT_EQ(val, 3);
    }
}

TEST(RangesTest, JoinAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<vector<int>> nested = {{1}, {2, 3}};
    auto v = rv::join(nested);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
}

TEST(RangesTest, SliceAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto v = rv::slice(vec, 1, 4);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[2], 4);
}

TEST(RangesTest, SplitAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 0, 2};
    auto v = rv::split(vec, 0);
    int count = 0;
    for (auto subrange: v) {
        (void) subrange;
        ++count;
    }
    ASSERT_EQ(count, 2);
}

TEST(RangesTest, ConcatAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> a = {1, 2};
    vector<int> b = {3, 4};
    auto v = rv::concat(a, b);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
}

TEST(RangesTest, CountedAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> vec = {10, 20, 30, 40};
    auto v = rv::counted(vec.begin(), 2);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[1], 20);
}

TEST(RangesTest, AllSelectsReferenceOrOwningView) {
    static_assert(is_same_v<decltype(ranges::all(declval<vector<int>&>())), ranges::ref_view<vector<int>>>);
    static_assert(is_same_v<decltype(ranges::all(declval<const vector<int>&>())), ranges::ref_view<const vector<int>>>);
    static_assert(is_same_v<decltype(ranges::all(declval<vector<int>&&>())), ranges::owning_view<vector<int>>>);
}

TEST(RangesTest, RefViewIsUsableForContainers) {
    vector<int> vec = {1, 2, 3, 4};
    ranges::ref_view<vector<int>> rv_view{vec};
    ASSERT_EQ(rv_view.size(), 4U);
    ASSERT_FALSE(rv_view.empty());
    ASSERT_EQ(rv_view.base()[0], 1);
    ASSERT_EQ(*rv_view.begin(), 1);
}

TEST(RangesTest, OwningViewIsMovable) {
    static_assert(!is_copy_constructible_v<ranges::owning_view<vector<int>>>);
    static_assert(is_move_constructible_v<ranges::owning_view<vector<int>>>);
    static_assert(is_move_assignable_v<ranges::owning_view<vector<int>>>);

    auto owned = ranges::all(vector<int>{1, 2, 3});
    auto moved = _NEFORCE move(owned);
    vector<int> result;
    for (auto x: moved) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[2], 3);
}

TEST(RangesTest, LvaluePipeDoesNotOwnTheSource) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    auto v = vec | rv::filter([](int x) { return x > 0; });
    vec[1] = 42;
    vec.push_back(7);
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[1], 42);
    ASSERT_EQ(result[3], 7);
}

TEST(RangesTest, RvaluePipeOwnsTheSource) {
    namespace rv = ranges::views;
    auto v = vector<int>{1, 2, 3} | rv::filter([](int x) { return x > 1; });
    vector<int> result;
    for (auto x: v) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[1], 3);
}

TEST(RangesTest, SizedViewProvidesExactSize) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    ASSERT_EQ((vec | rv::take(3)).size(), 3U);
    ASSERT_EQ((vec | rv::take(9)).size(), 5U);
    ASSERT_EQ((vec | rv::drop(2)).size(), 3U);
    ASSERT_EQ((vec | rv::drop(9)).size(), 0U);
    ASSERT_EQ((vec | rv::transform([](int x) { return x * 2; })).size(), 5U);
    ASSERT_EQ((vec | rv::reverse()).size(), 5U);
}

TEST(RangesTest, SizedViewChainsRemainSized) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto v = vec | rv::transform([](int x) { return x + 1; }) | rv::drop(1) | rv::take(3);
    ASSERT_EQ(v.size(), 3U);
    long sum = 0;
    for (auto x: v) {
        sum += x;
    }
    ASSERT_EQ(sum, 3 + 4 + 5);
}
TEST(RangesTest, DistinctAdaptorRemovesDuplicates) {
    namespace rv = ranges::views;
    vector<int> vec = {3, 1, 3, 2, 1};
    vector<int> result;
    for (auto x: vec | rv::distinct()) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[2], 2);
}

TEST(RangesTest, DistinctByAdaptorUsesKey) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    size_t total = 0;
    for (auto x: vec | rv::distinct_by([](int value) { return value % 2; })) {
        (void) x;
        ++total;
    }
    ASSERT_EQ(total, 2U);
}

TEST(RangesTest, OrderByAdaptorSorts) {
    namespace rv = ranges::views;
    vector<int> vec = {5, 3, 9, 1};
    vector<int> result;
    for (auto x: vec | rv::order_by([](int value) { return value; })) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[3], 9);
}

TEST(RangesTest, OrderByDescendingAdaptorSorts) {
    namespace rv = ranges::views;
    vector<int> vec = {5, 3, 9, 1};
    auto view = vec | rv::order_by_descending([](int value) { return value; });
    ASSERT_EQ(view.size(), 4U);
    ASSERT_EQ(*view.begin(), 9);
}

TEST(RangesTest, ThenByAdaptorKeepsPrimaryKeyDominant) {
    namespace rv = ranges::views;
    vector<int> vec = {12, 21, 11, 22};
    vector<int> result;
    for (auto x:
         vec | rv::order_by([](int value) { return value / 10; }) | rv::then_by([](int value) { return value % 10; })) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 11);
    ASSERT_EQ(result[1], 12);
    ASSERT_EQ(result[2], 21);
    ASSERT_EQ(result[3], 22);
}

TEST(RangesTest, ThenByDescendingAdaptorReversesSecondaryKey) {
    namespace rv = ranges::views;
    vector<int> vec = {12, 21, 11, 22};
    vector<int> result;
    for (auto x: vec | rv::order_by([](int value) { return value / 10; }) |
                         rv::then_by_descending([](int value) { return value % 10; })) {
        result.push_back(x);
    }
    ASSERT_EQ(result[0], 12);
    ASSERT_EQ(result[1], 11);
    ASSERT_EQ(result[2], 22);
    ASSERT_EQ(result[3], 21);
}

TEST(RangesTest, GroupByAdaptorGroupsEqualKeys) {
    namespace rv = ranges::views;
    vector<int> vec = {3, 1, 3, 2, 1};
    vector<int> keys;
    vector<size_t> sizes;
    for (auto group: vec | rv::group_by([](int value) { return value; })) {
        keys.push_back(group.key());
        size_t count = 0;
        for (auto x: group) {
            (void) x;
            ++count;
        }
        sizes.push_back(count);
    }
    ASSERT_EQ(keys.size(), 3U);
    ASSERT_EQ(keys[0], 1);
    ASSERT_EQ(keys[2], 3);
    ASSERT_EQ(sizes[0], 2U);
    ASSERT_EQ(sizes[1], 1U);
}

TEST(RangesTest, GroupByAdaptorWithoutKeyUsesElement) {
    namespace rv = ranges::views;
    vector<int> vec = {2, 2, 5};
    size_t groups = 0;
    for (auto group: vec | rv::group_by()) {
        (void) group;
        ++groups;
    }
    ASSERT_EQ(groups, 2U);
}

TEST(RangesTest, ChunkAdaptorSplitsWithPartialTail) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto view = vec | rv::chunk(2);
    ASSERT_EQ(view.size(), 3U);
    vector<size_t> sizes;
    for (auto piece: view) {
        size_t count = 0;
        for (auto x: piece) {
            (void) x;
            ++count;
        }
        sizes.push_back(count);
    }
    ASSERT_EQ(sizes.size(), 3U);
    ASSERT_EQ(sizes[0], 2U);
    ASSERT_EQ(sizes[2], 1U);
}

TEST(RangesTest, SlidingAdaptorDropsIncompleteTail) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto view = vec | rv::sliding(3);
    ASSERT_EQ(view.size(), 3U);
    size_t windows = 0;
    for (auto window: view) {
        size_t count = 0;
        for (auto x: window) {
            (void) x;
            ++count;
        }
        ASSERT_EQ(count, 3U);
        ++windows;
    }
    ASSERT_EQ(windows, 3U);
}

TEST(RangesTest, ZipAdaptorPairsToShorterLength) {
    namespace rv = ranges::views;
    vector<int> left = {1, 2, 3};
    vector<int> right = {10, 20};
    vector<pair<int, int>> result;
    for (auto item: left | rv::zip(right)) {
        result.push_back(item);
    }
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result[0].first, 1);
    ASSERT_EQ(result[0].second, 10);
    ASSERT_EQ(result[1].second, 20);
}

TEST(RangesTest, ZipAdaptorTwoArgs) {
    namespace rv = ranges::views;
    vector<int> left = {1, 2};
    vector<int> right = {10, 20};
    auto view = rv::zip(left, right);
    ASSERT_EQ(view.size(), 2U);
}

TEST(RangesTest, EnumerateAdaptorAddsIndices) {
    namespace rv = ranges::views;
    vector<int> vec = {10, 20, 30};
    vector<pair<size_t, int>> result;
    for (auto item: vec | rv::enumerate()) {
        result.push_back(item);
    }
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0].first, 0U);
    ASSERT_EQ(result[2].first, 2U);
    ASSERT_EQ(result[2].second, 30);
}

TEST(RangesTest, ScanAdaptorYieldsRunningTotals) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    vector<int> result;
    for (auto x: vec | rv::scan(0, [](int acc, int value) { return acc + value; })) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 0);
    ASSERT_EQ(result[3], 6);
}

TEST(RangesTest, ScanAdaptorOnEmptyRange) {
    namespace rv = ranges::views;
    vector<int> vec;
    size_t total = 0;
    for (auto x: vec | rv::scan(5, [](int acc, int value) { return acc + value; })) {
        ASSERT_EQ(x, 5);
        ++total;
    }
    ASSERT_EQ(total, 1U);
}
TEST(RangesTest, SliceClampsNegativeStart) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto view = vec | rv::slice(-2, 3);
    ASSERT_EQ(view.size(), 3U);
    size_t total = 0;
    for (auto x: view) {
        (void) x;
        ++total;
    }
    ASSERT_EQ(total, 3U);
}

TEST(RangesTest, SliceClampsOutOfRangeBounds) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    ASSERT_EQ((vec | rv::slice(0, 99)).size(), 5U);
    ASSERT_EQ((vec | rv::slice(3, 1)).size(), 0U);
}

TEST(RangesTest, SliceZeroBoundsIsEmpty) {
    vector<int> vec = {1, 2, 3};
    ranges::slice_view<ranges::ref_view<vector<int>>> view{ranges::ref_view<vector<int>>{vec}, 0, 0};
    ASSERT_EQ(view.size(), 0U);
    size_t total = 0;
    for (auto x: view) {
        (void) x;
        ++total;
    }
    ASSERT_EQ(total, 0U);
}

TEST(RangesTest, SliceConstIterationWorks) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    const auto view = vec | rv::slice(1, 4);
    size_t total = 0;
    for (auto x: view) {
        (void) x;
        ++total;
    }
    ASSERT_EQ(total, 3U);
}

TEST(RangesTest, FilterDistanceCountsMatchingElements) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto view = vec | rv::filter([](int x) { return x % 2 == 0; });
    auto first = view.begin();
    auto last = view.end();
    ASSERT_EQ(last - first, 3);
    ASSERT_EQ(first - last, -3);
    ASSERT_EQ(std::ranges::distance(view), 3);
}

TEST(RangesTest, FilterIteratorStaysBidirectional) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto view = vec | rv::filter([](int x) { return x % 2 == 0; });
    static_assert(bidirectional_iterator<decltype(view.begin())>);
    static_assert(ranges::common_range<decltype(view)>);
}

TEST(RangesTest, TakeIteratorEqualityRequiresSamePosition) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto view = vec | rv::take(2);
    auto first = view.begin();
    auto advanced = first;
    ++advanced;
    ++advanced;
    ASSERT_NE(first, advanced);
    auto same = first;
    ASSERT_EQ(first, same);
}

TEST(RangesTest, TakeAndDropClampNegativeCounts) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    ASSERT_EQ((vec | rv::take(-1)).size(), 0U);
    ASSERT_EQ((vec | rv::drop(-1)).size(), 3U);
}

TEST(RangesTest, RepeatNegativeCountIsEmpty) {
    namespace rv = ranges::views;
    size_t total = 0;
    for (auto x: rv::repeat(7, -3)) {
        (void) x;
        ++total;
    }
    ASSERT_EQ(total, 0U);
}

TEST(RangesTest, RepeatIteratorEqualityComparesValue) {
    namespace rv = ranges::views;
    auto first_view = rv::repeat(1, 3);
    auto second_view = rv::repeat(2, 3);
    ASSERT_FALSE(first_view.begin() == second_view.begin());
}

TEST(RangesTest, CountedNegativeCountIsEmpty) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4};
    auto view = rv::counted(vec.begin(), -2);
    size_t total = 0;
    for (auto x: view) {
        (void) x;
        ++total;
    }
    ASSERT_EQ(total, 0U);
    ASSERT_EQ(view.size(), 0U);
}

TEST(RangesTest, CountedIteratorComparesBasePosition) {
    vector<int> vec = {1, 2, 3, 4};
    ranges::counted_iterator<vector<int>::iterator> first(vec.begin(), 2);
    ranges::counted_iterator<vector<int>::iterator> shifted(vec.begin() + 2, 2);
    ASSERT_FALSE(first == shifted);
    ASSERT_EQ(*first, 1);
    ASSERT_EQ(*shifted, 3);
}

TEST(RangesTest, CountedViewDefaultConstructedIsEmpty) {
    ranges::counted_view<vector<int>::iterator> view{};
    ASSERT_EQ(view.size(), 0U);
}

TEST(RangesTest, SubrangeViewDefaultConstructedSizeIsZero) {
    ranges::subrange_view<const int*> view{};
    ASSERT_EQ(view.size(), 0);
}

TEST(RangesTest, ElementsWorkWithStdPairAndTuple) {
    namespace rv = ranges::views;
    vector<pair<int, int>> pairs = {{1, 10}, {2, 20}};
    int sum = 0;
    for (auto key: pairs | rv::keys) {
        sum += key;
    }
    for (auto value: pairs | rv::values) {
        sum += value;
    }
    vector<tuple<int, int>> tuples = {{3, 30}};
    for (auto key: tuples | rv::elements<0>) {
        sum += key;
    }
    ASSERT_EQ(sum, 1 + 2 + 10 + 20 + 3);
}

TEST(RangesTest, KeysAndValuesAreComposableClosures) {
    namespace rv = ranges::views;
    vector<pair<int, int>> pairs = {{1, 10}, {2, 20}};
    vector<int> doubled;
    for (auto key: pairs | (rv::keys | rv::transform([](int x) { return x * 2; }))) {
        doubled.push_back(key);
    }
    ASSERT_EQ(doubled.size(), 2U);
    ASSERT_EQ(doubled[0], 2);
    ASSERT_EQ(doubled[1], 4);
    ASSERT_EQ(rv::keys(pairs).size(), 2U);
}

TEST(RangesTest, ClosureCompositionAppliesInOrder) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5};
    auto composed = rv::filter([](int x) { return x > 1; }) | rv::transform([](int x) { return x * 10; });
    vector<int> result;
    for (auto x: vec | composed) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 20);
    ASSERT_EQ(result[3], 50);
}

TEST(RangesTest, IotaAcceptsDifferentBoundType) {
    namespace rv = ranges::views;
    auto view = rv::iota(0, 10LL);
    size_t total = 0;
    long sum = 0;
    for (auto x: view) {
        sum += x;
        ++total;
    }
    ASSERT_EQ(total, 10U);
    ASSERT_EQ(sum, 45);
}

TEST(RangesTest, ViewStaysUsableWithoutConstIteration) {
    struct narrow_view : ranges::view_base<narrow_view> {
        const int* first;
        const int* last;
        const int* begin() { return first; }
        const int* end() { return last; }
    };
    static_assert(ranges::View<narrow_view>);
    static_assert(!ranges::Range<const narrow_view>);
    int raw[4] = {1, 2, 3, 4};
    narrow_view left{{}, raw, raw + 2};
    narrow_view right{{}, raw + 2, raw + 4};
    auto concatenated = ranges::concat_view<narrow_view, narrow_view>{left, right};
    size_t concat_total = 0;
    for (auto x: concatenated) {
        (void) x;
        ++concat_total;
    }
    ASSERT_EQ(concat_total, 4U);
    auto sliced = ranges::slice_view<narrow_view>{left, 0, 1};
    size_t slice_total = 0;
    for (auto x: sliced) {
        (void) x;
        ++slice_total;
    }
    ASSERT_EQ(slice_total, 1U);
    auto split = ranges::split_view<narrow_view, int>{left, 2};
    size_t group_total = 0;
    for (auto group: split) {
        for (auto x: group) {
            (void) x;
            ++group_total;
        }
    }
    ASSERT_EQ(group_total, 1U);
}

TEST(RangesTest, FilterIteratorProvidesArrowForLvalueReference) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4};
    auto view = vec | rv::filter([](int x) { return x > 2; });
    auto iter = view.begin();
    ASSERT_EQ(*iter.operator->(), 3);
}

TEST(RangesTest, ReverseRequiresCommonRange) {
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3};
    ASSERT_EQ((vec | rv::reverse()).size(), 3U);
    static_assert(requires(vector<int>& v) { v | rv::reverse(); });
}

#endif
