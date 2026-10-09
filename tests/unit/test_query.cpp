#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/iterator/query.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/optional.hpp>
#include <NeForce/core/utility/pair.hpp>
#include <gtest/gtest.h>
using namespace neforce;
#ifdef NEFORCE_STANDARD_20

TEST(QueryTest, FromLvalueContainerIsLazyAndNonOwning) {
    vector<int> vec = {1, 2, 3};
    auto query = ranges::from(vec);
    vec[0] = 10;
    vec.push_back(4);
    auto result = query.to_vector();
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[3], 4);
}

TEST(QueryTest, FromRvalueContainerOwnsSource) {
    auto query = ranges::from(vector<int>{1, 2, 3});
    auto result = query.to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[2], 3);
}

TEST(QueryTest, LazyOperatorsDoNotEvaluate) {
    vector<int> vec = {1, 2, 3, 4, 5};
    size_t calls = 0;
    auto query = ranges::from(vec).filter([&calls](int x) {
        ++calls;
        return x > 2;
    });
    ASSERT_EQ(calls, 0U);
    ASSERT_EQ(query.count(), 3U);
    ASSERT_GT(calls, 0U);
}

TEST(QueryTest, FilterTransformTakeDrop) {
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    auto result = ranges::from(vec)
                          .filter([](int x) { return x % 2 == 0; })
                          .transform([](int x) { return x * 10; })
                          .drop(1)
                          .take(2)
                          .to_vector();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result[0], 40);
    ASSERT_EQ(result[1], 60);
}

TEST(QueryTest, TakeAndDropBeyondSize) {
    vector<int> vec = {1, 2, 3};
    ASSERT_EQ(ranges::from(vec).take(10).count(), 3U);
    ASSERT_EQ(ranges::from(vec).drop(10).count(), 0U);
    ASSERT_TRUE(ranges::from(vec).drop(3).is_empty());
}

TEST(QueryTest, TakeWhileAndDropWhile) {
    vector<int> vec = {1, 2, 3, 4, 1, 2};
    auto head = ranges::from(vec).take_while([](int x) { return x < 4; }).to_vector();
    ASSERT_EQ(head.size(), 3U);
    auto tail = ranges::from(vec).drop_while([](int x) { return x < 4; }).to_vector();
    ASSERT_EQ(tail.size(), 3U);
    ASSERT_EQ(tail[0], 4);
    ASSERT_EQ(tail[2], 2);
}

TEST(QueryTest, ReverseChain) {
    vector<int> vec = {1, 2, 3, 4};
    auto result = ranges::from(vec).reverse().to_vector();
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 4);
    ASSERT_EQ(result[3], 1);
}

TEST(QueryTest, TransformManyFlattens) {
    vector<int> vec = {1, 2, 3};
    vector<vector<int>> groups = {{1, 10}, {2, 20}, {3, 30}};
    auto result = ranges::from(vec)
                          .transform_many([&groups](int x) -> const vector<int>& { return groups[x - 1]; })
                          .to_vector();
    ASSERT_EQ(result.size(), 6U);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[1], 10);
    ASSERT_EQ(result[5], 30);
}

TEST(QueryTest, JoinFlattensNested) {
    vector<vector<int>> nested = {{1, 2}, {3}, {}, {4, 5}};
    auto result = ranges::from(nested).join().to_vector();
    ASSERT_EQ(result.size(), 5U);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[4], 5);
}

TEST(QueryTest, ElementsKeysAndValues) {
    vector<pair<int, int>> pairs = {{1, 10}, {2, 20}};
    auto keys = ranges::from(pairs).keys().to_vector();
    auto values = ranges::from(pairs).values().to_vector();
    ASSERT_EQ(keys.size(), 2U);
    ASSERT_EQ(keys[1], 2);
    ASSERT_EQ(values[1], 20);
    auto first = ranges::from(pairs).elements<0>().to_vector();
    ASSERT_EQ(first[0], 1);
}

TEST(QueryTest, SliceRange) {
    vector<int> vec = {1, 2, 3, 4, 5};
    auto result = ranges::from(vec).slice(1, 4).to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[2], 4);
}

