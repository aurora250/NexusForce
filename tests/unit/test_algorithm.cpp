#include <NeForce/core/algorithm/algorithm.hpp>
#include <NeForce/core/algorithm/leonardo_heap.hpp>
#include <NeForce/core/container/deque.hpp>
#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/iterator/insert_iterator.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    auto g_less_comp = [](int a, int b) { return a < b; };
    auto g_greater_comp = [](int a, int b) { return a > b; };
    auto g_equal_to = [](int a, int b) { return a == b; };
    auto g_is_even = [](int x) { return x % 2 == 0; };
    auto g_adjacent_equal = [](int a, int b) { return a == b; };

    vector<int> sorted(vector<int> v) {
        sort(v.begin(), v.end());
        return v;
    }

    vector<int> random_vector(size_t n, uint32_t seed = 42) {
        random_mt mt(seed);
        vector<int> v(n);
        for (auto& x: v) {
            x = mt.next_int(-1000, 1000);
        }
        return v;
    }

    struct move_only {
        int value;
        explicit move_only(int v = 0) :
        value(v) {}
        move_only(const move_only&) = delete;
        move_only& operator=(const move_only&) = delete;
        move_only(move_only&& other) noexcept :
        value(other.value) {
            other.value = -1;
        }
        move_only& operator=(move_only&& other) noexcept {
            value = other.value;
            other.value = -1;
            return *this;
        }
        bool operator==(const move_only& other) const { return value == other.value; }
    };

    struct counting_predicate {
        int* count;
        explicit counting_predicate(int* c) :
        count(c) {}
        bool operator()(int x) const {
            ++(*count);
            return x % 2 == 0;
        }
    };

    struct tracker {
        static int copies;
        static int moves;
        int val;
        tracker(int v = 0) :
        val(v) {}
        tracker(const tracker& o) :
        val(o.val) {
            ++copies;
        }
        tracker(tracker&& o) noexcept :
        val(o.val) {
            o.val = 0;
            ++moves;
        }
        tracker& operator=(const tracker& o) {
            val = o.val;
            ++copies;
            return *this;
        }
        tracker& operator=(tracker&& o) noexcept {
            val = o.val;
            o.val = 0;
            ++moves;
            return *this;
        }
        bool operator==(const tracker& o) const { return val == o.val; }
        static void reset() { copies = moves = 0; }
    };
    int tracker::copies = 0;
    int tracker::moves = 0;

    struct stable_element {
        int value{0};
        int id{0};
        bool operator==(const stable_element& other) const { return value == other.value; }
        bool operator<(const stable_element& other) const { return value < other.value; }
    };

    struct compare_stable {
        bool operator()(const stable_element& a, const stable_element& b) const { return a.value < b.value; }
    };

    template <typename Iterator>
    class input_iterator_wrapper {
    public:
        using value_type = typename iterator_traits<Iterator>::value_type;
        using difference_type = typename iterator_traits<Iterator>::difference_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = input_iterator_tag;

        input_iterator_wrapper() :
        it_() {}
        explicit input_iterator_wrapper(Iterator it) :
        it_(it) {}

        value_type operator*() const { return *it_; }
        input_iterator_wrapper& operator++() {
            ++it_;
            return *this;
        }
        input_iterator_wrapper operator++(int) {
            auto tmp = *this;
            ++it_;
            return tmp;
        }
        bool operator==(const input_iterator_wrapper& other) const { return it_ == other.it_; }
        bool operator!=(const input_iterator_wrapper& other) const { return !(*this == other); }

    private:
        Iterator it_;
    };

    template <typename Iterator>
    input_iterator_wrapper<Iterator> make_input_wrapper(Iterator it) {
        return input_iterator_wrapper<Iterator>(it);
    }
} // namespace

TEST(LowerBoundTest, DefaultComparison) {
    vector<int> v{1, 2, 3, 3, 4, 5};
    auto it = lower_bound(v.begin(), v.end(), 3);
    EXPECT_EQ(it, v.begin() + 2);
    EXPECT_EQ(*it, 3);

    it = lower_bound(v.begin(), v.end(), 0);
    EXPECT_EQ(it, v.begin());

    it = lower_bound(v.begin(), v.end(), 6);
    EXPECT_EQ(it, v.end());

    vector<int> empty;
    EXPECT_EQ(lower_bound(empty.begin(), empty.end(), 1), empty.end());
}

TEST(LowerBoundTest, CustomComparison) {
    vector<int> v{5, 4, 3, 3, 2, 1};
    auto it = lower_bound(v.begin(), v.end(), 3, g_greater_comp);
    EXPECT_EQ(it, v.begin() + 2);
    EXPECT_EQ(*it, 3);
}

TEST(UpperBoundTest, DefaultComparison) {
    vector<int> v{1, 2, 3, 3, 4, 5};
    auto it = upper_bound(v.begin(), v.end(), 3);
    EXPECT_EQ(it, v.begin() + 4);

    it = upper_bound(v.begin(), v.end(), 0);
    EXPECT_EQ(it, v.begin());

    it = upper_bound(v.begin(), v.end(), 6);
    EXPECT_EQ(it, v.end());

    vector<int> empty;
    EXPECT_EQ(upper_bound(empty.begin(), empty.end(), 1), empty.end());
}

TEST(UpperBoundTest, CustomComparison) {
    vector<int> v{5, 4, 3, 3, 2, 1};
    const auto it = upper_bound(v.begin(), v.end(), 3, g_greater_comp);
    EXPECT_EQ(it, v.begin() + 4);
}

TEST(UpperBoundTest, LessComparison) {
    vector<int> v{1, 2, 3, 3, 4, 5};
    auto it = upper_bound(v.begin(), v.end(), 3, g_less_comp);
    EXPECT_EQ(it, v.begin() + 4);
    EXPECT_EQ(*it, 4);
}

TEST(BinarySearchTest, Default) {
    vector<int> v{1, 2, 3, 4, 5};
    EXPECT_TRUE(binary_search(v.begin(), v.end(), 3));
    EXPECT_FALSE(binary_search(v.begin(), v.end(), 6));
    EXPECT_FALSE(binary_search(v.begin(), v.begin(), 0));
}

TEST(BinarySearchTest, CustomComparison) {
    vector<int> v{5, 4, 3, 2, 1};
    EXPECT_TRUE(binary_search(v.begin(), v.end(), 3, g_greater_comp));
    EXPECT_FALSE(binary_search(v.begin(), v.end(), 6, g_greater_comp));
}

TEST(IncludesTest, Default) {
    vector<int> v1{1, 2, 3, 4, 5, 6};
    vector<int> v2{2, 3, 5};
    EXPECT_TRUE(includes(v1.begin(), v1.end(), v2.begin(), v2.end()));

    vector<int> v3{2, 3, 7};
    EXPECT_FALSE(includes(v1.begin(), v1.end(), v3.begin(), v3.end()));

    EXPECT_TRUE(includes(v1.begin(), v1.end(), v2.begin(), v2.begin()));

    vector<int> empty;
    EXPECT_FALSE(includes(empty.begin(), empty.end(), v2.begin(), v2.end()));
    EXPECT_TRUE(includes(v1.begin(), v1.end(), empty.begin(), empty.end()));
}

TEST(IncludesTest, CustomComparison) {
    vector<int> v1{6, 5, 4, 3, 2, 1};
    vector<int> v2{5, 3, 1};
    EXPECT_TRUE(includes(v1.begin(), v1.end(), v2.begin(), v2.end(), g_greater_comp));

    vector<int> v3{5, 4};
    EXPECT_TRUE(includes(v1.begin(), v1.end(), v3.begin(), v3.end(), g_greater_comp));
}

TEST(AllOfTest, Basic) {
    vector<int> v{2, 4, 6, 8};
    EXPECT_TRUE(all_of(v.begin(), v.end(), g_is_even));
    v.push_back(9);
    EXPECT_FALSE(all_of(v.begin(), v.end(), g_is_even));
    vector<int> empty;
    EXPECT_TRUE(all_of(empty.begin(), empty.end(), g_is_even));
}

TEST(AnyOfTest, Basic) {
    vector<int> v{1, 3, 5, 8};
    EXPECT_TRUE(any_of(v.begin(), v.end(), g_is_even));
    v.assign({1, 3, 5});
    EXPECT_FALSE(any_of(v.begin(), v.end(), g_is_even));
    vector<int> empty;
    EXPECT_FALSE(any_of(empty.begin(), empty.end(), g_is_even));
}

TEST(NoneOfTest, Basic) {
    vector<int> v{1, 3, 5, 7};
    EXPECT_TRUE(none_of(v.begin(), v.end(), g_is_even));
    v.push_back(10);
    EXPECT_FALSE(none_of(v.begin(), v.end(), g_is_even));
    vector<int> empty;
    EXPECT_TRUE(none_of(empty.begin(), empty.end(), g_is_even));
}

TEST(AdjacentFindTest, Default) {
    vector<int> v{1, 2, 2, 3, 4};
    auto it = adjacent_find(v.begin(), v.end());
    EXPECT_EQ(it, v.begin() + 1);
    EXPECT_EQ(*it, 2);

    vector<int> v2{1, 2, 3};
    EXPECT_EQ(adjacent_find(v2.begin(), v2.end()), v2.end());

    vector<int> empty;
    EXPECT_EQ(adjacent_find(empty.begin(), empty.end()), empty.end());
}

TEST(AdjacentFindTest, CustomPredicate) {
    vector<int> v{5, 4, 4, 2, 1};
    auto it = adjacent_find(v.begin(), v.end(), g_adjacent_equal);
    EXPECT_EQ(it, v.begin() + 1);
    EXPECT_EQ(*it, 4);
}

TEST(CountIfTest, WithValueAndPredicate) {
    vector<int> v{1, 2, 3, 2, 2, 4};
    auto n = count_if(v.begin(), v.end(), 2, g_equal_to);
    EXPECT_EQ(n, 3);

    n = count_if(v.begin(), v.end(), 5, g_equal_to);
    EXPECT_EQ(n, 0);

    vector<int> empty;
    EXPECT_EQ(count_if(empty.begin(), empty.end(), 1, g_equal_to), 0);
}

TEST(CountIfTest, WithPredicateOnly) {
    vector<int> v{2, 4, 6, 8, 9};
    auto n = count_if(v.begin(), v.end(), g_is_even);
    EXPECT_EQ(n, 4);

    n = count_if(v.begin(), v.end(), [](int x) { return x > 10; });
    EXPECT_EQ(n, 0);
}

TEST(CountTest, Basic) {
    vector<int> v{1, 2, 3, 2, 2, 4};
    EXPECT_EQ(count(v.begin(), v.end(), 2), 3);
    EXPECT_EQ(count(v.begin(), v.end(), 5), 0);
    vector<int> empty;
    EXPECT_EQ(count(empty.begin(), empty.end(), 0), 0);
}

TEST(FindTest, Basic) {
    vector<int> v{1, 2, 3, 4, 5};
    auto it = find(v.begin(), v.end(), 3);
    EXPECT_EQ(it, v.begin() + 2);
    EXPECT_EQ(find(v.begin(), v.end(), 6), v.end());
    vector<int> empty;
    EXPECT_EQ(find(empty.begin(), empty.end(), 0), empty.end());
}

TEST(FindIfTest, Basic) {
    vector<int> v{1, 3, 5, 6, 7};
    auto it = find_if(v.begin(), v.end(), g_is_even);
    EXPECT_EQ(it, v.begin() + 3);
    EXPECT_EQ(find_if(v.begin(), v.end(), [](int x) { return x > 10; }), v.end());
}

TEST(FindIfNotTest, Basic) {
    vector<int> v{2, 4, 6, 7, 8};
    auto it = find_if_not(v.begin(), v.end(), g_is_even);
    EXPECT_EQ(it, v.begin() + 3);
    EXPECT_EQ(find_if_not(v.begin(), v.end(), [](int x) { return x < 10; }), v.end());
}

