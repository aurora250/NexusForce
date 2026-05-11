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
    auto base = fv.base();
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
    auto base = tv.base();
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
    auto base = tv.base();
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
    auto base = twv.base();
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
    auto base = dv.base();
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
    auto base = dwv.base();
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
    auto base = rv_view.base();
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
    auto base = jv.base();
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
    auto base = cv.base();
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
    ranges::filter_view<vector<int>, function<bool(int)>> fv;
    ranges::transform_view<vector<int>, function<int(int)>> tv;
    ranges::take_view<vector<int>> tkv;
    ranges::drop_view<vector<int>> dv;
    ranges::reverse_view<vector<int>> rv;
    ranges::join_view<vector<vector<int>>> jv;
    SUCCEED();
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

#endif