TEST(QueryTest, SplitByDelimiter) {
    vector<char> text = {'a', ',', 'b', ',', 'c'};
    size_t groups = 0;
    size_t total = 0;
    for (auto group: ranges::from(text).split(',')) {
        ++groups;
        for (auto value: group) {
            (void) value;
            ++total;
        }
    }
    ASSERT_EQ(groups, 3U);
    ASSERT_EQ(total, 3U);
}

TEST(QueryTest, ConcatAppendsRange) {
    vector<int> head = {1, 2};
    vector<int> tail = {3, 4};
    auto from_lvalue = ranges::from(head).concat(tail).to_vector();
    ASSERT_EQ(from_lvalue.size(), 4U);
    ASSERT_EQ(from_lvalue[2], 3);
    auto from_rvalue = ranges::from(head).concat(vector<int>{8, 9}).to_vector();
    ASSERT_EQ(from_rvalue.size(), 4U);
    ASSERT_EQ(from_rvalue[2], 8);
}

TEST(QueryTest, QueryRemainsUsableAsView) {
    vector<int> vec = {1, 2, 3, 4, 5};
    auto query = ranges::from(vec).filter([](int x) { return x > 1; });
    long sum = 0;
    for (auto value: query | ranges::views::take(2)) {
        sum += value;
    }
    ASSERT_EQ(sum, 5);
}

TEST(QueryTest, QueryCanBeUsedAsSource) {
    vector<int> vec = {1, 2, 3, 4};
    auto first = ranges::from(vec).filter([](int x) { return x % 2 == 0; });
    auto second = ranges::from(first).transform([](int x) { return x + 1; }).to_vector();
    ASSERT_EQ(second.size(), 2U);
    ASSERT_EQ(second[0], 3);
    ASSERT_EQ(second[1], 5);
}

TEST(QueryTest, QueryIsReiterable) {
    vector<int> vec = {1, 2, 3};
    auto query = ranges::from(vec);
    ASSERT_EQ(query.count(), 3U);
    vec.push_back(4);
    ASSERT_EQ(query.count(), 4U);
    ASSERT_EQ(query.count(), 4U);
}

TEST(QueryTest, TerminalsWorkOnConstQuery) {
    vector<int> vec = {1, 2, 3};
    const auto query = ranges::from(vec).transform([](int x) { return x * 2; });
    ASSERT_EQ(query.count(), 3U);
    ASSERT_EQ(query.sum(), 12);
    ASSERT_EQ(query.max().value_or(0), 6);
}

TEST(QueryTest, ToVectorKeepsOrder) {
    vector<int> vec = {5, 4, 3, 2, 1};
    auto result = ranges::from(vec).to_vector();
    ASSERT_EQ(result.size(), 5U);
    ASSERT_EQ(result[0], 5);
    ASSERT_EQ(result[4], 1);
}

TEST(QueryTest, ToListAndToSet) {
    vector<int> vec = {3, 1, 3, 2, 1};
    auto listed = ranges::from(vec).to_list();
    ASSERT_EQ(listed.size(), 5U);
    auto unique = ranges::from(vec).to_set();
    ASSERT_EQ(unique.size(), 3U);
    ASSERT_EQ(unique.count(2), 1U);
}

TEST(QueryTest, ToCustomContainer) {
    vector<int> vec = {1, 2, 3};
    auto result = ranges::from(vec).transform([](int x) { return x + 1; }).to<vector<int>>();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 2);
    auto widened = ranges::from(vec).transform([](int x) { return static_cast<long long>(x); }).to<vector<long long>>();
    ASSERT_EQ(widened.size(), 3U);
    ASSERT_EQ(widened[2], 3);
}

TEST(QueryTest, CountAndCountIf) {
    vector<int> vec = {1, 2, 3, 4, 5};
    ASSERT_EQ(ranges::from(vec).count(), 5U);
    ASSERT_EQ(ranges::from(vec).count_if([](int x) { return x % 2 == 0; }), 2U);
    ASSERT_EQ(ranges::from(vec).filter([](int x) { return x > 10; }).count(), 0U);
}

TEST(QueryTest, SumProductAndAverage) {
    vector<int> vec = {1, 2, 3, 4};
    ASSERT_EQ(ranges::from(vec).sum(), 10);
    ASSERT_EQ(ranges::from(vec).sum(100L), 110L);
    ASSERT_EQ(ranges::from(vec).product(1), 24);
    ASSERT_DOUBLE_EQ(ranges::from(vec).average(), 2.5);
}