TEST(SearchTest, Default) {
    vector<int> v{1, 2, 3, 4, 5, 6};
    vector<int> sub{3, 4};
    auto it = search(v.begin(), v.end(), sub.begin(), sub.end());
    EXPECT_EQ(it, v.begin() + 2);

    sub = {3, 5};
    EXPECT_EQ(search(v.begin(), v.end(), sub.begin(), sub.end()), v.end());

    sub.clear();
    EXPECT_EQ(search(v.begin(), v.end(), sub.begin(), sub.end()), v.begin());

    vector<int> empty;
    EXPECT_EQ(search(empty.begin(), empty.end(), sub.begin(), sub.end()), empty.end());

    sub = {1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(search(v.begin(), v.end(), sub.begin(), sub.end()), v.end());
}

TEST(SearchTest, CustomPredicate) {
    vector<int> v{1, 2, 3, 4, 5};
    vector<int> sub{3, 4};
    auto it = search(v.begin(), v.end(), sub.begin(), sub.end(), g_equal_to);
    EXPECT_EQ(it, v.begin() + 2);

    it = search(v.begin(), v.end(), sub.begin(), sub.end(), [](int a, int b) { return a == b; });
    EXPECT_EQ(it, v.begin() + 2);
}

TEST(SearchNTest, Default) {
    vector<int> v{1, 2, 2, 2, 3, 4};
    auto it = search_n(v.begin(), v.end(), 3, 2);
    EXPECT_EQ(it, v.begin() + 1);

    it = search_n(v.begin(), v.end(), 4, 2);
    EXPECT_EQ(it, v.end());

    it = search_n(v.begin(), v.end(), 1, 3);
    EXPECT_EQ(it, v.begin() + 4);

    vector<int> empty;
    EXPECT_EQ(search_n(empty.begin(), empty.end(), 1, 2), empty.end());
}

TEST(SearchNTest, CustomPredicate) {
    vector<int> v{5, 4, 4, 4, 3, 2};
    auto it = search_n(v.begin(), v.end(), 3, 4, g_equal_to);
    EXPECT_EQ(it, v.begin() + 1);

    it = search_n(v.begin(), v.end(), 2, 3, g_equal_to);
    EXPECT_EQ(it, v.end());
}

TEST(FindEndTest, VectorRandomAccess) {
    vector<int> v{1, 2, 3, 1, 2, 3};
    vector<int> sub{1, 2};
    auto it = find_end(v.begin(), v.end(), sub.begin(), sub.end());
    EXPECT_EQ(it, v.begin() + 3);

    sub = {3, 3};
    EXPECT_EQ(find_end(v.begin(), v.end(), sub.begin(), sub.end()), v.end());

    sub.clear();
    EXPECT_EQ(find_end(v.begin(), v.end(), sub.begin(), sub.end()), v.end());
}

TEST(FindEndTest, Empty) {
    vector<int> empty;
    vector<int> sub{1};
    EXPECT_EQ(find_end(empty.begin(), empty.end(), sub.begin(), sub.end()), empty.end());
}

TEST(FindFirstOfTest, Default) {
    vector<int> v{1, 2, 3, 4, 5};
    vector<int> pool{5, 3, 9};
    auto it = find_first_of(v.begin(), v.end(), pool.begin(), pool.end());
    EXPECT_EQ(it, v.begin() + 2);

    pool = {9, 10};
    EXPECT_EQ(find_first_of(v.begin(), v.end(), pool.begin(), pool.end()), v.end());

    pool.clear();
    EXPECT_EQ(find_first_of(v.begin(), v.end(), pool.begin(), pool.end()), v.end());
}

TEST(FindFirstOfTest, CustomPredicate) {
    vector<int> v{10, 20, 30, 40};
    vector<int> pool{30, 50};
    auto it = find_first_of(v.begin(), v.end(), pool.begin(), pool.end(), g_equal_to);
    EXPECT_EQ(it, v.begin() + 2);

    it = find_first_of(v.begin(), v.end(), pool.begin(), pool.end(), [](int a, int b) { return a % b == 0; });
    EXPECT_EQ(it, v.begin() + 2);
}

TEST(Equal, Basic) {
    vector<int> a{1, 2, 3};
    vector<int> b{1, 2, 3};
    EXPECT_TRUE(equal(a.begin(), a.end(), b.begin()));
}

TEST(Equal, DifferentValues) {
    vector<int> a{1, 2, 3};
    vector<int> b{1, 2, 4};
    EXPECT_FALSE(equal(a.begin(), a.end(), b.begin()));
}

TEST(Equal, DifferentLengthFirstShorter) {
    vector<int> a{1, 2};
    vector<int> b{1, 2, 3};
    EXPECT_TRUE(equal(a.begin(), a.end(), b.begin()));
}

TEST(Equal, Empty) {
    vector<int> a;
    vector<int> b{1, 2, 3};
    EXPECT_TRUE(equal(a.begin(), a.end(), b.begin()));
}

TEST(Equal, CustomPredicate) {
    vector<string> a{"abc", "def"};
    vector<string> b{"ABC", "DEF"};
    auto iequal = [](const string& x, const string& y) {
        return equal(x.begin(), x.end(), y.begin(),
                     [](char cx, char cy) { return to_lowercase(cx) == to_lowercase(cy); });
    };
    EXPECT_TRUE(equal(a.begin(), a.end(), b.begin(), iequal));
}

TEST(EqualRange, Found) {
    vector<int> v{1, 2, 2, 3, 4};
    auto p = equal_range(v.begin(), v.end(), 2);
    EXPECT_EQ(p.first - v.begin(), 1);
    EXPECT_EQ(p.second - v.begin(), 3);
}

TEST(EqualRange, NotFound) {
    vector<int> v{1, 2, 2, 3, 4};
    auto p = equal_range(v.begin(), v.end(), 5);
    EXPECT_EQ(p.first, p.second);
    EXPECT_EQ(p.first - v.begin(), 5);
}

TEST(EqualRange, MultipleEqualElements) {
    vector<int> v{0, 0, 0, 1, 1, 2};
    auto p = equal_range(v.begin(), v.end(), 1);
    EXPECT_EQ(p.first - v.begin(), 3);
    EXPECT_EQ(p.second - v.begin(), 5);
}

TEST(EqualRange, EmptyRange) {
    vector<int> v;
    auto p = equal_range(v.begin(), v.end(), 10);
    EXPECT_EQ(p.first, v.begin());
    EXPECT_EQ(p.second, v.begin());
}

TEST(EqualRange, CustomCompare) {
    vector<int> v{5, 4, 4, 3, 2, 1};
    auto p = equal_range(v.begin(), v.end(), 4, greater<>{});
    EXPECT_EQ(p.first - v.begin(), 1);
    EXPECT_EQ(p.second - v.begin(), 3);
}

TEST(MaxValue, Basic) {
    int a = 3, b = 5;
    EXPECT_EQ(&max(a, b), &b);
    EXPECT_EQ(max(7, 2), 7);
}

TEST(MaxValue, Equal) {
    int a = 4, b = 4;
    EXPECT_EQ(&max(a, b), &a);
}

TEST(MaxValue, CustomCompare) {
    auto abs_less = [](int x, int y) { return abs(x) < abs(y); };
    int a = -10, b = 5;
    EXPECT_EQ(&max(a, b, abs_less), &a);
}

TEST(MinValue, Basic) {
    int a = 3, b = 5;
    EXPECT_EQ(&min(a, b), &a);
    EXPECT_EQ(min(7, 2), 2);
}

TEST(MinValue, Equal) {
    int a = 4, b = 4;
    EXPECT_EQ(&min(a, b), &a);
}

TEST(MinValue, CustomCompare) {
    auto abs_less = [](int x, int y) { return abs(x) < abs(y); };
    int a = -10, b = 5;
    EXPECT_EQ(&min(a, b, abs_less), &b);
}

TEST(Median, Basic) {
    EXPECT_EQ(median(1, 2, 3), 2);
    EXPECT_EQ(median(3, 2, 1), 2);
    EXPECT_EQ(median(1, 3, 2), 2);
}

TEST(Median, EqualValues) {
    EXPECT_EQ(median(2, 2, 2), 2);
    EXPECT_EQ(median(1, 2, 2), 2);
    EXPECT_EQ(median(2, 1, 2), 2);
    EXPECT_EQ(median(2, 2, 1), 2);
}

TEST(Median, CustomCompare) {
    auto abs_less = [](int x, int y) { return abs(x) < abs(y); };
    EXPECT_EQ(median(-5, 1, -10, abs_less), -5);
    EXPECT_EQ(median(10, -1, 5, abs_less), 5);
}

TEST(MaxElement, Basic) {
    vector<int> v{1, 3, 2, 5, 4};
    auto it = max_element(v.begin(), v.end());
    EXPECT_EQ(*it, 5);
    EXPECT_EQ(it - v.begin(), 3);
}

TEST(MaxElement, MultipleMax) {
    vector<int> v{1, 5, 2, 5, 4};
    auto it = max_element(v.begin(), v.end());
    EXPECT_EQ(*it, 5);
    EXPECT_EQ(it - v.begin(), 1);
}

TEST(MaxElement, Empty) {
    vector<int> v;
    auto it = max_element(v.begin(), v.end());
    EXPECT_EQ(it, v.end());
}

TEST(MaxElement, CustomCompare) {
    auto abs_less = [](int x, int y) { return abs(x) < abs(y); };
    vector<int> v{1, -3, 2, -4, 5};
    auto it = max_element(v.begin(), v.end(), abs_less);
    EXPECT_EQ(*it, 5);
}

TEST(MaxElement, NonContiguousIterator) {
    list<int> v{1, 3, 2};
    auto it = max_element(v.begin(), v.end());
    EXPECT_EQ(*it, 3);
}

TEST(MinElement, Basic) {
    vector<int> v{3, 1, 2, 5, 4};
    auto it = min_element(v.begin(), v.end());
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(it - v.begin(), 1);
}

TEST(MinElement, MultipleMin) {
    vector<int> v{3, 1, 2, 1, 4};
    auto it = min_element(v.begin(), v.end());
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(it - v.begin(), 1);
}

TEST(MinElement, Empty) {
    vector<int> v;
    auto it = min_element(v.begin(), v.end());
    EXPECT_EQ(it, v.end());
}

TEST(MinElement, CustomCompare) {
    auto abs_less = [](int x, int y) { return abs(x) < abs(y); };
    vector<int> v{5, -1, 3, -2, 4};
    auto it = min_element(v.begin(), v.end(), abs_less);
    EXPECT_EQ(*it, -1);
}

TEST(MaxInitializerList, Basic) {
    auto result = max({1, 3, 2, 5, 4});
    EXPECT_EQ(result, 5);
}

TEST(MaxInitializerList, SingleElement) {
    auto result = max({42});
    EXPECT_EQ(result, 42);
}

TEST(MinInitializerList, Basic) {
    auto result = min({3, 1, 2, 5, 4});
    EXPECT_EQ(result, 1);
}

TEST(MinInitializerList, SingleElement) {
    auto result = min({42});
    EXPECT_EQ(result, 42);
}

TEST(MinMaxElement, Basic) {
    vector<int> v{3, 1, 2, 5, 4};
    auto p = minmax_element(v.begin(), v.end());
    EXPECT_EQ(*p.first, 1);
    EXPECT_EQ(*p.second, 5);
    EXPECT_EQ(p.first - v.begin(), 1);
    EXPECT_EQ(p.second - v.begin(), 3);
}

TEST(MinMaxElement, SingleElement) {
    vector<int> v{7};
    auto p = minmax_element(v.begin(), v.end());
    EXPECT_EQ(p.first, v.begin());
    EXPECT_EQ(p.second, v.begin());
    EXPECT_EQ(*p.first, 7);
}

TEST(MinMaxElement, Empty) {
    vector<int> v;
    auto p = minmax_element(v.begin(), v.end());
    EXPECT_EQ(p.first, v.end());
    EXPECT_EQ(p.second, v.end());
}

TEST(MinMaxElement, EvenCount) {
    vector<int> v{4, 2, 6, 1};
    auto p = minmax_element(v.begin(), v.end());
    EXPECT_EQ(*p.first, 1);
    EXPECT_EQ(*p.second, 6);
}

TEST(MinMaxElement, OddCount) {
    vector<int> v{4, 2, 6, 1, 5};
    auto p = minmax_element(v.begin(), v.end());
    EXPECT_EQ(*p.first, 1);
    EXPECT_EQ(*p.second, 6);
}

TEST(MinMaxElement, CustomCompare) {
    auto abs_less = [](int x, int y) { return abs(x) < abs(y); };
    vector<int> v{-4, 2, -6, 1, 5};
    auto p = minmax_element(v.begin(), v.end(), abs_less);
    EXPECT_EQ(*p.first, 1);
    EXPECT_EQ(*p.second, -6);
}

TEST(Clamp, Inside) { EXPECT_EQ(clamp(5, 1, 10), 5); }

TEST(Clamp, Below) {
    int value = 0;
    const int& result = clamp(value, 1, 10);
    EXPECT_EQ(result, 1);
    int low = 1, high = 10;
    EXPECT_EQ(&clamp(value, low, high), &low);
}

TEST(Clamp, Above) {
    int value = 20;
    int low = 1, high = 10;
    const int& result = clamp(value, low, high);
    EXPECT_EQ(result, 10);
    EXPECT_EQ(&result, &high);
}

TEST(Clamp, AtLowerBound) {
    int value = 1, low = 1, high = 10;
    const int& result = clamp(value, low, high);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(&result, &value);
}

TEST(Clamp, AtUpperBound) {
    int value = 10, low = 1, high = 10;
    const int& result = clamp(value, low, high);
    EXPECT_EQ(result, 10);
    EXPECT_EQ(&result, &value);
}

TEST(Clamp, CustomCompare) {
    auto comp = less<int>{};
    int value = 15, low = 10, high = 20;
    const int& result = clamp(value, low, high, comp);
    EXPECT_EQ(result, 15);
    EXPECT_EQ(&result, &value);
}

TEST(LexicographicalCompare, FirstIsPrefix) {
    vector<int> a{1, 2};
    vector<int> b{1, 2, 3};
    EXPECT_TRUE(lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

TEST(LexicographicalCompare, SecondIsPrefix) {
    vector<int> a{1, 2, 3};
    vector<int> b{1, 2};
    EXPECT_FALSE(lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

TEST(LexicographicalCompare, FirstLessAtPosition) {
    vector<int> a{1, 2, 3};
    vector<int> b{1, 3, 0};
    EXPECT_TRUE(lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

TEST(LexicographicalCompare, SecondLessAtPosition) {
    vector<int> a{1, 3, 0};
    vector<int> b{1, 2, 3};
    EXPECT_FALSE(lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

TEST(LexicographicalCompare, Equal) {
    vector<int> a{1, 2, 3};
    vector<int> b{1, 2, 3};
    EXPECT_FALSE(lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

TEST(LexicographicalCompare, CustomCompare) {
    auto comp = [](char a, char b) { return to_lowercase(a) < to_lowercase(b); };
    string a = "apple";
    string b = "Banana";
    EXPECT_TRUE(lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), comp));
}

TEST(Mismatch, AllMatch) {
    vector<int> a{1, 2, 3};
    vector<int> b{1, 2, 3, 4};
    auto p = mismatch(a.begin(), a.end(), b.begin());
    EXPECT_EQ(p.first, a.end());
    EXPECT_EQ(*p.second, 4);
    EXPECT_EQ(p.second - b.begin(), 3);
}

TEST(Mismatch, FirstMismatch) {
    vector<int> a{1, 2, 4};
    vector<int> b{1, 2, 3};
    auto p = mismatch(a.begin(), a.end(), b.begin());
    EXPECT_EQ(*p.first, 4);
    EXPECT_EQ(*p.second, 3);
}

TEST(Mismatch, Empty) {
    vector<int> a;
    vector<int> b{1, 2};
    auto p = mismatch(a.begin(), a.end(), b.begin());
    EXPECT_EQ(p.first, a.end());
    EXPECT_EQ(p.second, b.begin());
}

TEST(Mismatch, CustomPredicate) {
    auto iequal = [](const string& x, const string& y) {
        return x.size() == y.size() && equal(x.begin(), x.end(), y.begin(),
                                             [](char cx, char cy) { return to_lowercase(cx) == to_lowercase(cy); });
    };
    vector<string> a{"abc", "def", "Ghi"};
    vector<string> b{"ABC", "DEF", "ghi"};
    auto p = mismatch(a.begin(), a.end(), b.begin(), iequal);
    EXPECT_EQ(p.first, a.end());
    EXPECT_EQ(p.second, b.begin() + 3);
}

TEST(HeapTest, IsHeapUntilEmpty) {
    vector<int> v;
    EXPECT_EQ(is_heap_until(v.begin(), v.end()), v.end());
    EXPECT_EQ(is_heap_until(v.begin(), v.end(), less<>{}), v.end());
    EXPECT_EQ(is_heap_until(v.begin(), v.end(), greater<>{}), v.end());
}

TEST(HeapTest, IsHeapUntilSingle) {
    vector<int> v{5};
    EXPECT_EQ(is_heap_until(v.begin(), v.end()), v.end());
    EXPECT_EQ(is_heap_until(v.begin(), v.end(), less<>{}), v.end());
    EXPECT_EQ(is_heap_until(v.begin(), v.end(), greater<>{}), v.end());
}

TEST(HeapTest, IsHeapUntilValidMaxHeap) {
    vector<int> v{10, 8, 9, 3, 7, 2, 1};
    EXPECT_EQ(is_heap_until(v.begin(), v.end()), v.end());
    EXPECT_EQ(is_heap_until(v.begin(), v.end(), less<>{}), v.end());
}

TEST(HeapTest, IsHeapUntilValidMinHeap_Corrected) {
    vector<int> v{1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(is_heap_until(v.begin(), v.end(), greater<>{}), v.end());

    vector<int> v2{2, 1, 3};
    auto it = is_heap_until(v2.begin(), v2.end(), greater<>{});
    EXPECT_EQ(it, v2.begin() + 1);
}

TEST(HeapTest, IsHeapUntilInvalid) {
    vector<int> v{1, 2, 3};
    auto it = is_heap_until(v.begin(), v.end());
    EXPECT_EQ(it, v.begin() + 1);
    it = is_heap_until(v.begin(), v.end(), less<>{});
    EXPECT_EQ(it, v.begin() + 1);
}

TEST(HeapTest, IsHeapEmpty) {
    vector<int> v;
    EXPECT_TRUE(is_heap(v.begin(), v.end()));
    EXPECT_TRUE(is_heap(v.begin(), v.end(), less<>{}));
    EXPECT_TRUE(is_heap(v.begin(), v.end(), greater<>{}));
}

TEST(HeapTest, IsHeapSingle) {
    vector<int> v{7};
    EXPECT_TRUE(is_heap(v.begin(), v.end()));
    EXPECT_TRUE(is_heap(v.begin(), v.end(), less<>{}));
    EXPECT_TRUE(is_heap(v.begin(), v.end(), greater<>{}));
}

TEST(HeapTest, IsHeapValid) {
    vector<int> v{12, 10, 8, 3, 7, 5};
    EXPECT_TRUE(is_heap(v.begin(), v.end(), less<>{}));
    EXPECT_FALSE(is_heap(v.begin(), v.end(), greater<>{}));
}

TEST(HeapTest, PushHeapToEmpty) {
    vector<int> v{5};
    push_heap(v.begin(), v.end());
    EXPECT_EQ(v, vector<int>({5}));
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    vector<int> v2{3};
    push_heap(v2.begin(), v2.end(), greater<>{});
    EXPECT_EQ(v2, vector<int>({3}));
    EXPECT_TRUE(is_heap(v2.begin(), v2.end(), greater<>{}));
}

TEST(HeapTest, PushHeapSingleToHeap) {
    vector<int> v{10};
    v.push_back(20);
    push_heap(v.begin(), v.end());
    EXPECT_EQ(v, (vector<int>{20, 10}));
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    v.push_back(5);
    push_heap(v.begin(), v.end());
    EXPECT_EQ(v, (vector<int>{20, 10, 5}));
    EXPECT_TRUE(is_heap(v.begin(), v.end()));
}

TEST(HeapTest, PushHeapMaintainsHeap) {
    vector<int> v{16, 14, 10, 8, 7, 9, 3, 2, 4, 1};
    v.push_back(12);
    push_heap(v.begin(), v.end());
    EXPECT_TRUE(is_heap(v.begin(), v.end()));
}

TEST(HeapTest, PushHeapWithGreater) {
    vector<int> v1 = {1, 3, 5, 7, 9, 11};
    make_heap(v1.begin(), v1.end(), greater<>{});
    v1.push_back(4);
    push_heap(v1.begin(), v1.end(), greater<>{});

    vector<int> v2 = {1, 3, 5, 7, 9, 11};
    make_heap(v2.begin(), v2.end(), greater<>{});
    v2.push_back(4);
    push_heap(v2.begin(), v2.end(), greater<>{});

    EXPECT_TRUE(is_heap(v1.begin(), v1.end(), greater<>{}));
    EXPECT_TRUE(is_heap(v2.begin(), v2.end(), greater<>{}));

    EXPECT_EQ(v1.size(), v2.size());
}

TEST(HeapTest, PopHeapEmpty) {
    vector<int> v;
    pop_heap(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(HeapTest, PopHeapSingle) {
    vector<int> v{5};
    pop_heap(v.begin(), v.end());
    EXPECT_EQ(v, vector<int>({5}));
    EXPECT_TRUE(is_heap(v.begin(), v.end()));
}

TEST(HeapTest, PopHeapRemovesTop) {
    vector<int> v{20, 16, 18, 8, 14, 9, 3, 2, 4, 1};
    auto std_v = v;
    pop_heap(std_v.begin(), std_v.end());
    pop_heap(v.begin(), v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end() - 1));
    EXPECT_EQ(v.back(), 20);
}

TEST(HeapTest, PopHeapWithGreater) {
    vector<int> v{1, 2, 3, 5, 4, 7, 9, 8, 6};
    make_heap(v.begin(), v.end(), greater<>{});
    auto std_v = v;
    pop_heap(std_v.begin(), std_v.end(), greater<>{});
    pop_heap(v.begin(), v.end(), greater<>{});
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end() - 1, greater<>{}));
    EXPECT_EQ(v.back(), 1);
}

TEST(HeapTest, SortHeapEmpty) {
    vector<int> v;
    sort_heap(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(HeapTest, SortHeapSingle) {
    vector<int> v{42};
    sort_heap(v.begin(), v.end());
    EXPECT_EQ(v, vector<int>({42}));
}

TEST(HeapTest, SortHeapMaxHeap) {
    vector<int> v{20, 16, 18, 8, 14, 9, 3, 2, 4, 1};
    auto std_v = v;
    sort_heap(std_v.begin(), std_v.end());
    sort_heap(v.begin(), v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(HeapTest, SortHeapMinHeap) {
    vector<int> v{1, 2, 3, 5, 4, 7, 9, 8, 6};
    make_heap(v.begin(), v.end(), greater<>{});
    auto std_v = v;
    sort_heap(std_v.begin(), std_v.end(), greater<>{});
    sort_heap(v.begin(), v.end(), greater<>{});
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>{}));
}

TEST(HeapTest, MakeHeapFromRandom) {
    vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    auto std_v = v;
    make_heap(std_v.begin(), std_v.end());
    make_heap(v.begin(), v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end()));
}

TEST(HeapTest, MakeHeapWithGreater) {
    vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    auto std_v = v;
    make_heap(std_v.begin(), std_v.end(), greater<>{});
    make_heap(v.begin(), v.end(), greater<>{});
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end(), greater<>{}));
}

TEST(HeapTest, MakeHeapEmpty) {
    vector<int> v;
    make_heap(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
    make_heap(v.begin(), v.end(), greater<>{});
    EXPECT_TRUE(v.empty());
}

TEST(HeapTest, MakeHeapSingle) {
    vector<int> v{1};
    make_heap(v.begin(), v.end());
    EXPECT_EQ(v, vector<int>({1}));
    make_heap(v.begin(), v.end(), greater<>{});
    EXPECT_EQ(v, vector<int>({1}));
}

TEST(HeapTest, FullOperationSequence) {
    vector<int> v{4, 8, 1, 6, 3, 9, 2, 7, 5};
    auto std_v = v;

    make_heap(v.begin(), v.end());
    make_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    v.push_back(10);
    std_v.push_back(10);
    push_heap(v.begin(), v.end());
    push_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);

    v.push_back(0);
    std_v.push_back(0);
    push_heap(v.begin(), v.end());
    push_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);

    pop_heap(v.begin(), v.end());
    pop_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end() - 1));

    sort_heap(v.begin(), v.end() - 1);
    sort_heap(std_v.begin(), std_v.end() - 1);
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_sorted(v.begin(), v.end() - 1));
}

TEST(HeapTest, DuplicateElements) {
    vector<int> v{5, 5, 5, 5, 5};
    auto std_v = v;

    make_heap(v.begin(), v.end());
    make_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    v.push_back(5);
    std_v.push_back(5);
    push_heap(v.begin(), v.end());
    push_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);

    pop_heap(v.begin(), v.end());
    pop_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end() - 1));

    sort_heap(v.begin(), v.end());
    sort_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
}

TEST(HeapTest, CustomComparator) {
    vector<int> v = {1, 2, 3};
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{1, 3, 2}));
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{2, 1, 3}));
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{2, 3, 1}));
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{3, 1, 2}));
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{3, 2, 1}));
    EXPECT_FALSE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(HeapTest, DifferentContainer) {
    deque<int> d{8, 3, 1, 5, 9, 2, 6, 4, 7};
    auto std_d = d;

    make_heap(d.begin(), d.end());
    make_heap(std_d.begin(), std_d.end());
    EXPECT_EQ(d, std_d);
    EXPECT_TRUE(is_heap(d.begin(), d.end()));

    d.push_back(10);
    std_d.push_back(10);
    push_heap(d.begin(), d.end());
    push_heap(std_d.begin(), std_d.end());
    EXPECT_EQ(d, std_d);

    pop_heap(d.begin(), d.end());
    pop_heap(std_d.begin(), std_d.end());
    EXPECT_EQ(d, std_d);

    sort_heap(d.begin(), d.end());
    sort_heap(std_d.begin(), std_d.end());
    EXPECT_EQ(d, std_d);
}

TEST(HeapTest, NegativeValues) {
    vector<int> v{-5, -1, -10, 0, 3, -7, 2, -3, 8};
    make_heap(v.begin(), v.end());
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    v.push_back(-6);
    push_heap(v.begin(), v.end());
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    pop_heap(v.begin(), v.end());
    EXPECT_TRUE(is_heap(v.begin(), prev(v.end())));
    v.pop_back();

    sort_heap(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(HeapTest, LargeRandomSequence) {
    vector<int> v(1000);
    generate(v.begin(), v.end(), []() { return rand() % 10000; });
    auto std_v = v;

    make_heap(v.begin(), v.end());
    make_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_heap(v.begin(), v.end()));

    for (int i = 0; i < 100; ++i) {
        int val = rand() % 10000;
        v.push_back(val);
        std_v.push_back(val);
        push_heap(v.begin(), v.end());
        push_heap(std_v.begin(), std_v.end());
    }
    EXPECT_EQ(v, std_v);

    for (int i = 0; i < 50; ++i) {
        pop_heap(v.begin(), v.end());
        pop_heap(std_v.begin(), std_v.end());
        EXPECT_EQ(v.back(), std_v.back());
        v.pop_back();
        std_v.pop_back();
        EXPECT_EQ(v, std_v);
    }

    sort_heap(v.begin(), v.end());
    sort_heap(std_v.begin(), std_v.end());
    EXPECT_EQ(v, std_v);
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(MergeTest, BasicMergeInt) {
    vector<int> a{1, 3, 5, 7};
    vector<int> b{2, 4, 6, 8};
    vector<int> result(8);
    merge(a.begin(), a.end(), b.begin(), b.end(), result.begin());
    EXPECT_EQ(result, (vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST(MergeTest, EmptyRanges) {
    vector<int> a;
    vector<int> b{1, 2, 3};
    vector<int> r1(3);
    merge(a.begin(), a.end(), b.begin(), b.end(), r1.begin());
    EXPECT_EQ(r1, b);

    vector<int> r2(3);
    merge(b.begin(), b.end(), a.begin(), a.end(), r2.begin());
    EXPECT_EQ(r2, b);

    vector<int> r3(0);
    merge(a.begin(), a.end(), a.begin(), a.end(), r3.begin());
    EXPECT_TRUE(r3.empty());
}

TEST(MergeTest, Stability) {
    vector<stable_element> first{{1, 10}, {2, 20}, {2, 21}, {3, 30}};
    vector<stable_element> second{{2, 40}, {2, 41}, {4, 50}};
    vector<stable_element> result(7);
    merge(first.begin(), first.end(), second.begin(), second.end(), result.begin(), compare_stable{});
    vector<int> expected_ids{10, 20, 21, 40, 41, 30, 50};
    vector<int> got_ids;
    for (const auto& e: result) {
        got_ids.push_back(e.id);
    }
    EXPECT_EQ(got_ids, expected_ids);
}

TEST(MergeTest, TransparentComparator) {
    vector<int> a{1, 3, 5};
    vector<long> b{2L, 4L, 6L};
    vector<long> result(6);
    merge(a.begin(), a.end(), b.begin(), b.end(), result.begin());
    EXPECT_EQ(result, (vector<long>{1, 2, 3, 4, 5, 6}));
}

TEST(MergeTest, InputIterator) {
    vector<int> a{10, 30, 50};
    vector<int> b{20, 40, 60};
    vector<int> result(6);
    auto ia = make_input_wrapper(a.begin());
    auto ea = make_input_wrapper(a.end());
    auto ib = make_input_wrapper(b.begin());
    auto eb = make_input_wrapper(b.end());
    merge(ia, ea, ib, eb, result.begin());
    EXPECT_EQ(result, (vector<int>{10, 20, 30, 40, 50, 60}));
}

TEST(MergeTest, CustomComparator) {
    vector<int> a{5, 4, 3, 2};
    vector<int> b{8, 6, 1};
    sort(a.begin(), a.end(), greater<int>());
    sort(b.begin(), b.end(), greater<int>());
    vector<int> result(7);
    merge(a.begin(), a.end(), b.begin(), b.end(), result.begin(), greater<int>());
    vector<int> expected{8, 6, 5, 4, 3, 2, 1};
    EXPECT_EQ(result, expected);
}

TEST(InplaceMergeTest, Basic) {
    vector<int> v{1, 3, 5, 2, 4, 6};
    inplace_merge(v.begin(), v.begin() + 3, v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST(InplaceMergeTest, EmptySections) {
    vector<int> v{1, 2, 3};
    inplace_merge(v.begin(), v.begin(), v.end());
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
    inplace_merge(v.begin(), v.end(), v.end());
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(InplaceMergeTest, AlreadyMerged) {
    vector<int> v{1, 2, 3, 4, 5, 6};
    auto mid = v.begin() + 3;
    inplace_merge(v.begin(), mid, v.end());
    EXPECT_EQ(v, (vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST(InplaceMergeTest, Stability) {
    vector<stable_element> v{{1, 11}, {3, 33}, {5, 55}, {1, 12}, {2, 22}, {3, 34}};
    inplace_merge(v.begin(), v.begin() + 3, v.end(), compare_stable{});
    vector<int> ids;
    for (const auto& e: v) {
        ids.push_back(e.id);
    }
    vector<int> expected_ids{11, 12, 22, 33, 34, 55};
    EXPECT_EQ(ids, expected_ids);
}

TEST(RotateTest, BidirectionalList) {
    list<int> l{1, 3, 5, 2, 4, 6};
    auto mid = next(l.begin(), 3);
    rotate(l.begin(), mid, l.end());
    EXPECT_EQ(l, (list<int>{2, 4, 6, 1, 3, 5}));
}

TEST(InplaceMergeTest, BidirectionalIterator) {
    list<int> l{1, 3, 5, 2, 4, 6};
    auto mid = next(l.begin(), 3);
    inplace_merge(l.begin(), mid, l.end());
    EXPECT_TRUE(is_sorted(l.begin(), l.end()));
    EXPECT_EQ(l, (list<int>{1, 2, 3, 4, 5, 6}));
}

TEST(InplaceMergeTest, LargeRandomSequence) {
    constexpr int N = 1000;
    vector<int> v;
    for (int i = 0; i < N; ++i) {
        v.push_back(i);
    }
    shuffle(v.begin(), v.end());
    auto mid = v.begin() + N / 2;
    sort(v.begin(), mid);
    sort(mid, v.end());
    inplace_merge(v.begin(), mid, v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(CopyTest, BasicCopy) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(5);
    auto end = copy(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, src);
}

TEST(CopyTest, EmptyRange) {
    vector<int> empty;
    vector<int> dst;
    auto end = copy(empty.begin(), empty.end(), dst.begin());
    EXPECT_EQ(end, dst.begin());
}

TEST(CopyTest, NonContiguousIterators) {
    list<int> src = {10, 20, 30};
    list<int> dst(3);
    auto end = copy(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, src);
}

TEST(CopyTest, MoveOnlyElements) {
    vector<move_only> src;
    src.emplace_back(1);
    src.emplace_back(2);
    vector<move_only> dst(2);
    auto end = copy(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst[0].value, 1);
    EXPECT_EQ(dst[1].value, 2);
}

TEST(CopyNTest, BasicCopyN) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(3);
    auto result = copy_n(src.begin(), 3, dst.begin());
    EXPECT_EQ(result, dst.end());
    EXPECT_EQ(dst[0], 1);
    EXPECT_EQ(dst[1], 2);
    EXPECT_EQ(dst[2], 3);
}

TEST(CopyNTest, ZeroCount) {
    vector<int> src = {1, 2, 3};
    vector<int> dst;
    auto result = copy_n(src.begin(), 0, dst.begin());
    EXPECT_EQ(result, dst.begin());
}

TEST(CopyNTest, NonRandomAccess) {
    list<int> src = {100, 200, 300};
    vector<int> dst(2);
    auto result = copy_n(src.begin(), 2, dst.begin());
    EXPECT_EQ(result, dst.end());
    EXPECT_EQ(dst[0], 100);
    EXPECT_EQ(dst[1], 200);
}

TEST(CopyIfTest, EvenNumbers) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(5);
    auto end = copy_if(src.begin(), src.end(), dst.begin(), [](int x) { return x % 2 == 0; });
    vector<int> expected = {2, 4};
    EXPECT_EQ(vector<int>(dst.begin(), end), expected);
}

TEST(CopyIfTest, AllPass) {
    vector<int> src = {1, 2, 3};
    vector<int> dst(3);
    auto end = copy_if(src.begin(), src.end(), dst.begin(), [](int) { return true; });
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, src);
}

TEST(CopyIfTest, NonePass) {
    vector<int> src = {1, 2, 3};
    vector<int> dst(3);
    auto end = copy_if(src.begin(), src.end(), dst.begin(), [](int) { return false; });
    EXPECT_EQ(end, dst.begin());
}

TEST(CopyBackwardTest, Basic) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(5);
    auto last = copy_backward(src.begin(), src.end(), dst.end());
    EXPECT_EQ(last, dst.begin());
    EXPECT_EQ(dst, src);
}

TEST(CopyBackwardTest, OverlapSafe) {
    vector<int> v = {1, 2, 3, 4, 5};
    copy_backward(v.begin(), v.begin() + 3, v.end());
    EXPECT_EQ(v, (vector<int>{1, 2, 1, 2, 3}));
}

TEST(CopyBackwardTest, Empty) {
    vector<int> empty;
    auto result = copy_backward(empty.begin(), empty.end(), empty.end());
    EXPECT_EQ(result, empty.begin());
}

TEST(MoveTest, BasicMove) {
    vector<string> src = {"hello", "world"};
    vector<string> dst(2);
    auto end = move(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst[0], "hello");
    EXPECT_EQ(dst[1], "world");
    EXPECT_TRUE(src[0].empty());
    EXPECT_TRUE(src[1].empty());
}

TEST(MoveTest, MoveOnly) {
    vector<move_only> src;
    src.emplace_back(1);
    src.emplace_back(2);
    vector<move_only> dst(2);
    move(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(dst[0].value, 1);
    EXPECT_EQ(dst[1].value, 2);
    EXPECT_EQ(src[0].value, -1);
}

TEST(MoveTest, EmptyRange) {
    vector<int> empty;
    auto result = move(empty.begin(), empty.end(), empty.begin());
    EXPECT_EQ(result, empty.begin());
}

TEST(MoveBackwardTest, Basic) {
    vector<string> src = {"a", "b", "c", "d"};
    vector<string> dst(4);
    move_backward(src.begin(), src.end(), dst.end());
    EXPECT_EQ(dst[0], "a");
    EXPECT_EQ(dst[3], "d");
    EXPECT_TRUE(src[0].empty());
}

TEST(MoveBackwardTest, Overlap) {
    vector<string> v = {"1", "2", "3", "4", "5"};
    move_backward(v.begin(), v.begin() + 3, v.end());
    EXPECT_EQ(v[0], "");
    EXPECT_EQ(v[1], "");
    EXPECT_EQ(v[2], "1");
    EXPECT_EQ(v[3], "2");
    EXPECT_EQ(v[4], "3");
}

TEST(FillTest, Basic) {
    vector<int> v(5);
    fill(v.begin(), v.end(), 10);
    EXPECT_EQ(v, vector<int>(5, 10));
}

TEST(FillTest, Empty) {
    vector<int> v;
    fill(v.begin(), v.end(), 42);
    EXPECT_TRUE(v.empty());
}

TEST(FillNTest, Basic) {
    vector<int> v(5);
    auto it = fill_n(v.begin(), 3, 7);
    EXPECT_EQ(it, v.begin() + 3);
    EXPECT_EQ(v[0], 7);
    EXPECT_EQ(v[1], 7);
    EXPECT_EQ(v[2], 7);
    EXPECT_EQ(v[3], 0);
}

TEST(FillNTest, ZeroCount) {
    vector<int> v = {1, 2, 3};
    auto it = fill_n(v.begin(), 0, 99);
    EXPECT_EQ(it, v.begin());
    EXPECT_EQ(v[0], 1);
}

TEST(IterSwapTest, Basic) {
    int a = 1, b = 2;
    iter_swap(&a, &b);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);
}

TEST(IterSwapTest, SameElement) {
    int x = 5;
    iter_swap(&x, &x);
    EXPECT_EQ(x, 5);
}

TEST(SwapRangesTest, Basic) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6};
    auto it = swap_ranges(v1.begin(), v1.end(), v2.begin());
    EXPECT_EQ(it, v2.end());
    EXPECT_EQ(v1, (vector<int>{4, 5, 6}));
    EXPECT_EQ(v2, (vector<int>{1, 2, 3}));
}

TEST(SwapRangesTest, DifferentLengths) {
    vector<int> v1 = {1, 2, 3, 4};
    vector<int> v2 = {10, 20};
    auto it = swap_ranges(v1.begin(), v1.begin() + 2, v2.begin());
    EXPECT_EQ(it, v2.end());
    EXPECT_EQ(v1[0], 10);
    EXPECT_EQ(v1[1], 20);
    EXPECT_EQ(v1[2], 3);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[1], 2);
}

TEST(ForEachTest, Basic) {
    vector<int> v = {1, 2, 3};
    int sum = 0;
    for_each(v.begin(), v.end(), [&](int x) { sum += x; });
    EXPECT_EQ(sum, 6);
}

TEST(ForEachNTest, Basic) {
    vector<int> v = {10, 20, 30};
    int count = 0;
    auto it = for_each_n(v.begin(), 2, [&](int x) {
        count *= 10;
        count += x;
    });
    EXPECT_EQ(it, v.begin() + 2);
    EXPECT_EQ(count, 120);
}

TEST(GenerateTest, Basic) {
    vector<int> v(4);
    int n = 1;
    generate(v.begin(), v.end(), [&]() { return n++; });
    EXPECT_EQ(v, (vector<int>{1, 2, 3, 4}));
}

TEST(GenerateNTest, Basic) {
    vector<int> v(5);
    int counter = 10;
    auto it = generate_n(v.begin(), 3, [&]() { return counter += 2; });
    EXPECT_EQ(it, v.begin() + 3);
    EXPECT_EQ(v[0], 12);
    EXPECT_EQ(v[1], 14);
    EXPECT_EQ(v[2], 16);
}

TEST(ReplaceTest, Basic) {
    vector<int> v = {1, 2, 3, 2, 4};
    replace(v.begin(), v.end(), 2, 99);
    EXPECT_EQ(v, (vector<int>{1, 99, 3, 99, 4}));
}

TEST(ReplaceIfTest, EvenToZero) {
    vector<int> v = {1, 2, 3, 4, 5};
    replace_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }, 0);
    EXPECT_EQ(v, (vector<int>{1, 0, 3, 0, 5}));
}

TEST(ReplaceCopyTest, Basic) {
    vector<int> src = {1, 2, 3, 2, 1};
    vector<int> dst(5);
    auto end = replace_copy(src.begin(), src.end(), dst.begin(), 2, 9);
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, (vector<int>{1, 9, 3, 9, 1}));
}

TEST(ReplaceCopyIfTest, CopyOdd) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(5);
    auto end = replace_copy_if(src.begin(), src.end(), dst.begin(), [](int x) { return x % 2 != 0; }, -1);
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, (vector<int>{-1, 2, -1, 4, -1}));
}