TEST(QueryTest, AggregatesOnEmptyQuery) {
    vector<int> vec;
    ASSERT_EQ(ranges::from(vec).sum(), 0);
    ASSERT_DOUBLE_EQ(ranges::from(vec).average(), 0.0);
    ASSERT_FALSE(ranges::from(vec).min().has_value());
    ASSERT_FALSE(ranges::from(vec).max().has_value());
    ASSERT_FALSE(ranges::from(vec).first().has_value());
    ASSERT_TRUE(ranges::from(vec).all_of([](int) { return false; }));
    ASSERT_FALSE(ranges::from(vec).any_of([](int) { return true; }));
}

TEST(QueryTest, MinMaxMinByMaxBy) {
    vector<int> vec = {4, 1, 7, 3};
    ASSERT_EQ(ranges::from(vec).min().value_or(0), 1);
    ASSERT_EQ(ranges::from(vec).max().value_or(0), 7);
    ASSERT_EQ(ranges::from(vec).min_by([](int x) { return -x; }).value_or(0), 7);
    ASSERT_EQ(ranges::from(vec).max_by([](int x) { return -x; }).value_or(0), 1);
}

TEST(QueryTest, MinByOnEmptyQuery) {
    vector<string> words;
    ASSERT_FALSE(ranges::from(words).min_by([](const string& s) { return s.size(); }).has_value());
}

TEST(QueryTest, FirstLastAndElementAt) {
    vector<int> vec = {10, 20, 30};
    ASSERT_EQ(ranges::from(vec).first().value_or(0), 10);
    ASSERT_EQ(ranges::from(vec).last().value_or(0), 30);
    ASSERT_EQ(ranges::from(vec).first_or(-1), 10);
    ASSERT_EQ(ranges::from(vec).last_or(-1), 30);
    ASSERT_EQ(ranges::from(vec).element_at(1).value_or(0), 20);
    ASSERT_FALSE(ranges::from(vec).element_at(3).has_value());
    ASSERT_EQ(ranges::from(vec).drop(3).first_or(-1), -1);
    ASSERT_EQ(ranges::from(vec).drop(3).last_or(-1), -1);
}

TEST(QueryTest, FirstStopsAfterOneElement) {
    vector<int> vec = {1, 2, 3, 4, 5};
    size_t visited = 0;
    auto query = ranges::from(vec).transform([&visited](int x) {
        ++visited;
        return x;
    });
    ASSERT_EQ(query.first().value_or(0), 1);
    ASSERT_EQ(visited, 1U);
}

TEST(QueryTest, AnyOfShortCircuits) {
    vector<int> vec = {1, 2, 3, 4, 5};
    size_t visited = 0;
    auto found = ranges::from(vec).any_of([&visited](int x) {
        ++visited;
        return x == 2;
    });
    ASSERT_TRUE(found);
    ASSERT_EQ(visited, 2U);
}

TEST(QueryTest, ElementAtEvaluatesOnlyTarget) {
    vector<int> vec = {1, 2, 3, 4, 5};
    size_t visited = 0;
    auto query = ranges::from(vec).transform([&visited](int x) {
        ++visited;
        return x;
    });
    ASSERT_EQ(query.element_at(2).value_or(0), 3);
    ASSERT_EQ(visited, 1U);
    ASSERT_FALSE(query.element_at(9).has_value());
    ASSERT_EQ(visited, 1U);
}

TEST(QueryTest, AllOfNoneOfAndContains) {
    vector<int> vec = {2, 4, 6};
    ASSERT_TRUE(ranges::from(vec).all_of([](int x) { return x % 2 == 0; }));
    ASSERT_FALSE(ranges::from(vec).all_of([](int x) { return x > 2; }));
    ASSERT_TRUE(ranges::from(vec).none_of([](int x) { return x < 0; }));
    ASSERT_TRUE(ranges::from(vec).contains(4));
    ASSERT_FALSE(ranges::from(vec).contains(5));
}

TEST(QueryTest, AggregateAndFold) {
    vector<int> vec = {1, 2, 3, 4};
    auto sum = ranges::from(vec).aggregate(0, [](int acc, int x) { return acc + x; });
    ASSERT_EQ(sum, 10);
    auto product = ranges::from(vec).fold(1, [](int acc, int x) { return acc * x; });
    ASSERT_EQ(product, 24);
    auto text = ranges::from(vec).aggregate(string(""), [](string acc, int x) {
        acc.push_back(static_cast<char>('0' + x));
        return acc;
    });
    ASSERT_EQ(text.size(), 4U);
    ASSERT_EQ(text[3], '4');
}