TEST(ReverseTest, RandomAccess) {
    vector<int> v = {1, 2, 3, 4, 5};
    reverse(v.begin(), v.end());
    EXPECT_EQ(v, (vector<int>{5, 4, 3, 2, 1}));
}

TEST(ReverseTest, Bidirectional) {
    list<int> l = {10, 20, 30};
    reverse(l.begin(), l.end());
    EXPECT_EQ(l, (list<int>{30, 20, 10}));
}

TEST(ReverseTest, Empty) {
    vector<int> v;
    reverse(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(RotateTest, Basic) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto it = rotate(v.begin(), v.begin() + 2, v.end());
    EXPECT_EQ(it, v.begin() + 3);
    EXPECT_EQ(v, (vector<int>{3, 4, 5, 1, 2}));
}

TEST(RotateTest, MiddleAtBegin) {
    vector<int> v = {1, 2, 3};
    auto it = rotate(v.begin(), v.begin(), v.end());
    EXPECT_EQ(it, v.end());
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(RotateTest, MiddleAtEnd) {
    vector<int> v = {1, 2, 3};
    auto it = rotate(v.begin(), v.end(), v.end());
    EXPECT_EQ(it, v.begin());
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(RotateCopyTest, Basic) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(5);
    auto end = rotate_copy(src.begin(), src.begin() + 2, src.end(), dst.begin());
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, (vector<int>{3, 4, 5, 1, 2}));
}

TEST(ShiftLeftTest, BasicShift) {
    vector<string> v = {"a", "b", "c", "d", "e"};
    auto new_end = shift_left(v.begin(), v.end(), 2);
    EXPECT_EQ(new_end, v.begin() + 3);
    EXPECT_EQ(v[0], "c");
    EXPECT_EQ(v[1], "d");
    EXPECT_EQ(v[2], "e");
}

TEST(ShiftLeftTest, ShiftZero) {
    vector<int> v = {1, 2, 3};
    auto new_end = shift_left(v.begin(), v.end(), 0);
    EXPECT_EQ(new_end, v.end());
    EXPECT_EQ(v[0], 1);
}

TEST(ShiftLeftTest, ShiftExceedsSize) {
    vector<int> v = {1, 2, 3};
    auto new_end = shift_left(v.begin(), v.end(), 10);
    EXPECT_EQ(new_end, v.begin());
}

TEST(ShiftRightTest, BasicShift) {
    vector<string> v = {"a", "b", "c", "d", "e"};
    auto new_first = shift_right(v.begin(), v.end(), 2);
    EXPECT_EQ(new_first, v.begin() + 2);
    EXPECT_EQ(v[2], "a");
    EXPECT_EQ(v[3], "b");
    EXPECT_EQ(v[4], "c");
}

TEST(ShiftRightTest, ShiftZero) {
    vector<int> v = {1, 2, 3};
    auto new_first = shift_right(v.begin(), v.end(), 0);
    EXPECT_EQ(new_first, v.begin());
    EXPECT_EQ(v[0], 1);
}

TEST(ShiftRightTest, ShiftExceedsSize) {
    vector<int> v = {1, 2, 3};
    auto new_first = shift_right(v.begin(), v.end(), 5);
    EXPECT_EQ(new_first, v.end());
}

TEST(TransformUnaryTest, Square) {
    vector<int> src = {1, 2, 3};
    vector<int> dst(3);
    auto end = transform(src.begin(), src.end(), dst.begin(), [](int x) { return x * x; });
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, (vector<int>{1, 4, 9}));
}