TEST(QueryTest, ForEachVisitsAllElements) {
    vector<int> vec = {1, 2, 3};
    long total = 0;
    ranges::from(vec).for_each([&total](int x) { total += x; });
    ASSERT_EQ(total, 6);
    size_t visited = 0;
    ranges::from(vec).filter([](int x) { return x > 1; }).for_each([&visited](int) { ++visited; });
    ASSERT_EQ(visited, 2U);
}

TEST(QueryTest, IsEmptyChecksOneElement) {
    vector<int> vec = {1, 2, 3};
    ASSERT_FALSE(ranges::from(vec).is_empty());
    ASSERT_TRUE(ranges::from(vec).filter([](int x) { return x > 10; }).is_empty());
}

TEST(QueryTest, SizeIsAvailableForSizedChains) {
    vector<int> vec = {1, 2, 3, 4, 5};
    auto sized = ranges::from(vec).transform([](int x) { return x + 1; }).take(3);
    ASSERT_EQ(sized.size(), 3U);
    ASSERT_EQ(sized.size_hint(), 3U);
    ASSERT_EQ(sized.count(), 3U);
    auto dropped = ranges::from(vec).drop(2);
    ASSERT_EQ(dropped.size(), 3U);
    auto filtered = ranges::from(vec).filter([](int x) { return x > 1; });
    static_assert(requires(const decltype(sized)& q) { q.size(); });
    ASSERT_EQ(filtered.size_hint(), 0U);
}

TEST(QueryTest, SizeHintIsZeroForFilteredChain) {
    vector<int> vec = {1, 2, 3};
    auto filtered = ranges::from(vec).filter([](int x) { return x > 1; });
    ASSERT_EQ(filtered.size_hint(), 0U);
    ASSERT_EQ(filtered.to_vector().size(), 2U);
}

TEST(QueryTest, TransformChangesElementType) {
    vector<int> vec = {1, 2};
    auto widened = ranges::from(vec).transform([](int x) { return static_cast<long long>(x) * 2; }).to_vector();
    ASSERT_EQ(widened.size(), 2U);
    ASSERT_EQ(widened[1], 4);
    static_assert(is_same_v<decltype(widened)::value_type, long long>);
}

TEST(QueryTest, FilterAfterTransformUsesTransformedValues) {
    vector<int> vec = {1, 2, 3, 4};
    auto result =
            ranges::from(vec).transform([](int x) { return x * x; }).filter([](int x) { return x > 4; }).to_vector();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result[0], 9);
    ASSERT_EQ(result[1], 16);
}

TEST(QueryTest, LongChainKeepsResultCorrect) {
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto result = ranges::from(vec)
                          .filter([](int x) { return x % 2 == 0; })
                          .transform([](int x) { return x * 3; })
                          .filter([](int x) { return x < 20; })
                          .transform([](int x) { return x - 2; })
                          .drop(1)
                          .take(2)
                          .to_vector();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[1], 16);
}

TEST(QueryTest, PipelineAndChainProduceSameResult) {
    vector<int> vec = {1, 2, 3, 4, 5, 6};
    auto piped = (vec | ranges::views::filter([](int x) { return x % 2 == 0; }) | ranges::views::transform([](int x) {
                      return x * 5;
                  })).begin();
    auto chained = ranges::from(vec)
                           .filter([](int x) { return x % 2 == 0; })
                           .transform([](int x) { return x * 5; })
                           .to_vector();
    ASSERT_EQ(*piped, chained[0]);
    ASSERT_EQ(chained.size(), 3U);
    ASSERT_EQ(chained[0], 10);
    ASSERT_EQ(chained[2], 30);
}
TEST(QueryTest, DistinctRemovesDuplicates) {
    vector<int> vec = {3, 1, 3, 2, 1, 3};
    auto result = ranges::from(vec).distinct().to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 3);
    ASSERT_EQ(result[1], 1);
    ASSERT_EQ(result[2], 2);
}