TEST(TransformBinaryTest, Add) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6};
    vector<int> dst(3);
    auto end = transform(v1.begin(), v1.end(), v2.begin(), dst.begin(), plus<>{});
    EXPECT_EQ(end, dst.end());
    EXPECT_EQ(dst, (vector<int>{5, 7, 9}));
}

TEST(UniqueCopyTest, Basic) {
    vector<int> src = {1, 1, 2, 2, 3, 3};
    vector<int> dst(10);
    auto end = unique_copy(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(vector<int>(dst.begin(), end), (vector<int>{1, 2, 3}));
}

TEST(UniqueCopyTest, WithPredicate) {
    vector<int> src = {1, 2, 4, 6, 7};
    vector<int> dst(10);
    auto end = unique_copy(src.begin(), src.end(), dst.begin(), [](int a, int b) { return b - a <= 1; });
    EXPECT_EQ(vector<int>(dst.begin(), end), (vector<int>{1, 4, 6}));
}

TEST(UniqueTest, Basic) {
    vector<string> v = {"a", "a", "b", "b", "c"};
    auto new_end = unique(v.begin(), v.end());
    EXPECT_EQ(new_end, v.begin() + 3);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "c");
}

TEST(UniqueTest, NoDuplicates) {
    vector<int> v = {1, 2, 3};
    auto new_end = unique(v.begin(), v.end());
    EXPECT_EQ(new_end, v.end());
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(UniqueTest, WithPredicate) {
    vector<int> v = {1, 1, 2, 3, 3, 4};
    auto new_end = unique(v.begin(), v.end(), equal_to<>{});
    EXPECT_EQ(new_end, v.begin() + 4);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
}

TEST(UniqueTest, MoveOnly) {
    vector<move_only> v;
    v.emplace_back(1);
    v.emplace_back(1);
    v.emplace_back(2);
    auto new_end = unique(v.begin(), v.end());
    EXPECT_EQ(new_end, v.begin() + 2);
    EXPECT_EQ(v[0].value, 1);
    EXPECT_EQ(v[1].value, 2);
    EXPECT_EQ(v[2].value, -1);
}

TEST(NumericTest, Accumulate_Default) {
    vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(accumulate(v.begin(), v.end(), 0), 15);
    EXPECT_EQ(accumulate(v.begin(), v.end(), 10), 25);
}

TEST(NumericTest, Accumulate_DifferentInitType) {
    vector<int> v{1, 2, 3};
    double res = accumulate(v.begin(), v.end(), 0.5);
    EXPECT_DOUBLE_EQ(res, 6.5);
}

TEST(NumericTest, Accumulate_BinaryOp) {
    vector<int> v{1, 2, 3, 4};
    int res = accumulate(v.begin(), v.end(), 1, multiplies<>{});
    EXPECT_EQ(res, 24);
}

TEST(NumericTest, Accumulate_EmptyRange) {
    vector<int> v;
    EXPECT_EQ(accumulate(v.begin(), v.end(), 5), 5);
}

TEST(NumericTest, AdjacentDifference_Default) {
    vector<int> v{1, 3, 6, 10};
    vector<int> out(4);
    auto it = adjacent_difference(v.begin(), v.end(), out.begin());
    vector<int> expected{1, 2, 3, 4};
    EXPECT_EQ(out, expected);
    EXPECT_EQ(it, out.end());
}

TEST(NumericTest, AdjacentDifference_SingleElement) {
    vector<int> v{42};
    vector<int> out(1);
    auto it = adjacent_difference(v.begin(), v.end(), out.begin());
    EXPECT_EQ(out[0], 42);
    EXPECT_EQ(it, out.end());
}

TEST(NumericTest, AdjacentDifference_EmptyRange) {
    vector<int> v;
    vector<int> out;
    auto it = adjacent_difference(v.begin(), v.end(), out.begin());
    EXPECT_EQ(it, out.begin());
}

TEST(NumericTest, AdjacentDifference_CustomOp) {
    vector<int> v{1, 2, 3, 4};
    vector<int> out(4);
    adjacent_difference(v.begin(), v.end(), out.begin(), plus<>{});
    EXPECT_EQ(out, vector<int>({1, 3, 5, 7}));
}

TEST(NumericTest, AdjacentDifference_InPlace) {
    vector<int> v{1, 3, 6, 10};
    auto it = adjacent_difference(v.begin(), v.end(), v.begin());
    EXPECT_EQ(v, vector<int>({1, 2, 3, 4}));
    EXPECT_EQ(it, v.end());
}

TEST(NumericTest, InnerProduct_Default) {
    vector<int> a{1, 2, 3};
    vector<int> b{4, 5, 6};
    EXPECT_EQ(inner_product(a.begin(), a.end(), b.begin(), 0), 32);
    EXPECT_EQ(inner_product(a.begin(), a.end(), b.begin(), 10), 42);
}

TEST(NumericTest, InnerProduct_MixedType) {
    vector<int> a{1, 2, 3};
    vector<double> b{0.5, 0.5, 1.0};
    double res = inner_product(a.begin(), a.end(), b.begin(), 0.0);
    EXPECT_DOUBLE_EQ(res, 1 * 0.5 + 2 * 0.5 + 3 * 1.0);
}

TEST(NumericTest, InnerProduct_CustomOps) {
    vector<int> a{1, 2, 3};
    vector<int> b{4, 5, 6};
    int res = inner_product(a.begin(), a.end(), b.begin(), 0, plus<>{}, minus<>{});
    EXPECT_EQ(res, -9);
}

TEST(NumericTest, InnerProduct_EmptyRange) {
    vector<int> a, b;
    EXPECT_EQ(inner_product(a.begin(), a.end(), b.begin(), 100), 100);
}

TEST(NumericTest, PartialSum_Default) {
    vector<int> v{1, 2, 3, 4};
    vector<int> out(4);
    auto it = partial_sum(v.begin(), v.end(), out.begin());
    vector<int> expected{1, 3, 6, 10};
    EXPECT_EQ(out, expected);
    EXPECT_EQ(it, out.end());
}

TEST(NumericTest, PartialSum_SingleElement) {
    vector<int> v{7};
    vector<int> out(1);
    auto it = partial_sum(v.begin(), v.end(), out.begin());
    EXPECT_EQ(out[0], 7);
    EXPECT_EQ(it, out.end());
}

TEST(NumericTest, PartialSum_EmptyRange) {
    vector<int> v;
    vector<int> out;
    auto it = partial_sum(v.begin(), v.end(), out.begin());
    EXPECT_EQ(it, out.begin());
}

TEST(NumericTest, PartialSum_CustomOp) {
    vector<int> v{1, 2, 3, 4};
    vector<int> out(4);
    partial_sum(v.begin(), v.end(), out.begin(), multiplies<>{});
    EXPECT_EQ(out, vector<int>({1, 2, 6, 24}));
}

TEST(NumericTest, PartialSum_InPlace) {
    vector<int> v{1, 2, 3, 4};
    auto it = partial_sum(v.begin(), v.end(), v.begin());
    EXPECT_EQ(v, vector<int>({1, 3, 6, 10}));
    EXPECT_EQ(it, v.end());
}

TEST(NumericTest, Iota_Basic) {
    vector<int> v(5);
    sequence_fill(v.begin(), v.end(), 10);
    EXPECT_EQ(v, vector<int>({10, 11, 12, 13, 14}));
}

TEST(NumericTest, Iota_EmptyRange) {
    vector<int> v;
    sequence_fill(v.begin(), v.end(), 0);
    EXPECT_TRUE(v.empty());
}

TEST(NumericTest, Iota_DifferentValueType) {
    vector<double> v(3);
    sequence_fill(v.begin(), v.end(), 1.5);
    EXPECT_DOUBLE_EQ(v[0], 1.5);
    EXPECT_DOUBLE_EQ(v[1], 2.5);
    EXPECT_DOUBLE_EQ(v[2], 3.5);
}

TEST(NumericTest, BidirectionalIterator) {
    list<int> l{1, 2, 3};
    EXPECT_EQ(accumulate(l.begin(), l.end(), 0), 6);
    vector<int> out(l.size());
    adjacent_difference(l.begin(), l.end(), out.begin());
    EXPECT_EQ(out, vector<int>({1, 1, 1}));
    partial_sum(l.begin(), l.end(), out.begin());
    EXPECT_EQ(out, vector<int>({1, 3, 6}));
    list<int> l2{2, 2, 2};
    EXPECT_EQ(inner_product(l.begin(), l.end(), l2.begin(), 0), 2 + 4 + 6);
}

TEST(PartitionTest, EmptyRange) {
    vector<int> v;
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.end());
    EXPECT_TRUE(v.empty());
}

TEST(PartitionTest, SingleElementTrue) {
    vector<int> v{2};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.end());
    EXPECT_EQ(v, vector<int>({2}));
}

TEST(PartitionTest, SingleElementFalse) {
    vector<int> v{3};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.begin());
    EXPECT_EQ(v, vector<int>({3}));
}

TEST(PartitionTest, AllTrue) {
    vector<int> v{2, 4, 6, 8};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.end());
    EXPECT_EQ(v.size(), 4);
}

TEST(PartitionTest, AllFalse) {
    vector<int> v{1, 3, 5, 7};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(mid, v.begin());
    EXPECT_EQ(v, vector<int>({1, 3, 5, 7}));
}