TEST(QueryTest, DistinctIsRepeatable) {
    vector<int> vec = {1, 1, 2, 3, 3};
    auto query = ranges::from(vec).distinct();
    ASSERT_EQ(query.count(), 3U);
    ASSERT_EQ(query.count(), 3U);
    auto result = query.to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[2], 3);
}

TEST(QueryTest, DistinctByKeyKeepsFirstOccurrence) {
    vector<int> vec = {1, 2, 3, 4, 5};
    auto result = ranges::from(vec).distinct_by([](int x) { return x % 2; }).to_vector();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[1], 2);
}

TEST(QueryTest, OrderBySortsAscending) {
    vector<int> vec = {5, 3, 9, 1, 7};
    auto result = ranges::from(vec).order_by([](int x) { return x; }).to_vector();
    ASSERT_EQ(result.size(), 5U);
    ASSERT_EQ(result[0], 1);
    ASSERT_EQ(result[4], 9);
}

TEST(QueryTest, OrderByDescendingSortsDescending) {
    vector<int> vec = {5, 3, 9, 1, 7};
    auto result = ranges::from(vec).order_by_descending([](int x) { return x; }).to_vector();
    ASSERT_EQ(result[0], 9);
    ASSERT_EQ(result[4], 1);
}

TEST(QueryTest, OrderByKeepsEqualKeysStable) {
    vector<pair<int, int>> vec = {{1, 10}, {2, 20}, {1, 30}, {2, 40}};
    auto result = ranges::from(vec).order_by([](const pair<int, int>& p) { return p.first; }).to_vector();
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0].second, 10);
    ASSERT_EQ(result[1].second, 30);
    ASSERT_EQ(result[2].second, 20);
    ASSERT_EQ(result[3].second, 40);
}

TEST(QueryTest, ThenByAddsSecondaryKey) {
    vector<int> vec = {12, 21, 11, 22};
    auto result =
            ranges::from(vec).order_by([](int x) { return x / 10; }).then_by([](int x) { return x % 10; }).to_vector();
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 11);
    ASSERT_EQ(result[1], 12);
    ASSERT_EQ(result[2], 21);
    ASSERT_EQ(result[3], 22);
}

TEST(QueryTest, ThenByDescendingAddsDescendingSecondaryKey) {
    vector<int> vec = {12, 21, 11, 22};
    auto result = ranges::from(vec)
                          .order_by([](int x) { return x / 10; })
                          .then_by_descending([](int x) { return x % 10; })
                          .to_vector();
    ASSERT_EQ(result[0], 12);
    ASSERT_EQ(result[1], 11);
    ASSERT_EQ(result[2], 22);
    ASSERT_EQ(result[3], 21);
}

TEST(QueryTest, OrderByIsMaterializedAndRepeatable) {
    vector<int> vec = {3, 1, 2};
    auto query = ranges::from(vec).order_by([](int x) { return x; });
    ASSERT_EQ(query.count(), 3U);
    vec.push_back(0);
    auto result = query.to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 1);
}

TEST(QueryTest, OrderBySizeIsExact) {
    vector<int> vec = {3, 1, 2, 5, 4};
    auto query = ranges::from(vec).order_by([](int x) { return x; });
    ASSERT_EQ(query.size(), 5U);
    ASSERT_EQ(query.size_hint(), 5U);
}

TEST(QueryTest, GroupByGroupsEqualKeys) {
    vector<int> vec = {3, 1, 3, 2, 1};
    vector<int> keys;
    vector<size_t> sizes;
    for (auto group: ranges::from(vec).group_by([](int x) { return x; })) {
        keys.push_back(group.key());
        sizes.push_back(ranges::from(group).count());
    }
    ASSERT_EQ(keys.size(), 3U);
    ASSERT_EQ(keys[0], 1);
    ASSERT_EQ(keys[1], 2);
    ASSERT_EQ(keys[2], 3);
    ASSERT_EQ(sizes[0], 2U);
    ASSERT_EQ(sizes[1], 1U);
    ASSERT_EQ(sizes[2], 2U);
}

TEST(QueryTest, GroupByWithoutKeyUsesElement) {
    vector<int> vec = {2, 2, 5};
    size_t groups = 0;
    for (auto group: ranges::from(vec).group_by()) {
        (void) group;
        ++groups;
    }
    ASSERT_EQ(groups, 2U);
}