TEST(PartitionTest, MixedValues) {
    vector<int> v{1, 2, 3, 4, 5, 6};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    bool ok = all_of(v.begin(), mid, [](int x) { return x % 2 == 0; }) &&
              all_of(mid, v.end(), [](int x) { return x % 2 != 0; });
    EXPECT_TRUE(ok);
    EXPECT_NE(mid, v.end());
    EXPECT_NE(mid, v.begin());
}

TEST(PartitionTest, ReturnIteratorPointsToFirstFalse) {
    vector<int> v{1, 2, 3, 4, 5};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x < 3; });
    for (auto it = v.begin(); it != mid; ++it) {
        EXPECT_LT(*it, 3);
    }
    for (auto it = mid; it != v.end(); ++it) {
        EXPECT_GE(*it, 3);
    }
}

TEST(PartitionTest, BidirectionalIterator) {
    list<int> v{1, 4, 2, 5, 3};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    EXPECT_TRUE(all_of(v.begin(), mid, [](int x) { return x % 2 == 0; }));
    EXPECT_TRUE(all_of(mid, v.end(), [](int x) { return x % 2 != 0; }));
}

TEST(PartitionTest, PredicateWithState) {
    vector<int> v{1, 2, 3, 4, 5, 6};
    int threshold = 3;
    auto pred = [&](int x) { return x < threshold; };
    auto mid = partition(v.begin(), v.end(), pred);
    for (auto it = v.begin(); it != mid; ++it) {
        EXPECT_LT(*it, threshold);
    }
    for (auto it = mid; it != v.end(); ++it) {
        EXPECT_GE(*it, threshold);
    }
}

TEST(PartitionTest, DuplicateValues) {
    vector<int> v{2, 2, 2, 1, 1, 1};
    auto mid = partition(v.begin(), v.end(), [](int x) { return x == 2; });
    bool ok =
            all_of(v.begin(), mid, [](int x) { return x == 2; }) && all_of(mid, v.end(), [](int x) { return x != 2; });
    EXPECT_TRUE(ok);
}

TEST(IsPermutationTest, BothEmpty) {
    vector<int> v1, v2;
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(IsPermutationTest, OneEmpty) {
    vector<int> v1 = {1};
    vector<int> v2;
    EXPECT_FALSE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
    EXPECT_FALSE(is_permutation(v2.begin(), v2.end(), v1.begin(), v1.end()));
}

TEST(IsPermutationTest, DifferentSize) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {1, 2};
    EXPECT_FALSE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(IsPermutationTest, SameSequence) {
    vector<int> v1 = {1, 2, 3, 4};
    vector<int> v2 = {1, 2, 3, 4};
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(IsPermutationTest, SimplePermutation) {
    vector<int> v1 = {1, 2, 3, 4};
    vector<int> v2 = {4, 2, 1, 3};
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(IsPermutationTest, DifferentElements) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {1, 2, 4};
    EXPECT_FALSE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(IsPermutationTest, WithDuplicates) {
    vector<int> v1 = {1, 2, 2, 3};
    vector<int> v2 = {2, 1, 3, 2};
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));

    vector<int> v3 = {1, 2, 3, 3};
    EXPECT_FALSE(is_permutation(v1.begin(), v1.end(), v3.begin(), v3.end()));
}

TEST(IsPermutationTest, PredicateVersion) {
    vector<string> v1 = {"a", "B", "c"};
    vector<string> v2 = {"A", "b", "C"};
    auto iequal = [](const string& a, const string& b) {
        return equal(a.begin(), a.end(), b.begin(),
                     [](char ca, char cb) { return to_lowercase(ca) == to_lowercase(cb); });
    };
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end(), iequal));
}

TEST(IsPermutationTest, TransparentComparator) {
    vector<int> v1 = {1, 2, 3};
    vector<long> v2 = {3L, 1L, 2L};
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));
}

TEST(IsPermutationTest, PrefixMismatchEarly) {
    vector<int> v1 = {1, 2, 3, 4};
    vector<int> v2 = {1, 3, 2, 4};
    EXPECT_TRUE(is_permutation(v1.begin(), v1.end(), v2.begin(), v2.end()));

    vector<int> v3 = {1, 2, 3, 4, 5};
    vector<int> v4 = {1, 2, 3, 4, 6};
    EXPECT_FALSE(is_permutation(v3.begin(), v3.end(), v4.begin(), v4.end()));
}

TEST(NextPermutationTest, EmptyRange) {
    vector<int> v;
    EXPECT_FALSE(next_permutation(v.begin(), v.end()));
    EXPECT_TRUE(v.empty());
}

TEST(NextPermutationTest, SingleElement) {
    vector<int> v = {5};
    EXPECT_FALSE(next_permutation(v.begin(), v.end()));
    EXPECT_EQ(v.front(), 5);
}

TEST(NextPermutationTest, BasicSequence) {
    vector<int> v = {1, 2, 3};
    vector<vector<int>> expected = {{1, 3, 2}, {2, 1, 3}, {2, 3, 1}, {3, 1, 2}, {3, 2, 1}};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_TRUE(next_permutation(v.begin(), v.end()));
        EXPECT_EQ(v, expected[i]);
    }
    EXPECT_FALSE(next_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(NextPermutationTest, DescendingOrder) {
    vector<int> v = {3, 2, 1};
    EXPECT_FALSE(next_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(NextPermutationTest, WithDuplicates) {
    vector<int> v = {1, 1, 2};
    EXPECT_TRUE(next_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 2, 1}));
    EXPECT_TRUE(next_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{2, 1, 1}));
    EXPECT_FALSE(next_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 1, 2}));
}

TEST(NextPermutationTest, CustomComparator) {
    vector<int> v = {3, 2, 1};
    EXPECT_TRUE(next_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{3, 1, 2}));
    EXPECT_TRUE(next_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{2, 3, 1}));
    EXPECT_TRUE(next_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{2, 1, 3}));
    EXPECT_TRUE(next_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{1, 3, 2}));
    EXPECT_TRUE(next_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
    EXPECT_FALSE(next_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{3, 2, 1}));
}

TEST(NextPermutationTest, NonContainerBidirectionalIterator) {
    list<int> l = {1, 2, 3};
    EXPECT_TRUE(next_permutation(l.begin(), l.end()));
    EXPECT_EQ(l, (list<int>{1, 3, 2}));
}

TEST(PrevPermutationTest, EmptyRange) {
    vector<int> v;
    EXPECT_FALSE(prev_permutation(v.begin(), v.end()));
    EXPECT_TRUE(v.empty());
}

TEST(PrevPermutationTest, SingleElement) {
    vector<int> v = {5};
    EXPECT_FALSE(prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v.front(), 5);
}

TEST(PrevPermutationTest, BasicSequence) {
    vector<int> v = {3, 2, 1};
    vector<vector<int>> expected = {{3, 1, 2}, {2, 3, 1}, {2, 1, 3}, {1, 3, 2}, {1, 2, 3}};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_TRUE(prev_permutation(v.begin(), v.end()));
        EXPECT_EQ(v, expected[i]);
    }
    EXPECT_FALSE(prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{3, 2, 1}));
}

TEST(PrevPermutationTest, AscendingOrder) {
    vector<int> v = {1, 2, 3};
    EXPECT_FALSE(prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{3, 2, 1}));
}

TEST(PrevPermutationTest, WithDuplicates) {
    vector<int> v = {2, 1, 1};
    EXPECT_TRUE(prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 2, 1}));
    EXPECT_TRUE(prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{1, 1, 2}));
    EXPECT_FALSE(prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, (vector<int>{2, 1, 1}));
}

TEST(PrevPermutationTest, DISABLED_CustomComparator) {
    vector<int> v = {1, 2, 3};
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{1, 3, 2}));
    EXPECT_TRUE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{3, 2, 1}));
    EXPECT_FALSE(prev_permutation(v.begin(), v.end(), greater<int>()));
    EXPECT_EQ(v, (vector<int>{1, 2, 3}));
}

TEST(PrevPermutationTest, BidirectionalIterator) {
    list<int> l = {3, 2, 1};
    EXPECT_TRUE(prev_permutation(l.begin(), l.end()));
    EXPECT_EQ(l, (list<int>{3, 1, 2}));
}

TEST(PermutationConsistency, NextPrevRoundTrip) {
    vector<int> original = {1, 3, 2};
    auto v = original;
    next_permutation(v.begin(), v.end());
    EXPECT_NE(v, original);
    prev_permutation(v.begin(), v.end());
    EXPECT_EQ(v, original);
}

TEST(PermutationConsistency, DISABLED_AllPermutationsGenerated) {
    vector<int> v = {1, 2, 2, 3};
    vector<vector<int>> all;
    auto start = v;
    do {
        all.push_back(v);
    } while (next_permutation(v.begin(), v.end()));

    EXPECT_EQ(all.size(), 12);
    auto sorted = all;
    sort(sorted.begin(), sorted.end());
    auto last = unique(sorted.begin(), sorted.end());
    EXPECT_EQ(distance(sorted.begin(), last), 12);
    EXPECT_EQ(v, (vector<int>{3, 2, 2, 1}));

    vector<vector<int>> rev;
    do {
        rev.push_back(v);
    } while (prev_permutation(v.begin(), v.end()));
    EXPECT_EQ(v, start);
    EXPECT_EQ(rev.size(), 12);
}

TEST(RemoveCopyTest, Basic) {
    vector<int> src = {1, 2, 3, 2, 4};
    vector<int> dst(5, 0);
    auto end = remove_copy(src.begin(), src.end(), dst.begin(), 2);
    vector<int> expected = {1, 3, 4};
    vector<int> result(dst.begin(), end);
    EXPECT_EQ(result, expected);
    EXPECT_EQ(src, (vector<int>{1, 2, 3, 2, 4}));
}

TEST(RemoveCopyTest, EmptySource) {
    vector<int> src;
    vector<int> dst;
    auto end = remove_copy(src.begin(), src.end(), dst.begin(), 0);
    EXPECT_EQ(end, dst.begin());
}

TEST(RemoveCopyTest, NoMatch) {
    vector<int> src = {1, 3, 5};
    vector<int> dst(3, 0);
    auto end = remove_copy(src.begin(), src.end(), dst.begin(), 2);
    vector<int> result(dst.begin(), end);
    EXPECT_EQ(result, src);
}

TEST(RemoveCopyTest, AllMatch) {
    vector<int> src = {2, 2, 2};
    vector<int> dst(3, 0);
    auto end = remove_copy(src.begin(), src.end(), dst.begin(), 2);
    EXPECT_EQ(end, dst.begin());
}

TEST(RemoveCopyIfTest, Basic) {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dst(5, 0);
    auto end = remove_copy_if(src.begin(), src.end(), dst.begin(), [](int x) { return x % 2 == 0; });
    vector<int> result(dst.begin(), end);
    EXPECT_EQ(result, (vector<int>{1, 3, 5}));
}

TEST(RemoveCopyIfTest, EmptySource) {
    vector<int> src;
    vector<int> dst;
    auto end = remove_copy_if(src.begin(), src.end(), dst.begin(), [](int) { return true; });
    EXPECT_EQ(end, dst.begin());
}

TEST(RemoveCopyIfTest, PredicateAlwaysTrue) {
    vector<int> src = {1, 2, 3};
    vector<int> dst(3, 0);
    auto end = remove_copy_if(src.begin(), src.end(), dst.begin(), [](int) { return true; });
    EXPECT_EQ(end, dst.begin());
}

TEST(RemoveCopyIfTest, PredicateAlwaysFalse) {
    vector<int> src = {1, 2, 3};
    vector<int> dst(3, 0);
    auto end = remove_copy_if(src.begin(), src.end(), dst.begin(), [](int) { return false; });
    vector<int> result(dst.begin(), end);
    EXPECT_EQ(result, src);
}

TEST(RemoveTest, Basic) {
    vector<int> data = {1, 2, 3, 2, 4};
    auto new_end = remove(data.begin(), data.end(), 2);
    vector<int> remaining(data.begin(), new_end);
    EXPECT_EQ(remaining, (vector<int>{1, 3, 4}));
    EXPECT_EQ(data.size(), 5);
}

TEST(RemoveTest, EmptyRange) {
    vector<int> data;
    auto new_end = remove(data.begin(), data.end(), 0);
    EXPECT_EQ(new_end, data.begin());
}

TEST(RemoveTest, NoMatch) {
    vector<int> data = {1, 3, 5};
    auto new_end = remove(data.begin(), data.end(), 2);
    EXPECT_EQ(new_end, data.end());
    EXPECT_EQ(data, (vector<int>{1, 3, 5}));
}

TEST(RemoveTest, AllMatch) {
    vector<int> data = {2, 2, 2};
    auto new_end = remove(data.begin(), data.end(), 2);
    EXPECT_EQ(new_end, data.begin());
}

TEST(RemoveTest, MoveOnly) {
    vector<move_only> data;
    data.emplace_back(1);
    data.emplace_back(2);
    data.emplace_back(3);
    data.emplace_back(2);
    auto new_end = remove(data.begin(), data.end(), move_only(2));
    vector<int> remaining_values;
    for (auto it = data.begin(); it != new_end; ++it) {
        remaining_values.push_back(it->value);
    }
    EXPECT_EQ(remaining_values, (vector<int>{1, 3}));
    EXPECT_EQ(data.size(), 4);
}

TEST(RemoveTest, OrderPreserved) {
    vector<int> data = {1, 2, 3, 2, 4, 2, 5};
    auto new_end = remove(data.begin(), data.end(), 2);
    vector<int> result(data.begin(), new_end);
    EXPECT_EQ(result, (vector<int>{1, 3, 4, 5}));
}

TEST(RemoveIfTest, Basic) {
    vector<int> data = {1, 2, 3, 4, 5};
    auto new_end = remove_if(data.begin(), data.end(), [](int x) { return x % 2 == 0; });
    vector<int> remaining(data.begin(), new_end);
    EXPECT_EQ(remaining, (vector<int>{1, 3, 5}));
}

TEST(RemoveIfTest, PredicateCalledOncePerElement) {
    vector<int> data = {1, 2, 3, 4};
    int count = 0;
    remove_if(data.begin(), data.end(), counting_predicate(&count));
    EXPECT_EQ(count, 4);
}

TEST(RemoveIfTest, MoveOnlyWithPredicate) {
    vector<move_only> data;
    data.emplace_back(1);
    data.emplace_back(2);
    data.emplace_back(3);
    auto new_end = remove_if(data.begin(), data.end(), [](const move_only& m) { return m.value % 2 == 0; });
    vector<int> values;
    for (auto it = data.begin(); it != new_end; ++it) {
        values.push_back(it->value);
    }
    EXPECT_EQ(values, (vector<int>{1, 3}));
}

TEST(EraseTest, Basic) {
    vector<int> cont = {1, 2, 3, 2, 4};
    size_t removed = erase(cont, 2);
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(cont, (vector<int>{1, 3, 4}));
}

TEST(EraseTest, EmptyContainer) {
    vector<int> cont;
    size_t removed = erase(cont, 0);
    EXPECT_EQ(removed, 0);
    EXPECT_TRUE(cont.empty());
}

TEST(EraseTest, NoMatch) {
    vector<int> cont = {1, 3, 5};
    size_t removed = erase(cont, 2);
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(cont, (vector<int>{1, 3, 5}));
}

TEST(EraseTest, AllMatch) {
    vector<int> cont = {2, 2, 2};
    size_t removed = erase(cont, 2);
    EXPECT_EQ(removed, 3);
    EXPECT_TRUE(cont.empty());
}

TEST(EraseTest, StringElements) {
    vector<string> cont = {"hello", "world", "hello", "test"};
    size_t removed = erase(cont, string("hello"));
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(cont, (vector<string>{"world", "test"}));
}

TEST(EraseTest, ContainerSizeAfterErase) {
    list<int> cont = {1, 2, 3, 2};
    size_t removed = erase(cont, 2);
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(cont.size(), 2);
    vector<int> result(cont.begin(), cont.end());
    EXPECT_EQ(result, (vector<int>{1, 3}));
}

TEST(EraseIfTest, Basic) {
    vector<int> cont = {1, 2, 3, 4, 5};
    size_t removed = erase_if(cont, [](int x) { return x % 2 == 0; });
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(cont, (vector<int>{1, 3, 5}));
}

TEST(EraseIfTest, PredicateWithState) {
    vector<int> cont = {1, 2, 3, 4};
    int count = 0;
    erase_if(cont, counting_predicate(&count));
    EXPECT_EQ(count, 4);
    EXPECT_EQ(cont, (vector<int>{1, 3}));
}

TEST(EraseIfTest, AllErased) {
    vector<int> cont = {2, 4, 6};
    size_t removed = erase_if(cont, [](int x) { return true; });
    EXPECT_EQ(removed, 3);
    EXPECT_TRUE(cont.empty());
}

TEST(EraseIfTest, NoneErased) {
    vector<int> cont = {1, 3, 5};
    size_t removed = erase_if(cont, [](int x) { return false; });
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(cont, (vector<int>{1, 3, 5}));
}

TEST(EraseIfTest, EmptyContainer) {
    vector<int> cont;
    size_t removed = erase_if(cont, [](int) { return true; });
    EXPECT_EQ(removed, 0);
}

TEST(SetUnionTest, BothEmpty) {
    vector<int> a, b, out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetUnionTest, FirstEmpty) {
    vector<int> a, b = sorted({2, 4, 5});
    vector<int> out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, b);
}

TEST(SetUnionTest, SecondEmpty) {
    vector<int> a = sorted({1, 3, 6});
    vector<int> b, out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, a);
}

TEST(SetUnionTest, NoOverlap) {
    vector<int> a = sorted({1, 3, 5});
    vector<int> b = sorted({2, 4, 6});
    vector<int> out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({1, 2, 3, 4, 5, 6}));
}

TEST(SetUnionTest, CompleteOverlap) {
    vector<int> a = sorted({2, 3, 4});
    vector<int> b = sorted({2, 3, 4});
    vector<int> out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, a);
}

TEST(SetUnionTest, PartialOverlapWithDuplicates) {
    vector<int> a = sorted({1, 1, 2, 2, 3});
    vector<int> b = sorted({2, 2, 2, 4});
    vector<int> out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({1, 1, 2, 2, 2, 3, 4}));
}

TEST(SetUnionTest, SimpleValues) {
    vector<int> a = sorted({5, 10, 15, 20});
    vector<int> b = sorted({10, 20, 30});
    vector<int> out;
    set_union(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({5, 10, 15, 20, 30}));
}

TEST(SetIntersectionTest, BothEmpty) {
    vector<int> a, b, out;
    set_intersection(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetIntersectionTest, OneEmpty) {
    vector<int> a = sorted({1, 2, 3});
    vector<int> b, out;
    set_intersection(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetIntersectionTest, NoCommonElements) {
    vector<int> a = sorted({1, 3, 5});
    vector<int> b = sorted({2, 4, 6});
    vector<int> out;
    set_intersection(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetIntersectionTest, AllCommon) {
    vector<int> a = sorted({1, 2, 3});
    vector<int> b = sorted({1, 2, 3});
    vector<int> out;
    set_intersection(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, a);
}

TEST(SetIntersectionTest, PartialOverlapWithDuplicates) {
    vector<int> a = sorted({1, 2, 2, 3, 3, 4});
    vector<int> b = sorted({2, 2, 3, 5});
    vector<int> out;
    set_intersection(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({2, 2, 3}));
}

TEST(SetIntersectionTest, SimpleValues) {
    vector<int> a = sorted({0, 10, 20, 30, 40});
    vector<int> b = sorted({10, 20, 50});
    vector<int> out;
    set_intersection(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    vector<int> expect = sorted({10, 20});
    EXPECT_EQ(out, expect);
}

TEST(SetDifferenceTest, BothEmpty) {
    vector<int> a, b, out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetDifferenceTest, FirstEmpty) {
    vector<int> a, b = sorted({1, 2, 3});
    vector<int> out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetDifferenceTest, SecondEmpty) {
    vector<int> a = sorted({1, 2, 3});
    vector<int> b, out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, a);
}

TEST(SetDifferenceTest, NoOverlap) {
    vector<int> a = sorted({1, 3, 5});
    vector<int> b = sorted({2, 4, 6});
    vector<int> out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, a);
}

TEST(SetDifferenceTest, AllRemoved) {
    vector<int> a = sorted({1, 2, 3});
    vector<int> b = sorted({1, 2, 3});
    vector<int> out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetDifferenceTest, PartialOverlapWithDuplicates) {
    vector<int> a = sorted({1, 1, 2, 2, 3, 3, 4});
    vector<int> b = sorted({2, 3, 3, 5});
    vector<int> out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({1, 1, 2, 4}));
}

TEST(SetDifferenceTest, SimpleValues) {
    vector<int> a = sorted({5, 10, 15, 20, 25});
    vector<int> b = sorted({10, 20, 30});
    vector<int> out;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({5, 15, 25}));
}

TEST(SetSymmetricDifferenceTest, BothEmpty) {
    vector<int> a, b, out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetSymmetricDifferenceTest, FirstEmpty) {
    vector<int> a, b = sorted({2, 4, 5});
    vector<int> out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, b);
}

TEST(SetSymmetricDifferenceTest, SecondEmpty) {
    vector<int> a = sorted({1, 3, 6});
    vector<int> b, out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, a);
}

TEST(SetSymmetricDifferenceTest, NoCommon) {
    vector<int> a = sorted({1, 3, 5});
    vector<int> b = sorted({2, 4, 6});
    vector<int> out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({1, 2, 3, 4, 5, 6}));
}

TEST(SetSymmetricDifferenceTest, AllCommon) {
    vector<int> a = sorted({1, 2, 3});
    vector<int> b = sorted({1, 2, 3});
    vector<int> out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>{});
}

TEST(SetSymmetricDifferenceTest, PartialOverlapWithDuplicates) {
    vector<int> a = sorted({1, 1, 2, 2, 3, 4});
    vector<int> b = sorted({2, 2, 2, 3, 5});
    vector<int> out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({1, 1, 2, 4, 5}));
}

TEST(SetSymmetricDifferenceTest, SimpleValues) {
    vector<int> a = sorted({5, 10, 15, 20});
    vector<int> b = sorted({10, 20, 30});
    vector<int> out;
    set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), make_back_inserter(out));
    EXPECT_EQ(out, vector<int>({5, 15, 30}));
}

TEST(IsSortedTest, EmptyRange) {
    vector<int> v;
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), less<>()));
}