TEST(QueryTest, GroupByGroupIsUsableAsRange) {
    vector<int> vec = {1, 1, 2};
    vector<int> totals;
    for (auto group: ranges::from(vec).group_by([](int x) { return x; })) {
        totals.push_back(ranges::from(group).sum());
    }
    ASSERT_EQ(totals.size(), 2U);
    ASSERT_EQ(totals[0], 2);
    ASSERT_EQ(totals[1], 2);
}

TEST(QueryTest, ChunkSplitsEvenly) {
    vector<int> vec = {1, 2, 3, 4};
    vector<size_t> sizes;
    for (auto piece: ranges::from(vec).chunk(2)) {
        sizes.push_back(piece.size());
    }
    ASSERT_EQ(sizes.size(), 2U);
    ASSERT_EQ(sizes[0], 2U);
    ASSERT_EQ(sizes[1], 2U);
}

TEST(QueryTest, ChunkKeepsPartialTail) {
    vector<int> vec = {1, 2, 3, 4, 5};
    vector<vector<int>> pieces;
    for (auto piece: ranges::from(vec).chunk(2)) {
        pieces.push_back(ranges::from(piece).to_vector());
    }
    ASSERT_EQ(pieces.size(), 3U);
    ASSERT_EQ(pieces[0].size(), 2U);
    ASSERT_EQ(pieces[2].size(), 1U);
    ASSERT_EQ(pieces[2][0], 5);
}

TEST(QueryTest, ChunkCountOnEmptySource) {
    vector<int> vec;
    ASSERT_EQ(ranges::from(vec).chunk(3).count(), 0U);
    ASSERT_EQ(ranges::from(vec).chunk(3).to_vector().size(), 0U);
}

TEST(QueryTest, SlidingProducesOverlappingWindows) {
    vector<int> vec = {1, 2, 3, 4, 5};
    vector<vector<int>> windows;
    for (auto window: ranges::from(vec).sliding(3)) {
        windows.push_back(ranges::from(window).to_vector());
    }
    ASSERT_EQ(windows.size(), 3U);
    ASSERT_EQ(windows[0].size(), 3U);
    ASSERT_EQ(windows[0][0], 1);
    ASSERT_EQ(windows[0][2], 3);
    ASSERT_EQ(windows[1][0], 2);
    ASSERT_EQ(windows[2][2], 5);
}

TEST(QueryTest, SlidingDropsIncompleteTail) {
    vector<int> vec = {1, 2, 3, 4};
    ASSERT_EQ(ranges::from(vec).sliding(3).count(), 2U);
    ASSERT_EQ(ranges::from(vec).sliding(9).count(), 0U);
}

TEST(QueryTest, ZipPairsUpToShorterLength) {
    vector<int> left = {1, 2, 3};
    vector<int> right = {10, 20, 30, 40};
    auto result = ranges::from(left).zip(right).to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0].first, 1);
    ASSERT_EQ(result[0].second, 10);
    ASSERT_EQ(result[2].second, 30);
}

TEST(QueryTest, ZipPairsAreUsableByKeysAndValues) {
    vector<int> left = {1, 2};
    vector<int> right = {10, 20};
    auto keys = ranges::from(left).zip(right).elements<0>().to_vector();
    auto values = ranges::from(left).zip(right).elements<1>().to_vector();
    ASSERT_EQ(keys.size(), 2U);
    ASSERT_EQ(keys[1], 2);
    ASSERT_EQ(values[1], 20);
}

TEST(QueryTest, EnumerateAddsIndices) {
    vector<int> vec = {10, 20, 30};
    auto result = ranges::from(vec).enumerate().to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0].first, 0U);
    ASSERT_EQ(result[2].first, 2U);
    ASSERT_EQ(result[2].second, 30);
}

TEST(QueryTest, EnumerateValuesRestoresElements) {
    vector<int> vec = {7, 8, 9};
    auto values = ranges::from(vec).enumerate().values().to_vector();
    ASSERT_EQ(values.size(), 3U);
    ASSERT_EQ(values[1], 8);
}

TEST(QueryTest, ScanProducesRunningTotals) {
    vector<int> vec = {1, 2, 3};
    auto result = ranges::from(vec).scan(0, [](int acc, int x) { return acc + x; }).to_vector();
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0], 0);
    ASSERT_EQ(result[1], 1);
    ASSERT_EQ(result[2], 3);
    ASSERT_EQ(result[3], 6);
}