TEST(IsSortedTest, SingleElement) {
    vector<int> v{5};
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(IsSortedTest, SortedRange) {
    vector<int> v{1, 2, 3, 4, 5};
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(IsSortedTest, UnsortedRange) {
    vector<int> v{1, 3, 2, 4, 5};
    EXPECT_FALSE(is_sorted(v.begin(), v.end()));
}

TEST(IsSortedTest, Duplicates) {
    vector<int> v{1, 1, 2, 2, 3};
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(IsSortedTest, ReverseSortedFails) {
    vector<int> v{5, 4, 3, 2, 1};
    EXPECT_FALSE(is_sorted(v.begin(), v.end()));
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(IsSortedUntilTest, Empty) {
    vector<int> v;
    EXPECT_EQ(is_sorted_until(v.begin(), v.end()), v.end());
}

TEST(IsSortedUntilTest, Single) {
    vector<int> v{42};
    EXPECT_EQ(is_sorted_until(v.begin(), v.end()), v.end());
}

TEST(IsSortedUntilTest, Sorted) {
    vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(is_sorted_until(v.begin(), v.end()), v.end());
}

TEST(IsSortedUntilTest, BreakAtPosition) {
    vector<int> v{1, 2, 3, 1, 4};
    auto it = is_sorted_until(v.begin(), v.end());
    EXPECT_EQ(distance(v.begin(), it), 3);
    EXPECT_EQ(*it, 1);
}

TEST(IsSortedUntilTest, BreakAtStart) {
    vector<int> v{2, 1, 3};
    EXPECT_EQ(is_sorted_until(v.begin(), v.end()), v.begin() + 1);
}

TEST(MergeSortTest, Empty) {
    vector<int> v;
    merge_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(MergeSortTest, Single) {
    vector<int> v{7};
    merge_sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 7);
}

TEST(MergeSortTest, BasicSort) {
    auto v = random_vector(100, 123);
    auto expected = v;
    sort(expected.begin(), expected.end());
    merge_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(MergeSortTest, AlreadySorted) {
    vector<int> v{1, 2, 3, 4, 5};
    auto copy = v;
    merge_sort(v.begin(), v.end());
    EXPECT_EQ(v, copy);
}

TEST(MergeSortTest, ReverseSorted) {
    vector<int> v{5, 4, 3, 2, 1};
    merge_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(MergeSortTest, Duplicates) {
    vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    auto expected = v;
    sort(expected.begin(), expected.end());
    merge_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(MergeSortTest, CustomComparator) {
    vector<int> v{5, 4, 3, 2, 1};
    merge_sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(MergeSortTest, Stability) {
    vector<stable_element> v{{3, 0}, {1, 0}, {3, 1}, {2, 0}, {1, 1}};
    vector<stable_element> expected{{1, 0}, {1, 1}, {2, 0}, {3, 0}, {3, 1}};
    merge_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.value < b.value; });
    EXPECT_EQ(v, expected);
}

TEST(PartialSortTest, Empty) {
    vector<int> v;
    partial_sort(v.begin(), v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(PartialSortTest, FullSort) {
    auto v = random_vector(50, 777);
    auto copy = v;
    sort(copy.begin(), copy.end());
    partial_sort(v.begin(), v.end(), v.end());
    EXPECT_EQ(v, copy);
}

TEST(PartialSortTest, PartialMiddle) {
    auto v = random_vector(30, 555);
    auto copy = v;
    const size_t k = 10;
    nth_element(copy.begin(), copy.begin() + k, copy.end());
    sort(copy.begin(), copy.begin() + k);
    partial_sort(v.begin(), v.begin() + k, v.end());
    for (size_t i = 0; i < k; ++i) {
        EXPECT_EQ(v[i], copy[i]);
    }
    if (k < v.size()) {
        int max_k = *max_element(v.begin(), v.begin() + k);
        for (size_t i = k; i < v.size(); ++i) {
            EXPECT_GE(v[i], max_k);
        }
    }
}

TEST(PartialSortTest, CustomComparator) {
    vector<int> v{5, 3, 1, 4, 2};
    partial_sort(v.begin(), v.begin() + 3, v.end(), greater<>());
    EXPECT_EQ(v[0], 5);
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[2], 3);
}

TEST(PartialSortCopyTest, Empty) {
    vector<int> src;
    vector<int> dest(5);
    auto end = partial_sort_copy(src.begin(), src.end(), dest.begin(), dest.end());
    EXPECT_EQ(end, dest.begin());
}

TEST(PartialSortCopyTest, LessThanDestCapacity) {
    vector<int> src{5, 2, 8, 1};
    vector<int> dest(10);
    auto end = partial_sort_copy(src.begin(), src.end(), dest.begin(), dest.end());
    EXPECT_EQ(distance(dest.begin(), end), 4);
    EXPECT_TRUE(is_sorted(dest.begin(), end));
}

TEST(PartialSortCopyTest, MoreThanDestCapacity) {
    vector<int> src{9, 3, 6, 1, 7, 2, 8};
    vector<int> dest(3);
    auto end = partial_sort_copy(src.begin(), src.end(), dest.begin(), dest.end());
    EXPECT_EQ(end, dest.end());
    EXPECT_TRUE(is_sorted(dest.begin(), dest.end()));
    vector<int> expected{1, 2, 3};
    EXPECT_EQ(dest, expected);
}

TEST(PartialSortCopyTest, DestExactSize) {
    vector<int> src = random_vector(50, 111);
    vector<int> dest(src.size());
    auto end = partial_sort_copy(src.begin(), src.end(), dest.begin(), dest.end());
    EXPECT_EQ(end, dest.end());
    auto sorted_src = src;
    sort(sorted_src.begin(), sorted_src.end());
    EXPECT_EQ(dest, sorted_src);
}

TEST(InsertionSortTest, Empty) {
    vector<int> v;
    insertion_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(InsertionSortTest, Basic) {
    auto v = random_vector(30, 42);
    auto expected = v;
    sort(expected.begin(), expected.end());
    insertion_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(InsertionSortTest, Sorted) {
    vector<int> v{1, 2, 3, 4, 5};
    insertion_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(InsertionSortTest, Reverse) {
    vector<int> v{5, 4, 3, 2, 1};
    insertion_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(InsertionSortTest, Stability) {
    vector<stable_element> v{{2, 0}, {1, 0}, {2, 1}, {1, 1}, {3, 0}};
    insertion_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.value < b.value; });
    vector<stable_element> expected{{1, 0}, {1, 1}, {2, 0}, {2, 1}, {3, 0}};
    EXPECT_EQ(v, expected);
}

TEST(IntroSortTest, Basic) {
    auto v = random_vector(200, 999);
    auto expected = v;
    sort(expected.begin(), expected.end());
    int depth = 2 * static_cast<int>(logarithm_2(v.size())) + 1;
    introspective_sort(v.begin(), v.end(), depth);
    EXPECT_EQ(v, expected);
}

TEST(IntroSortTest, DepthLimit) {
    vector<int> v(1000);
    sequence_fill(v.begin(), v.end(), 0);
    int depth = 0;
    introspective_sort(v.begin(), v.end(), depth);
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(IntroSortTest, Reverse) {
    vector<int> v(100);
    sequence_fill(v.rbegin(), v.rend(), 0);
    auto expected = v;
    sort(expected.begin(), expected.end());
    introspective_sort(v.begin(), v.end(), 10);
    EXPECT_EQ(v, expected);
}

TEST(QuickSortTest, Basic) {
    auto v = random_vector(50, 101);
    auto expected = v;
    sort(expected.begin(), expected.end());
    quick_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(QuickSortTest, AlreadySorted) {
    vector<int> v{1, 2, 3, 4, 5};
    quick_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(QuickSortTest, ReverseSorted) {
    vector<int> v{5, 4, 3, 2, 1};
    quick_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(QuickSortTest, AllEqual) {
    vector<int> v(20, 7);
    quick_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SortTest, Basic) {
    auto v = random_vector(500, 7777);
    auto expected = v;
    sort(expected.begin(), expected.end());
    sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(SortTest, Empty) {
    vector<int> v;
    sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(SortTest, Single) {
    vector<int> v{42};
    sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 42);
}

TEST(SortTest, Sorted) {
    vector<int> v(100);
    sequence_fill(v.begin(), v.end(), 0);
    sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SortTest, Reverse) {
    vector<int> v(100);
    sequence_fill(v.rbegin(), v.rend(), 0);
    sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SortTest, LargeRandom) {
    auto v = random_vector(10000, 12345);
    auto expected = v;
    sort(expected.begin(), expected.end());
    sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(SortTest, CustomComparator) {
    vector<int> v = random_vector(50, 555);
    sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(NthElementTest, Basic) {
    vector<int> v{5, 2, 9, 1, 5, 6};
    const size_t n = 3;
    nth_element(v.begin(), v.begin() + n, v.end());
    EXPECT_EQ(v[n], 5);
    for (size_t i = 0; i < n; ++i) {
        EXPECT_LE(v[i], v[n]);
    }
    for (size_t i = n + 1; i < v.size(); ++i) {
        EXPECT_GE(v[i], v[n]);
    }
}

TEST(NthElementTest, FirstElement) {
    vector<int> v{3, 1, 2};
    nth_element(v.begin(), v.begin(), v.end());
    EXPECT_EQ(v[0], 1);
}

TEST(NthElementTest, LastElement) {
    vector<int> v{3, 1, 2};
    nth_element(v.begin(), v.end() - 1, v.end());
    EXPECT_EQ(v.back(), 3);
}

TEST(NthElementTest, Random) {
    for (int run = 0; run < 10; ++run) {
        auto v = random_vector(50, 100 + run);
        auto sorted = v;
        sort(sorted.begin(), sorted.end());
        const size_t n = 17;
        nth_element(v.begin(), v.begin() + n, v.end());
        EXPECT_EQ(v[n], sorted[n]);
        for (size_t i = 0; i < n; ++i) {
            EXPECT_LE(v[i], v[n]);
        }
        for (size_t i = n + 1; i < v.size(); ++i) {
            EXPECT_GE(v[i], v[n]);
        }
    }
}

TEST(NthElementTest, CustomComparator) {
    vector<int> v{1, 5, 2, 8, 3};
    nth_element(v.begin(), v.begin() + 2, v.end(), greater<>());
    EXPECT_EQ(v[2], 3);
    EXPECT_GE(v[0], v[2]);
    EXPECT_GE(v[1], v[2]);
    EXPECT_LE(v[3], v[2]);
    EXPECT_LE(v[4], v[2]);
}

TEST(BubbleSortTest, Empty) {
    vector<int> v;
    bubble_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(BubbleSortTest, SingleElement) {
    vector<int> v{42};
    bubble_sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 42);
}

TEST(BubbleSortTest, BasicSort) {
    auto v = random_vector(50, 111);
    auto expected = v;
    sort(expected.begin(), expected.end());
    bubble_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(BubbleSortTest, AlreadySorted) {
    vector<int> v{1, 2, 3, 4, 5};
    auto copy = v;
    bubble_sort(v.begin(), v.end());
    EXPECT_EQ(v, copy);
}

TEST(BubbleSortTest, ReverseSorted) {
    vector<int> v{5, 4, 3, 2, 1};
    bubble_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(BubbleSortTest, Duplicates) {
    vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    auto expected = v;
    sort(expected.begin(), expected.end());
    bubble_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(BubbleSortTest, CustomComparator) {
    vector<int> v{5, 4, 3, 2, 1};
    bubble_sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(BubbleSortTest, Stability) {
    vector<stable_element> v{{3, 0}, {1, 0}, {3, 1}, {2, 0}, {1, 1}};
    bubble_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.value < b.value; });
    vector<stable_element> expected{{1, 0}, {1, 1}, {2, 0}, {3, 0}, {3, 1}};
    EXPECT_EQ(v, expected);
}

TEST(CocktailSortTest, Empty) {
    vector<int> v;
    cocktail_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(CocktailSortTest, Single) {
    vector<int> v{7};
    cocktail_sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 7);
}

TEST(CocktailSortTest, Basic) {
    auto v = random_vector(30, 222);
    auto expected = v;
    sort(expected.begin(), expected.end());
    cocktail_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(CocktailSortTest, Sorted) {
    vector<int> v{1, 2, 3, 4, 5};
    cocktail_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(CocktailSortTest, Reverse) {
    vector<int> v{5, 4, 3, 2, 1};
    cocktail_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(CocktailSortTest, CustomComparator) {
    vector<int> v{5, 4, 3, 2, 1};
    cocktail_sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(CocktailSortTest, Stability) {
    vector<stable_element> v{{2, 0}, {1, 0}, {2, 1}, {1, 1}, {3, 0}};
    cocktail_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.value < b.value; });
    vector<stable_element> expected{{1, 0}, {1, 1}, {2, 0}, {2, 1}, {3, 0}};
    EXPECT_EQ(v, expected);
}

TEST(SelectSortTest, Empty) {
    vector<int> v;
    select_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(SelectSortTest, Single) {
    vector<int> v{99};
    select_sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 99);
}

TEST(SelectSortTest, Basic) {
    auto v = random_vector(40, 333);
    auto expected = v;
    sort(expected.begin(), expected.end());
    select_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(SelectSortTest, AlreadySorted) {
    vector<int> v{1, 2, 3, 4, 5};
    select_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SelectSortTest, Reverse) {
    vector<int> v{5, 4, 3, 2, 1};
    select_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SelectSortTest, CustomComparator) {
    vector<int> v{5, 4, 3, 2, 1};
    select_sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(SelectSortTest, Duplicates) {
    vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    auto expected = v;
    sort(expected.begin(), expected.end());
    select_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(ShellSortTest, Empty) {
    vector<int> v;
    shell_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(ShellSortTest, Single) {
    vector<int> v{13};
    shell_sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 13);
}

TEST(ShellSortTest, Basic) {
    auto v = random_vector(50, 444);
    auto expected = v;
    sort(expected.begin(), expected.end());
    shell_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(ShellSortTest, Sorted) {
    vector<int> v{1, 2, 3, 4, 5};
    shell_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(ShellSortTest, Reverse) {
    vector<int> v{5, 4, 3, 2, 1};
    shell_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(ShellSortTest, CustomComparator) {
    vector<int> v{5, 4, 3, 2, 1};
    shell_sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(ShellSortTest, LargeRandom) {
    auto v = random_vector(200, 7777);
    auto expected = v;
    sort(expected.begin(), expected.end());
    shell_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(ShellSortTest, Duplicates) {
    vector<int> v(50, 5);
    shell_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SmoothSortTest, Empty) {
    vector<int> v;
    smooth_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(SmoothSortTest, Single) {
    vector<int> v{42};
    smooth_sort(v.begin(), v.end());
    EXPECT_EQ(v[0], 42);
}

TEST(SmoothSortTest, Basic) {
    auto v = random_vector(60, 555);
    auto expected = v;
    sort(expected.begin(), expected.end());
    smooth_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(SmoothSortTest, Sorted) {
    vector<int> v{1, 2, 3, 4, 5};
    smooth_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SmoothSortTest, Reverse) {
    vector<int> v{5, 4, 3, 2, 1};
    smooth_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(SmoothSortTest, Duplicates) {
    vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    auto expected = v;
    sort(expected.begin(), expected.end());
    smooth_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(SmoothSortTest, LargeRandom) {
    auto v = random_vector(1000, 12345);
    auto expected = v;
    sort(expected.begin(), expected.end());
    smooth_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(SmoothSortTest, CustomType) {
    vector<string> v{"apple", "banana", "cherry", "date", "fig"};
    auto expected = v;
    sort(expected.begin(), expected.end());
    smooth_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(TimSortTest, Empty) {
    vector<int> v;
    tim_sort(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
}

TEST(TimSortTest, Basic) {
    auto v = random_vector(100, 666);
    auto expected = v;
    sort(expected.begin(), expected.end());
    tim_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(TimSortTest, AlreadySorted) {
    vector<int> v{1, 2, 3, 4, 5};
    auto copy = v;
    tim_sort(v.begin(), v.end());
    EXPECT_EQ(v, copy);
}

TEST(TimSortTest, ReverseSorted) {
    vector<int> v{5, 4, 3, 2, 1};
    tim_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(TimSortTest, Duplicates) {
    vector<int> v(100, 7);
    tim_sort(v.begin(), v.end());
    EXPECT_TRUE(is_sorted(v.begin(), v.end()));
}

TEST(TimSortTest, CustomComparator) {
    vector<int> v{5, 4, 3, 2, 1};
    tim_sort(v.begin(), v.end(), greater<>());
    EXPECT_TRUE(is_sorted(v.begin(), v.end(), greater<>()));
}

TEST(TimSortTest, Stability) {
    vector<stable_element> v{{3, 0}, {1, 0}, {3, 1}, {2, 0}, {1, 1}};
    tim_sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.value < b.value; });
    vector<stable_element> expected{{1, 0}, {1, 1}, {2, 0}, {3, 0}, {3, 1}};
    EXPECT_EQ(v, expected);
}

TEST(TimSortTest, LargeRandom) {
    auto v = random_vector(1000, 9999);
    auto expected = v;
    sort(expected.begin(), expected.end());
    tim_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

TEST(TimSortTest, ExactlyMultiplesOfMinRun) {
    vector<int> v(64);
    sequence_fill(v.rbegin(), v.rend(), 0);
    auto expected = v;
    sort(expected.begin(), expected.end());
    tim_sort(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}