TEST(QueryTest, ScanOnEmptySourceYieldsSeedOnly) {
    vector<int> vec;
    auto result = ranges::from(vec).scan(5, [](int acc, int x) { return acc + x; }).to_vector();
    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result[0], 5);
}

TEST(QueryTest, ScanCanBuildString) {
    vector<char> letters = {'a', 'b', 'c'};
    auto result = ranges::from(letters)
                          .scan(string(""),
                                [](string acc, char c) {
                                    acc.push_back(c);
                                    return acc;
                                })
                          .to_vector();
    ASSERT_EQ(result.size(), 4U);
    ASSERT_EQ(result[0].size(), 0U);
    ASSERT_EQ(result[3].size(), 3U);
}

TEST(QueryTest, ToMapFromPairs) {
    vector<pair<int, int>> pairs = {{1, 10}, {2, 20}, {1, 30}};
    auto result = ranges::from(pairs).to_map();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result.at(1), 10);
    ASSERT_EQ(result.at(2), 20);
}

TEST(QueryTest, ToMapWithKeyAndValueFunctions) {
    vector<int> vec = {1, 2, 3};
    auto result = ranges::from(vec).to_map([](int x) { return x; }, [](int x) { return x * 10; });
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result.at(3), 30);
}

TEST(QueryTest, ToMapFromZippedPairs) {
    vector<int> left = {1, 2};
    vector<int> right = {10, 20};
    auto result = ranges::from(left).zip(right).to_map();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result.at(2), 20);
}

TEST(QueryTest, ToLookupGroupsByKey) {
    vector<int> vec = {1, 2, 3, 4, 5};
    auto lookup = ranges::from(vec).to_lookup([](int x) { return x % 2; });
    ASSERT_EQ(lookup.size(), 2U);
    ASSERT_EQ(lookup.at(1).size(), 3U);
    ASSERT_EQ(lookup.at(0).size(), 2U);
    ASSERT_EQ(lookup.at(0)[0], 2);
    ASSERT_EQ(lookup.at(0)[1], 4);
}

TEST(QueryTest, OwnedSourceCanBeChained) {
    auto result = ranges::from(vector<int>{1, 2, 3, 4}).filter([](int x) { return x % 2 == 0; }).to_vector();
    ASSERT_EQ(result.size(), 2U);
    ASSERT_EQ(result[0], 2);
    ASSERT_EQ(result[1], 4);
}

TEST(QueryTest, OwnedSourceChainsThroughMaterializedOperators) {
    auto result = ranges::from(vector<int>{3, 1, 2}).order_by([](int x) { return x; }).to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 1);
    auto unique = ranges::from(vector<int>{1, 1, 2}).distinct().to_vector();
    ASSERT_EQ(unique.size(), 2U);
    auto grouped = ranges::from(vector<int>{1, 1, 2}).group_by([](int x) { return x; });
    ASSERT_EQ(grouped.count(), 2U);
}

TEST(QueryTest, LazyOperatorsOnConstQueryCopyView) {
    vector<int> vec = {1, 2, 3};
    const auto query = ranges::from(vec);
    auto doubled = query.transform([](int x) { return x * 2; }).to_vector();
    ASSERT_EQ(doubled.size(), 3U);
    ASSERT_EQ(doubled[2], 6);
    ASSERT_EQ(query.count(), 3U);
}

TEST(QueryTest, MaterializedChainComposesWithLazyOperators) {
    vector<int> vec = {5, 2, 5, 1, 3};
    auto result = ranges::from(vec).distinct().order_by_descending([](int x) { return x; }).take(3).to_vector();
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result[0], 5);
    ASSERT_EQ(result[1], 3);
    ASSERT_EQ(result[2], 2);
}

TEST(QueryTest, GroupByThenMapToCounts) {
    vector<int> vec = {1, 1, 2, 3, 3, 3};
    auto counts = ranges::from(vec)
                          .group_by([](int x) { return x; })
                          .to_map([](auto group) { return group.key(); },
                                  [](auto group) { return ranges::from(group).count(); });
    ASSERT_EQ(counts.size(), 3U);
    ASSERT_EQ(counts.at(1), 2U);
    ASSERT_EQ(counts.at(3), 3U);
}

#endif
