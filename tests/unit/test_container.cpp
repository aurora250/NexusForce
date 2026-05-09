#include <NeForce/core/algorithm/numeric.hpp>
#include <NeForce/core/algorithm/sort.hpp>
#include <NeForce/core/container/bitmap.hpp>
#include <NeForce/core/container/bitset.hpp>
#include <NeForce/core/container/bloom_filter.hpp>
#include <NeForce/core/container/deque.hpp>
#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/map.hpp>
#include <NeForce/core/container/multimap.hpp>
#include <NeForce/core/container/multiset.hpp>
#include <NeForce/core/container/priority_queue.hpp>
#include <NeForce/core/container/queue.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/container/stack.hpp>
#include <NeForce/core/container/unordered_map.hpp>
#include <NeForce/core/container/unordered_multimap.hpp>
#include <NeForce/core/container/unordered_multiset.hpp>
#include <NeForce/core/container/unordered_set.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
using namespace neforce;

TEST(ArrayTest, DefaultConstructor) {
    array<int, 5> arr;
    EXPECT_EQ(arr.size(), 5);
    EXPECT_FALSE(arr.empty());
    EXPECT_EQ(arr.max_size(), 5);
}

TEST(ArrayTest, InitializerListConstructor) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 4);
    EXPECT_EQ(arr[4], 5);
}

TEST(ArrayTest, InitializerListConstructorPartial) {
    array<int, 5> arr = {1, 2, 3};
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST(ArrayTest, CopyConstructor) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2(arr1);
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
    EXPECT_EQ(arr2[2], 3);
}

TEST(ArrayTest, CopyAssignment) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {4, 5, 6};
    arr2 = arr1;
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
    EXPECT_EQ(arr2[2], 3);
}

TEST(ArrayTest, MoveConstructor) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2(move(arr1));
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
    EXPECT_EQ(arr2[2], 3);
}

TEST(ArrayTest, MoveAssignment) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {4, 5, 6};
    arr2 = move(arr1);
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
    EXPECT_EQ(arr2[2], 3);
}

TEST(ArrayTest, BeginEnd) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    auto end_it = arr.end();
    EXPECT_EQ(end_it - arr.begin(), 5);
}

TEST(ArrayTest, ConstBeginEnd) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    auto end_it = arr.end();
    EXPECT_EQ(end_it - arr.begin(), 5);
}

TEST(ArrayTest, ReverseBeginEnd) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto rit = arr.rbegin();
    EXPECT_EQ(*rit, 5);
    ++rit;
    EXPECT_EQ(*rit, 4);
    auto rend_it = arr.rend();
    EXPECT_EQ(rend_it - arr.rbegin(), 5);
}

TEST(ArrayTest, ConstReverseBeginEnd) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    auto rit = arr.rbegin();
    EXPECT_EQ(*rit, 5);
    ++rit;
    EXPECT_EQ(*rit, 4);
    auto rend_it = arr.rend();
    EXPECT_EQ(rend_it - arr.rbegin(), 5);
}

TEST(ArrayTest, CbeginCend) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.cbegin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    auto end_it = arr.cend();
    EXPECT_EQ(end_it - arr.cbegin(), 5);
}

TEST(ArrayTest, CrbeginCrend) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto rit = arr.crbegin();
    EXPECT_EQ(*rit, 5);
    ++rit;
    EXPECT_EQ(*rit, 4);
    auto rend_it = arr.crend();
    EXPECT_EQ(rend_it - arr.crbegin(), 5);
}

TEST(ArrayTest, Size) {
    array<int, 5> arr;
    EXPECT_EQ(arr.size(), 5);
}

TEST(ArrayTest, MaxSize) {
    array<int, 5> arr;
    EXPECT_EQ(arr.max_size(), 5);
}

TEST(ArrayTest, Empty) {
    array<int, 5> arr;
    EXPECT_FALSE(arr.empty());
}

TEST(ArrayTest, At) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.at(0), 1);
    EXPECT_EQ(arr.at(4), 5);
    arr.at(2) = 10;
    EXPECT_EQ(arr.at(2), 10);
}

TEST(ArrayTest, ConstAt) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.at(0), 1);
    EXPECT_EQ(arr.at(4), 5);
}

TEST(ArrayTest, SubscriptOperator) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[4], 5);
    arr[2] = 10;
    EXPECT_EQ(arr[2], 10);
}

TEST(ArrayTest, ConstSubscriptOperator) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[4], 5);
}

TEST(ArrayTest, Front) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.front(), 1);
    arr.front() = 10;
    EXPECT_EQ(arr.front(), 10);
}

TEST(ArrayTest, ConstFront) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.front(), 1);
}

TEST(ArrayTest, Back) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.back(), 5);
    arr.back() = 10;
    EXPECT_EQ(arr.back(), 10);
}

TEST(ArrayTest, ConstBack) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.back(), 5);
}

TEST(ArrayTest, Data) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    int* ptr = arr.data();
    EXPECT_EQ(*ptr, 1);
    EXPECT_EQ(*(ptr + 4), 5);
    ptr[0] = 10;
    EXPECT_EQ(arr[0], 10);
}

TEST(ArrayTest, ConstData) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    const int* ptr = arr.data();
    EXPECT_EQ(*ptr, 1);
    EXPECT_EQ(*(ptr + 4), 5);
}

TEST(ArrayTest, Fill) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    arr.fill(42);
    for (size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(arr[i], 42);
    }
}

TEST(ArrayTest, Swap) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {4, 5, 6};
    arr1.swap(arr2);
    EXPECT_EQ(arr1[0], 4);
    EXPECT_EQ(arr1[1], 5);
    EXPECT_EQ(arr1[2], 6);
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
    EXPECT_EQ(arr2[2], 3);
}

TEST(ArrayTest, EqualTo) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {1, 2, 3};
    array<int, 3> arr3 = {4, 5, 6};
    EXPECT_TRUE(arr1.equal_to(arr2));
    EXPECT_FALSE(arr1.equal_to(arr3));
}

TEST(ArrayTest, LessThan) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {1, 2, 4};
    array<int, 3> arr3 = {1, 2, 2};
    EXPECT_TRUE(arr1.less_than(arr2));
    EXPECT_FALSE(arr1.less_than(arr3));
    EXPECT_FALSE(arr2.less_than(arr1));
}

TEST(ArrayTest, EqualityOperator) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {1, 2, 3};
    array<int, 3> arr3 = {4, 5, 6};
    EXPECT_TRUE(arr1 == arr2);
    EXPECT_FALSE(arr1 == arr3);
}

TEST(ArrayTest, InequalityOperator) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {4, 5, 6};
    EXPECT_TRUE(arr1 != arr2);
}

TEST(ArrayTest, LessThanOperator) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {1, 2, 4};
    EXPECT_TRUE(arr1 < arr2);
}

TEST(ArrayTest, GreaterThanOperator) {
    array<int, 3> arr1 = {1, 2, 4};
    array<int, 3> arr2 = {1, 2, 3};
    EXPECT_TRUE(arr1 > arr2);
}

TEST(ArrayTest, LessThanOrEqualOperator) {
    array<int, 3> arr1 = {1, 2, 3};
    array<int, 3> arr2 = {1, 2, 4};
    array<int, 3> arr3 = {1, 2, 3};
    EXPECT_TRUE(arr1 <= arr2);
    EXPECT_TRUE(arr1 <= arr3);
}

TEST(ArrayTest, GreaterThanOrEqualOperator) {
    array<int, 3> arr1 = {1, 2, 4};
    array<int, 3> arr2 = {1, 2, 3};
    array<int, 3> arr3 = {1, 2, 4};
    EXPECT_TRUE(arr1 >= arr2);
    EXPECT_TRUE(arr1 >= arr3);
}

TEST(ArrayTest, IteratorDereference) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    EXPECT_EQ(*it, 10);
}

TEST(ArrayTest, IteratorIncrement) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    ++it;
    EXPECT_EQ(*it, 20);
}

TEST(ArrayTest, IteratorDecrement) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.end();
    --it;
    EXPECT_EQ(*it, 30);
}

TEST(ArrayTest, IteratorAdvance) {
    array<int, 5> arr = {10, 20, 30, 40, 50};
    auto it = arr.begin();
    it += 3;
    EXPECT_EQ(*it, 40);
}

TEST(ArrayTest, IteratorDistance) {
    array<int, 5> arr = {10, 20, 30, 40, 50};
    auto it1 = arr.begin();
    auto it2 = arr.end();
    EXPECT_EQ(it2 - it1, 5);
}

TEST(ArrayTest, IteratorSubscript) {
    array<int, 5> arr = {10, 20, 30, 40, 50};
    auto it = arr.begin();
    EXPECT_EQ(it[2], 30);
    EXPECT_EQ(it[4], 50);
}

TEST(ArrayTest, IteratorEquality) {
    array<int, 3> arr = {10, 20, 30};
    auto it1 = arr.begin();
    auto it2 = arr.begin();
    auto it3 = arr.end();
    EXPECT_TRUE(it1 == it2);
    EXPECT_FALSE(it1 == it3);
}

TEST(ArrayTest, IteratorInequality) {
    array<int, 3> arr = {10, 20, 30};
    auto it1 = arr.begin();
    auto it2 = arr.end();
    EXPECT_TRUE(it1 != it2);
}

TEST(ArrayTest, IteratorLessThan) {
    array<int, 3> arr = {10, 20, 30};
    auto it1 = arr.begin();
    auto it2 = arr.end();
    EXPECT_TRUE(it1 < it2);
}

TEST(ArrayTest, IteratorGreaterThan) {
    array<int, 3> arr = {10, 20, 30};
    auto it1 = arr.end();
    auto it2 = arr.begin();
    EXPECT_TRUE(it1 > it2);
}

TEST(ArrayTest, IteratorBase) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    EXPECT_EQ(*(it.base()), 10);
}

TEST(ArrayTest, IteratorContainer) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    EXPECT_EQ(it.container(), &arr);
}

TEST(ArrayTest, ConstIteratorDereference) {
    const array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    EXPECT_EQ(*it, 10);
}

TEST(ArrayTest, ConstIteratorIncrement) {
    const array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    ++it;
    EXPECT_EQ(*it, 20);
}

TEST(ArrayTest, ConstIteratorDecrement) {
    const array<int, 3> arr = {10, 20, 30};
    auto it = arr.end();
    --it;
    EXPECT_EQ(*it, 30);
}

TEST(ArrayTest, ConstIteratorAdvance) {
    const array<int, 5> arr = {10, 20, 30, 40, 50};
    auto it = arr.begin();
    it += 3;
    EXPECT_EQ(*it, 40);
}

TEST(ArrayTest, ConstIteratorDistance) {
    const array<int, 5> arr = {10, 20, 30, 40, 50};
    auto it1 = arr.begin();
    auto it2 = arr.end();
    EXPECT_EQ(it2 - it1, 5);
}

TEST(ArrayTest, ConstIteratorSubscript) {
    const array<int, 5> arr = {10, 20, 30, 40, 50};
    auto it = arr.begin();
    EXPECT_EQ(it[2], 30);
    EXPECT_EQ(it[4], 50);
}

TEST(ArrayTest, RangeBasedForLoop) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    int sum = 0;
    for (auto& val: arr) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST(ArrayTest, ConstRangeBasedForLoop) {
    const array<int, 5> arr = {1, 2, 3, 4, 5};
    int sum = 0;
    for (const auto& val: arr) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST(ArrayTest, GetLvalue) {
    array<int, 3> arr = {10, 20, 30};
    EXPECT_EQ(get<0>(arr), 10);
    EXPECT_EQ(get<1>(arr), 20);
    EXPECT_EQ(get<2>(arr), 30);
    get<0>(arr) = 100;
    EXPECT_EQ(arr[0], 100);
}

TEST(ArrayTest, GetConstLvalue) {
    const array<int, 3> arr = {10, 20, 30};
    EXPECT_EQ(get<0>(arr), 10);
    EXPECT_EQ(get<1>(arr), 20);
    EXPECT_EQ(get<2>(arr), 30);
}

TEST(ArrayTest, GetRvalue) {
    int val = get<0>(array<int, 3>{10, 20, 30});
    EXPECT_EQ(val, 10);
}

TEST(ArrayTest, GetConstRvalue) {
    auto arr = []() -> const array<int, 3> { return {10, 20, 30}; };
    const int&& val = get<0>(move(arr()));
    EXPECT_EQ(val, 10);
}

TEST(ArrayTest, TupleSize) {
    using arr_type = array<int, 5>;
    EXPECT_EQ(tuple_size<arr_type>::value, 5);
    EXPECT_EQ(tuple_size_v<arr_type>, 5);
}

TEST(ArrayTest, TupleSizeConst) {
    using arr_type = const array<int, 5>;
    EXPECT_EQ(tuple_size<arr_type>::value, 5);
    EXPECT_EQ(tuple_size_v<arr_type>, 5);
}

TEST(ArrayTest, TupleElement) {
    using arr_type = array<int, 5>;
    EXPECT_TRUE((is_same_v<tuple_element_t<0, arr_type>, int>) );
    EXPECT_TRUE((is_same_v<tuple_element_t<4, arr_type>, int>) );
}

TEST(ArrayTest, StructuredBinding) {
    array<int, 3> arr = {1, 2, 3};
    auto& [a, b, c] = arr;
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(c, 3);
    a = 10;
    EXPECT_EQ(arr[0], 10);
}

TEST(ArrayTest, ConstStructuredBinding) {
    const array<int, 3> arr = {1, 2, 3};
    const auto& [a, b, c] = arr;
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(c, 3);
}

TEST(ArrayZeroTest, DefaultConstructor) {
    array<int, 0> arr;
    EXPECT_EQ(arr.size(), 0);
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.max_size(), 0);
}

TEST(ArrayZeroTest, IteratorDefaultConstructed) {
    using iterator = array<int, 0>::iterator;
    iterator it;
    EXPECT_EQ(it, iterator{});
}

TEST(ArrayZeroTest, ConstIteratorDefaultConstructed) {
    using const_iterator = array<int, 0>::const_iterator;
    const_iterator it;
    EXPECT_EQ(it, const_iterator{});
}

TEST(ArrayZeroTest, BeginEnd) {
    array<int, 0> arr;
    EXPECT_EQ(arr.begin(), arr.end());
}

TEST(ArrayZeroTest, ConstBeginEnd) {
    const array<int, 0> arr;
    EXPECT_EQ(arr.begin(), arr.end());
}

TEST(ArrayZeroTest, ReverseBeginEnd) {
    array<int, 0> arr;
    EXPECT_EQ(arr.rbegin(), arr.rend());
}

TEST(ArrayZeroTest, ConstReverseBeginEnd) {
    const array<int, 0> arr;
    EXPECT_EQ(arr.rbegin(), arr.rend());
}

TEST(ArrayZeroTest, CbeginCend) {
    array<int, 0> arr;
    EXPECT_EQ(arr.cbegin(), arr.cend());
}

TEST(ArrayZeroTest, CrbeginCrend) {
    array<int, 0> arr;
    EXPECT_EQ(arr.crbegin(), arr.crend());
}

TEST(ArrayZeroTest, Data) {
    array<int, 0> arr;
    EXPECT_EQ(arr.data(), nullptr);
}

TEST(ArrayZeroTest, ConstData) {
    const array<int, 0> arr;
    EXPECT_EQ(arr.data(), nullptr);
}

TEST(ArrayZeroTest, Fill) {
    array<int, 0> arr;
    arr.fill(42);
    SUCCEED();
}

TEST(ArrayZeroTest, Swap) {
    array<int, 0> arr1;
    array<int, 0> arr2;
    arr1.swap(arr2);
    SUCCEED();
}

TEST(ArrayZeroTest, EqualTo) {
    array<int, 0> arr1;
    array<int, 0> arr2;
    EXPECT_TRUE(arr1.equal_to(arr2));
}

TEST(ArrayZeroTest, LessThan) {
    array<int, 0> arr1;
    array<int, 0> arr2;
    EXPECT_FALSE(arr1.less_than(arr2));
}

TEST(ArrayZeroTest, EqualityOperator) {
    array<int, 0> arr1;
    array<int, 0> arr2;
    EXPECT_TRUE(arr1 == arr2);
}

TEST(ArrayZeroTest, NonDefaultConstructibleType) {
    struct NonDefault {
        NonDefault() = delete;
        explicit NonDefault(int) {}
    };
    array<NonDefault, 0> arr;
    EXPECT_EQ(arr.size(), 0);
}

TEST(ArrayTest, StringType) {
    array<string, 3> arr = {"hello", "world", "test"};
    EXPECT_EQ(arr[0], "hello");
    EXPECT_EQ(arr[1], "world");
    EXPECT_EQ(arr[2], "test");
    arr[0] = "modified";
    EXPECT_EQ(arr[0], "modified");
}

TEST(ArrayTest, StringCopy) {
    array<string, 2> arr1 = {"abc", "def"};
    array<string, 2> arr2 = arr1;
    EXPECT_EQ(arr2[0], "abc");
    EXPECT_EQ(arr2[1], "def");
}

TEST(ArrayTest, StringMove) {
    array<string, 2> arr1 = {"abc", "def"};
    array<string, 2> arr2 = move(arr1);
    EXPECT_EQ(arr2[0], "abc");
    EXPECT_EQ(arr2[1], "def");
}

TEST(ArrayTest, StringSwap) {
    array<string, 2> arr1 = {"abc", "def"};
    array<string, 2> arr2 = {"ghi", "jkl"};
    arr1.swap(arr2);
    EXPECT_EQ(arr1[0], "ghi");
    EXPECT_EQ(arr1[1], "jkl");
    EXPECT_EQ(arr2[0], "abc");
    EXPECT_EQ(arr2[1], "def");
}

TEST(ArrayTest, StringCompare) {
    array<string, 2> arr1 = {"abc", "def"};
    array<string, 2> arr2 = {"abc", "def"};
    array<string, 2> arr3 = {"abc", "deg"};
    EXPECT_TRUE(arr1.equal_to(arr2));
    EXPECT_TRUE(arr1.less_than(arr3));
}

TEST(ArrayTest, LargeArray) {
    constexpr size_t large_size = 1000;
    array<int, large_size> arr;
    arr.fill(7);
    for (size_t i = 0; i < large_size; ++i) {
        EXPECT_EQ(arr[i], 7);
    }
    EXPECT_EQ(arr.size(), large_size);
}

TEST(ArrayTest, IteratorPostIncrement) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.begin();
    auto old = it++;
    EXPECT_EQ(*old, 10);
    EXPECT_EQ(*it, 20);
}

TEST(ArrayTest, IteratorPostDecrement) {
    array<int, 3> arr = {10, 20, 30};
    auto it = arr.end();
    auto old = it--;
    EXPECT_EQ(old, arr.end());
    EXPECT_EQ(*it, 30);
}

TEST(ArrayTest, IteratorPlusEqual) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.begin();
    it += 2;
    EXPECT_EQ(*it, 3);
}

TEST(ArrayTest, IteratorMinusEqual) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.end();
    it -= 2;
    EXPECT_EQ(*it, 4);
}

TEST(ArrayTest, IteratorPlus) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.begin() + 3;
    EXPECT_EQ(*it, 4);
}

TEST(ArrayTest, IteratorMinus) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = arr.end() - 2;
    EXPECT_EQ(*it, 4);
}

TEST(ArrayTest, StdAlgorithmsCompatibility) {
    array<int, 5> arr = {5, 3, 4, 1, 2};
    sort(arr.begin(), arr.end());
    for (size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(arr[i], static_cast<int>(i + 1));
    }
}

TEST(ArrayTest, StdFindCompatibility) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    auto it = find(arr.begin(), arr.end(), 3);
    EXPECT_NE(it, arr.end());
    EXPECT_EQ(*it, 3);
}

TEST(ArrayTest, StdAccumulateCompatibility) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    int sum = accumulate(arr.begin(), arr.end(), 0);
    EXPECT_EQ(sum, 15);
}

TEST(ArrayTest, SingleElement) {
    array<int, 1> arr = {42};
    EXPECT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0], 42);
    EXPECT_EQ(arr.front(), 42);
    EXPECT_EQ(arr.back(), 42);
    EXPECT_EQ(*arr.begin(), 42);
}

#ifdef NEFORCE_STATE_DEBUG
TEST(ArrayDeathTest, OutOfBoundsAt) {
    array<int, 3> arr = {1, 2, 3};
    EXPECT_DEBUG_DEATH(ignore = arr.at(3), "");
}

TEST(ArrayDeathTest, OutOfBoundsSubscript) {
    array<int, 3> arr = {1, 2, 3};
    EXPECT_DEBUG_DEATH(ignore = arr[3], "");
}

TEST(ArrayDeathTest, IteratorOutOfBoundsDereference) {
    array<int, 3> arr = {1, 2, 3};
    auto it = arr.end();
    EXPECT_DEBUG_DEATH(ignore = *it, "");
}

TEST(ArrayDeathTest, IteratorOutOfBoundsIncrement) {
    array<int, 3> arr = {1, 2, 3};
    auto it = arr.end();
    EXPECT_DEBUG_DEATH(ignore = ++it, "");
}

TEST(ArrayDeathTest, IteratorOutOfBoundsDecrement) {
    array<int, 3> arr = {1, 2, 3};
    auto it = arr.begin();
    EXPECT_DEBUG_DEATH(ignore = --it, "");
}

TEST(ArrayDeathTest, IteratorOutOfBoundsAdvance) {
    array<int, 3> arr = {1, 2, 3};
    auto it = arr.begin();
    EXPECT_DEBUG_DEATH(it += 4, "");
}
#endif

TEST(BitReferenceTest, DefaultConstructor) {
    bit_reference ref;
    EXPECT_EQ(ref.to_string(), "0"_s);
}

TEST(BitReferenceTest, ConstructorWithPtrAndMask) {
    uint32_t word = 0;
    bit_reference ref(&word, 1U << 2);
    EXPECT_FALSE(static_cast<bool>(ref));
}

TEST(BitReferenceTest, CopyConstructor) {
    uint32_t word = 0b00000100;
    bit_reference ref1(&word, 1U << 2);
    bit_reference ref2(ref1);
    EXPECT_TRUE(static_cast<bool>(ref2));
}

TEST(BitReferenceTest, CopyAssignment) {
    uint32_t word1 = 0b00000100;
    uint32_t word2 = 0;
    bit_reference ref1(&word1, 1U << 2);
    bit_reference ref2(&word2, 1U << 3);
    ref2 = ref1;
    EXPECT_TRUE(static_cast<bool>(ref2));
}

TEST(BitReferenceTest, MoveConstructor) {
    uint32_t word = 0b00000100;
    bit_reference ref1(&word, 1U << 2);
    bit_reference ref2(move(ref1));
    EXPECT_TRUE(static_cast<bool>(ref2));
}

TEST(BitReferenceTest, MoveAssignment) {
    uint32_t word1 = 0b00000100;
    uint32_t word2 = 0;
    bit_reference ref1(&word1, 1U << 2);
    bit_reference ref2(&word2, 1U << 3);
    ref2 = move(ref1);
    EXPECT_TRUE(static_cast<bool>(ref2));
}

TEST(BitReferenceTest, AssignBoolTrue) {
    uint32_t word = 0;
    bit_reference ref(&word, 1U << 5);
    ref = true;
    EXPECT_TRUE(static_cast<bool>(ref));
    EXPECT_EQ(word, 1U << 5);
}

TEST(BitReferenceTest, AssignBoolFalse) {
    uint32_t word = 0b11111111;
    bit_reference ref(&word, 1U << 3);
    ref = false;
    EXPECT_FALSE(static_cast<bool>(ref));
    EXPECT_EQ(word & (1U << 3), 0U);
}

TEST(BitReferenceTest, ExplicitBoolConversionTrue) {
    uint32_t word = 0b00001000;
    bit_reference ref(&word, 1U << 3);
    EXPECT_TRUE(static_cast<bool>(ref));
}

TEST(BitReferenceTest, ExplicitBoolConversionFalse) {
    uint32_t word = 0;
    bit_reference ref(&word, 1U << 3);
    EXPECT_FALSE(static_cast<bool>(ref));
}

TEST(BitReferenceTest, Flip) {
    uint32_t word = 0;
    bit_reference ref(&word, 1U << 1);
    EXPECT_FALSE(static_cast<bool>(ref));
    ref.flip();
    EXPECT_TRUE(static_cast<bool>(ref));
    ref.flip();
    EXPECT_FALSE(static_cast<bool>(ref));
}

TEST(BitReferenceTest, Swap) {
    uint32_t word1 = 0b00000001;
    uint32_t word2 = 0b00000000;
    bit_reference ref1(&word1, 1U << 0);
    bit_reference ref2(&word2, 1U << 1);
    ref1.swap(ref2);
    EXPECT_FALSE(static_cast<bool>(ref1));
    EXPECT_TRUE(static_cast<bool>(ref2));
}

TEST(BitReferenceTest, SwapSelf) {
    uint32_t word = 0b00000001;
    bit_reference ref(&word, 1U << 0);
    ref.swap(ref);
    EXPECT_TRUE(static_cast<bool>(ref));
}

TEST(BitReferenceTest, EqualTo) {
    uint32_t word1 = 0b00000100;
    uint32_t word2 = 0b00000100;
    bit_reference ref1(&word1, 1U << 2);
    bit_reference ref2(&word2, 1U << 2);
    EXPECT_TRUE(ref1.equal_to(ref2));
}

TEST(BitReferenceTest, NotEqualTo) {
    uint32_t word1 = 0b00000100;
    uint32_t word2 = 0;
    bit_reference ref1(&word1, 1U << 2);
    bit_reference ref2(&word2, 1U << 2);
    EXPECT_FALSE(ref1.equal_to(ref2));
}

TEST(BitReferenceTest, LessThan) {
    uint32_t word1 = 0;
    uint32_t word2 = 0b00000001;
    bit_reference ref1(&word1, 1U << 0);
    bit_reference ref2(&word2, 1U << 0);
    EXPECT_TRUE(ref1.less_than(ref2));
    EXPECT_FALSE(ref2.less_than(ref1));
}

TEST(BitReferenceTest, ToHash) {
    uint32_t word = 0b00000001;
    bit_reference ref(&word, 1U << 0);
    size_t h = ref.to_hash();
    EXPECT_NE(h, 0);
}

TEST(BitReferenceTest, ToStringTrue) {
    uint32_t word = 0b00000010;
    bit_reference ref(&word, 1U << 1);
    EXPECT_EQ(ref.to_string(), "1"_s);
}

TEST(BitReferenceTest, ToStringFalse) {
    uint32_t word = 0;
    bit_reference ref(&word, 1U << 1);
    EXPECT_EQ(ref.to_string(), "0"_s);
}

TEST(BitReferenceTest, EqualityOperator) {
    uint32_t word1 = 0b00000100;
    uint32_t word2 = 0b00000100;
    bit_reference ref1(&word1, 1U << 2);
    bit_reference ref2(&word2, 1U << 2);
    EXPECT_TRUE(ref1 == ref2);
}

TEST(BitReferenceTest, InequalityOperator) {
    uint32_t word1 = 0b00000100;
    uint32_t word2 = 0;
    bit_reference ref1(&word1, 1U << 2);
    bit_reference ref2(&word2, 1U << 2);
    EXPECT_TRUE(ref1 != ref2);
}

TEST(BitmapTest, DefaultConstructor) {
    bitmap bm;
    EXPECT_TRUE(bm.empty());
    EXPECT_EQ(bm.size(), 0);
}

TEST(BitmapTest, ConstructorWithSize) {
    bitmap bm(100);
    EXPECT_EQ(bm.size(), 100);
    EXPECT_FALSE(bm.empty());
}

TEST(BitmapTest, ConstructorWithSizeAndValueTrue) {
    bitmap bm(50, true);
    EXPECT_EQ(bm.size(), 50);
    for (size_t i = 0; i < bm.size(); ++i) {
        EXPECT_TRUE(bm[i]);
    }
}

TEST(BitmapTest, ConstructorWithSizeAndValueFalse) {
    bitmap bm(50, false);
    EXPECT_EQ(bm.size(), 50);
    for (size_t i = 0; i < bm.size(); ++i) {
        EXPECT_FALSE(bm[i]);
    }
}

TEST(BitmapTest, ConstructorWithInt32) {
    bitmap bm(static_cast<int32_t>(10), true);
    EXPECT_EQ(bm.size(), 10);
    EXPECT_TRUE(bm[0]);
}

TEST(BitmapTest, ConstructorWithInt64) {
    bitmap bm(static_cast<int64_t>(20), false);
    EXPECT_EQ(bm.size(), 20);
    EXPECT_FALSE(bm[0]);
}

TEST(BitmapTest, ConstructorWithZeroSize) {
    bitmap bm(0);
    EXPECT_TRUE(bm.empty());
    EXPECT_EQ(bm.size(), 0);
}

TEST(BitmapTest, CopyConstructor) {
    bitmap bm1(10, true);
    bm1[3] = false;
    bitmap bm2(bm1);
    EXPECT_EQ(bm2.size(), 10);
    EXPECT_TRUE(bm2[0]);
    EXPECT_FALSE(bm2[3]);
}

TEST(BitmapTest, CopyAssignment) {
    bitmap bm1(10, true);
    bm1[5] = false;
    bitmap bm2;
    bm2 = bm1;
    EXPECT_EQ(bm2.size(), 10);
    EXPECT_TRUE(bm2[0]);
    EXPECT_FALSE(bm2[5]);
}

TEST(BitmapTest, CopyAssignmentSelf) {
    bitmap bm(10, true);
    bm = bm;
    EXPECT_EQ(bm.size(), 10);
    EXPECT_TRUE(bm[0]);
}

TEST(BitmapTest, MoveConstructor) {
    bitmap bm1(10, true);
    bitmap bm2(move(bm1));
    EXPECT_EQ(bm2.size(), 10);
    EXPECT_TRUE(bm2[0]);
}

TEST(BitmapTest, MoveAssignment) {
    bitmap bm1(10, true);
    bitmap bm2;
    bm2 = move(bm1);
    EXPECT_EQ(bm2.size(), 10);
    EXPECT_TRUE(bm2[0]);
}

TEST(BitmapTest, MoveAssignmentSelf) {
    bitmap bm(10, true);
    bm = move(bm);
    EXPECT_EQ(bm.size(), 10);
}

TEST(BitmapTest, RangeConstructorWithVector) {
    vector<bool> vec = {true, false, true, true, false};
    bitmap bm(vec.begin(), vec.end());
    EXPECT_EQ(bm.size(), 5);
    EXPECT_TRUE(bm[0]);
    EXPECT_FALSE(bm[1]);
    EXPECT_TRUE(bm[2]);
    EXPECT_TRUE(bm[3]);
    EXPECT_FALSE(bm[4]);
}

TEST(BitmapTest, RangeConstructorWithList) {
    list<bool> lst = {false, false, true, false};
    bitmap bm(lst.begin(), lst.end());
    EXPECT_EQ(bm.size(), 4);
    EXPECT_FALSE(bm[0]);
    EXPECT_FALSE(bm[1]);
    EXPECT_TRUE(bm[2]);
    EXPECT_FALSE(bm[3]);
}

TEST(BitmapTest, RangeConstructorEmpty) {
    vector<bool> vec;
    bitmap bm(vec.begin(), vec.end());
    EXPECT_TRUE(bm.empty());
    EXPECT_EQ(bm.size(), 0);
}

TEST(BitmapTest, BeginEnd) {
    bitmap bm(5);
    auto it = bm.begin();
    auto end_it = bm.end();
    EXPECT_EQ(end_it - it, 5);
}

TEST(BitmapTest, ConstBeginEnd) {
    const bitmap bm(5, true);
    auto it = bm.begin();
    auto end_it = bm.end();
    EXPECT_EQ(end_it - it, 5);
}

TEST(BitmapTest, CbeginCend) {
    bitmap bm(5);
    auto it = bm.cbegin();
    auto end_it = bm.cend();
    EXPECT_EQ(end_it - it, 5);
}

TEST(BitmapTest, ReverseBeginEnd) {
    bitmap bm(3);
    bm[0] = true;
    bm[1] = false;
    bm[2] = true;
    auto rit = bm.rbegin();
    EXPECT_TRUE(*rit);
    ++rit;
    EXPECT_FALSE(*rit);
    ++rit;
    EXPECT_TRUE(*rit);
}

TEST(BitmapTest, ConstReverseBeginEnd) {
    const bitmap bm(3, true);
    auto rit = bm.rbegin();
    EXPECT_TRUE(*rit);
}

TEST(BitmapTest, CrbeginCrend) {
    bitmap bm(3);
    bm[0] = true;
    auto rit = bm.crbegin();
    EXPECT_FALSE(*rit);
    ++rit;
    EXPECT_FALSE(*rit);
    ++rit;
    EXPECT_TRUE(*rit);
}

TEST(BitmapTest, Size) {
    bitmap bm(42);
    EXPECT_EQ(bm.size(), 42);
}

TEST(BitmapTest, MaxSize) {
    bitmap bm;
    EXPECT_GT(bm.max_size(), 0);
}

TEST(BitmapTest, Empty) {
    bitmap bm;
    EXPECT_TRUE(bm.empty());
    bm.push_back(true);
    EXPECT_FALSE(bm.empty());
}

TEST(BitmapTest, Capacity) {
    bitmap bm;
    EXPECT_EQ(bm.capacity(), 0);
    bm.reserve(100);
    EXPECT_GE(bm.capacity(), 100);
}

TEST(BitmapTest, SubscriptOperator) {
    bitmap bm(10);
    bm[3] = true;
    EXPECT_TRUE(bm[3]);
    EXPECT_FALSE(bm[4]);
    bm[3] = false;
    EXPECT_FALSE(bm[3]);
}

TEST(BitmapTest, ConstSubscriptOperator) {
    const bitmap bm(10, true);
    EXPECT_TRUE(bm[0]);
    EXPECT_TRUE(bm[9]);
}

TEST(BitmapTest, Front) {
    bitmap bm(5);
    bm[0] = true;
    EXPECT_TRUE(bm.front());
    bm.front() = false;
    EXPECT_FALSE(bm.front());
}

TEST(BitmapTest, ConstFront) {
    const bitmap bm(5, true);
    EXPECT_TRUE(bm.front());
}

TEST(BitmapTest, Back) {
    bitmap bm(5);
    bm[4] = true;
    EXPECT_TRUE(bm.back());
    bm.back() = false;
    EXPECT_FALSE(bm.back());
}

TEST(BitmapTest, ConstBack) {
    const bitmap bm(5, true);
    EXPECT_TRUE(bm.back());
}

TEST(BitmapTest, Reserve) {
    bitmap bm;
    bm.reserve(200);
    EXPECT_GE(bm.capacity(), 200);
    EXPECT_EQ(bm.size(), 0);
}

TEST(BitmapTest, ReserveSmallerThanCapacity) {
    bitmap bm(100);
    size_t cap = bm.capacity();
    bm.reserve(50);
    EXPECT_EQ(bm.capacity(), cap);
}

TEST(BitmapTest, PushBack) {
    bitmap bm;
    bm.push_back(true);
    bm.push_back(false);
    bm.push_back(true);
    EXPECT_EQ(bm.size(), 3);
    EXPECT_TRUE(bm[0]);
    EXPECT_FALSE(bm[1]);
    EXPECT_TRUE(bm[2]);
}

TEST(BitmapTest, PushBackMany) {
    bitmap bm;
    for (int i = 0; i < 1000; ++i) {
        bm.push_back(i % 2 == 0);
    }
    EXPECT_EQ(bm.size(), 1000);
    EXPECT_TRUE(bm[0]);
    EXPECT_FALSE(bm[1]);
}

TEST(BitmapTest, InsertSingleElementAtBegin) {
    bitmap bm(3, false);
    auto it = bm.insert(bm.begin(), true);
    EXPECT_EQ(bm.size(), 4);
    EXPECT_TRUE(bm[0]);
    EXPECT_FALSE(bm[1]);
    EXPECT_EQ(it, bm.begin());
}

TEST(BitmapTest, InsertSingleElementAtEnd) {
    bitmap bm(3, false);
    auto it = bm.insert(bm.end(), true);
    EXPECT_EQ(bm.size(), 4);
    EXPECT_FALSE(bm[2]);
    EXPECT_TRUE(bm[3]);
    EXPECT_EQ(static_cast<bool>(*it), true);
}

TEST(BitmapTest, InsertSingleElementAtMiddle) {
    bitmap bm(5, false);
    auto it = bm.insert(bm.begin() + 2, true);
    EXPECT_EQ(bm.size(), 6);
    EXPECT_TRUE(bm[2]);
    EXPECT_EQ(static_cast<bool>(*it), true);
}

TEST(BitmapTest, InsertRange) {
    bitmap bm(3, false);
    vector<bool> vals = {true, true, false};
    bm.insert(bm.begin() + 1, vals.begin(), vals.end());
    EXPECT_EQ(bm.size(), 6);
    EXPECT_FALSE(bm[0]);
    EXPECT_TRUE(bm[1]);
    EXPECT_TRUE(bm[2]);
    EXPECT_FALSE(bm[3]);
    EXPECT_FALSE(bm[4]);
    EXPECT_FALSE(bm[5]);
}

TEST(BitmapTest, InsertBoolArray) {
    bitmap bm(2, false);
    bool arr[] = {true, false, true};
    bm.insert(bm.begin() + 1, arr, arr + 3);
    EXPECT_EQ(bm.size(), 5);
    EXPECT_FALSE(bm[0]);
    EXPECT_TRUE(bm[1]);
    EXPECT_FALSE(bm[2]);
    EXPECT_TRUE(bm[3]);
    EXPECT_FALSE(bm[4]);
}

TEST(BitmapTest, InsertNBools) {
    bitmap bm(2, false);
    bm.insert(bm.begin(), 3, true);
    EXPECT_EQ(bm.size(), 5);
    EXPECT_TRUE(bm[0]);
    EXPECT_TRUE(bm[1]);
    EXPECT_TRUE(bm[2]);
    EXPECT_FALSE(bm[3]);
    EXPECT_FALSE(bm[4]);
}

TEST(BitmapTest, InsertNInt32) {
    bitmap bm(2, false);
    bm.insert(bm.begin(), static_cast<int32_t>(2), true);
    EXPECT_EQ(bm.size(), 4);
    EXPECT_TRUE(bm[0]);
    EXPECT_TRUE(bm[1]);
}

TEST(BitmapTest, InsertNInt64) {
    bitmap bm(2, false);
    bm.insert(bm.begin(), static_cast<int64_t>(2), true);
    EXPECT_EQ(bm.size(), 4);
    EXPECT_TRUE(bm[0]);
    EXPECT_TRUE(bm[1]);
}

TEST(BitmapTest, InsertZeroElements) {
    bitmap bm(3, false);
    bm.insert(bm.begin(), 0, true);
    EXPECT_EQ(bm.size(), 3);
}

TEST(BitmapTest, PopBack) {
    bitmap bm(5, true);
    bm.pop_back();
    EXPECT_EQ(bm.size(), 4);
}

TEST(BitmapTest, EraseSingleElement) {
    bitmap bm(5);
    bm[0] = true;
    bm[1] = false;
    bm[2] = true;
    bm[3] = false;
    bm[4] = true;
    auto it = bm.erase(bm.begin() + 2);
    EXPECT_EQ(bm.size(), 4);
    EXPECT_TRUE(bm[0]);
    EXPECT_FALSE(bm[1]);
    EXPECT_FALSE(bm[2]);
    EXPECT_TRUE(bm[3]);
    EXPECT_EQ(it, bm.begin() + 2);
}

TEST(BitmapTest, EraseRange) {
    bitmap bm(5);
    bm[0] = true;
    bm[1] = false;
    bm[2] = true;
    bm[3] = false;
    bm[4] = true;
    auto it = bm.erase(bm.begin() + 1, bm.begin() + 4);
    EXPECT_EQ(bm.size(), 2);
    EXPECT_TRUE(bm[0]);
    EXPECT_TRUE(bm[1]);
    EXPECT_EQ(it, bm.begin() + 1);
}

TEST(BitmapTest, ResizeSmaller) {
    bitmap bm(10, true);
    bm.resize(5);
    EXPECT_EQ(bm.size(), 5);
}

TEST(BitmapTest, ResizeLargerWithDefault) {
    bitmap bm(5, true);
    bm.resize(10);
    EXPECT_EQ(bm.size(), 10);
    EXPECT_TRUE(bm[0]);
    EXPECT_FALSE(bm[9]);
}

TEST(BitmapTest, ResizeLargerWithTrue) {
    bitmap bm(5, false);
    bm.resize(10, true);
    EXPECT_EQ(bm.size(), 10);
    EXPECT_FALSE(bm[0]);
    EXPECT_TRUE(bm[9]);
}

TEST(BitmapTest, Clear) {
    bitmap bm(10, true);
    bm.clear();
    EXPECT_TRUE(bm.empty());
    EXPECT_EQ(bm.size(), 0);
}

TEST(BitmapTest, Swap) {
    bitmap bm1(5, true);
    bitmap bm2(3, false);
    bm1.swap(bm2);
    EXPECT_EQ(bm1.size(), 3);
    EXPECT_FALSE(bm1[0]);
    EXPECT_EQ(bm2.size(), 5);
    EXPECT_TRUE(bm2[0]);
}

TEST(BitmapTest, SwapSelf) {
    bitmap bm(5, true);
    bm.swap(bm);
    EXPECT_EQ(bm.size(), 5);
    EXPECT_TRUE(bm[0]);
}

TEST(BitmapTest, EqualTo) {
    bitmap bm1(5, true);
    bitmap bm2(5, true);
    bm1[2] = false;
    bm2[2] = false;
    EXPECT_TRUE(bm1.equal_to(bm2));
}

TEST(BitmapTest, NotEqualToDifferentSize) {
    bitmap bm1(5, true);
    bitmap bm2(6, true);
    EXPECT_FALSE(bm1.equal_to(bm2));
}

TEST(BitmapTest, NotEqualToDifferentValues) {
    bitmap bm1(5, true);
    bitmap bm2(5, true);
    bm2[3] = false;
    EXPECT_FALSE(bm1.equal_to(bm2));
}

TEST(BitmapTest, LessThan) {
    bitmap bm1(3);
    bm1[0] = false;
    bm1[1] = true;
    bm1[2] = false;
    bitmap bm2(3);
    bm2[0] = false;
    bm2[1] = true;
    bm2[2] = true;
    EXPECT_TRUE(bm1.less_than(bm2));
    EXPECT_FALSE(bm2.less_than(bm1));
}

TEST(BitmapTest, LessThanDifferentSize) {
    bitmap bm1(3, true);
    bitmap bm2(4, true);
    EXPECT_TRUE(bm1.less_than(bm2));
}

TEST(BitmapTest, EqualityOperator) {
    bitmap bm1(3, true);
    bitmap bm2(3, true);
    EXPECT_TRUE(bm1 == bm2);
}

TEST(BitmapTest, InequalityOperator) {
    bitmap bm1(3, true);
    bitmap bm2(3, false);
    EXPECT_TRUE(bm1 != bm2);
}

TEST(BitmapTest, LessThanOperator) {
    bitmap bm1(2, false);
    bitmap bm2(2, true);
    EXPECT_TRUE(bm1 < bm2);
}

TEST(BitmapTest, GreaterThanOperator) {
    bitmap bm1(2, true);
    bitmap bm2(2, false);
    EXPECT_TRUE(bm1 > bm2);
}

TEST(BitmapTest, LessThanOrEqualOperator) {
    bitmap bm1(2, false);
    bitmap bm2(2, false);
    bitmap bm3(2, true);
    EXPECT_TRUE(bm1 <= bm2);
    EXPECT_TRUE(bm1 <= bm3);
}

TEST(BitmapTest, GreaterThanOrEqualOperator) {
    bitmap bm1(2, true);
    bitmap bm2(2, true);
    bitmap bm3(2, false);
    EXPECT_TRUE(bm1 >= bm2);
    EXPECT_TRUE(bm1 >= bm3);
}

TEST(BitmapIteratorTest, DefaultConstructor) {
    bitmap::iterator it;
    bitmap::iterator it2;
    EXPECT_TRUE(it == it2);
}

TEST(BitmapIteratorTest, Dereference) {
    bitmap bm(5);
    bm[2] = true;
    auto it = bm.begin();
    EXPECT_FALSE(*it);
    ++it;
    ++it;
    EXPECT_TRUE(*it);
}

TEST(BitmapIteratorTest, Increment) {
    bitmap bm(3);
    bm[0] = false;
    bm[1] = true;
    bm[2] = false;
    auto it = bm.begin();
    ++it;
    EXPECT_TRUE(*it);
}

TEST(BitmapIteratorTest, Decrement) {
    bitmap bm(3);
    bm[2] = true;
    auto it = bm.end();
    --it;
    EXPECT_TRUE(*it);
}

TEST(BitmapIteratorTest, AdvancePositive) {
    bitmap bm(100);
    bm[50] = true;
    auto it = bm.begin();
    it += 50;
    EXPECT_TRUE(*it);
}

TEST(BitmapIteratorTest, AdvanceNegative) {
    bitmap bm(100);
    bm[50] = true;
    auto it = bm.end();
    it -= 50;
    EXPECT_TRUE(static_cast<bool>(*it));
}

TEST(BitmapIteratorTest, DistanceTo) {
    bitmap bm(100);
    auto it1 = bm.begin();
    auto it2 = bm.end();
    EXPECT_EQ(it2 - it1, 100);
}

TEST(BitmapIteratorTest, Subscript) {
    bitmap bm(50);
    bm[30] = true;
    auto it = bm.begin();
    EXPECT_TRUE(it[30]);
}

TEST(BitmapIteratorTest, Equality) {
    bitmap bm(10);
    auto it1 = bm.begin();
    auto it2 = bm.begin();
    auto it3 = bm.end();
    EXPECT_TRUE(it1 == it2);
    EXPECT_FALSE(it1 == it3);
}

TEST(BitmapIteratorTest, Inequality) {
    bitmap bm(10);
    auto it1 = bm.begin();
    auto it2 = bm.end();
    EXPECT_TRUE(it1 != it2);
}

TEST(BitmapIteratorTest, LessThan) {
    bitmap bm(10);
    auto it1 = bm.begin();
    auto it2 = bm.end();
    EXPECT_TRUE(it1 < it2);
}

TEST(BitmapIteratorTest, GreaterThan) {
    bitmap bm(10);
    auto it1 = bm.end();
    auto it2 = bm.begin();
    EXPECT_TRUE(it1 > it2);
}

TEST(BitmapIteratorTest, PostIncrement) {
    bitmap bm(3);
    bm[1] = true;
    auto it = bm.begin();
    auto old = it++;
    EXPECT_FALSE(*old);
    EXPECT_TRUE(*it);
}

TEST(BitmapIteratorTest, PostDecrement) {
    bitmap bm(3);
    bm[2] = true;
    auto it = bm.end();
    auto old = it--;
    EXPECT_EQ(old, bm.end());
    EXPECT_TRUE(*it);
}

TEST(BitmapIteratorTest, Base) {
    bitmap bm(50);
    auto it = bm.begin();
    EXPECT_NE(it.base(), nullptr);
}

TEST(BitmapIteratorTest, Container) {
    bitmap bm;
    auto it = bm.begin();
    EXPECT_EQ(it.container(), &bm);
}

TEST(BitmapIteratorTest, RangeBasedForLoop) {
    bitmap bm(10);
    bm[2] = true;
    bm[5] = true;
    bm[8] = true;
    int count = 0;
    for (auto val: bm) {
        if (val) {
            ++count;
        }
    }
    EXPECT_EQ(count, 3);
}

TEST(BitmapTest, LargeBitmap) {
    const size_t large_size = 10000;
    bitmap bm(large_size, true);
    EXPECT_EQ(bm.size(), large_size);
    bm[5000] = false;
    EXPECT_FALSE(bm[5000]);
    EXPECT_TRUE(bm[0]);
    EXPECT_TRUE(bm[large_size - 1]);
}

TEST(BitmapTest, WordBoundary) {
    bitmap bm(128);
    bm[31] = true;
    bm[32] = true;
    bm[63] = true;
    bm[64] = true;
    EXPECT_TRUE(bm[31]);
    EXPECT_TRUE(bm[32]);
    EXPECT_TRUE(bm[63]);
    EXPECT_TRUE(bm[64]);
    EXPECT_FALSE(bm[30]);
    EXPECT_FALSE(bm[33]);
    EXPECT_FALSE(bm[62]);
    EXPECT_FALSE(bm[65]);
}

TEST(BitmapTest, ConstIteratorDereference) {
    const bitmap bm(5, true);
    auto it = bm.cbegin();
    EXPECT_TRUE(*it);
}

TEST(BitmapTest, ConstIteratorTraversal) {
    const bitmap bm(5);
    int count = 0;
    for (auto it = bm.cbegin(); it != bm.cend(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 5);
}

TEST(BitmapTest, ConstReverseIteratorTraversal) {
    const bitmap bm(5, true);
    int count = 0;
    for (auto it = bm.crbegin(); it != bm.crend(); ++it) {
        EXPECT_TRUE(*it);
        ++count;
    }
    EXPECT_EQ(count, 5);
}

TEST(BitmapTest, InsertReallocation) {
    bitmap bm;
    for (int i = 0; i < 100; ++i) {
        bm.push_back(i % 2 == 0);
    }
    bm.insert(bm.begin() + 33, 50, true);
    EXPECT_EQ(bm.size(), 150);
}

TEST(BitmapTest, RangeInsertReallocation) {
    bitmap bm(10, false);
    vector<bool> vals(100, true);
    bm.insert(bm.begin() + 5, vals.begin(), vals.end());
    EXPECT_EQ(bm.size(), 110);
    EXPECT_FALSE(bm[4]);
    EXPECT_TRUE(bm[5]);
    EXPECT_TRUE(bm[104]);
    EXPECT_FALSE(bm[105]);
}

TEST(BitmapTest, CopyEmptyBitmap) {
    bitmap bm1;
    bitmap bm2(bm1);
    EXPECT_TRUE(bm2.empty());
}

TEST(BitmapTest, AssignEmptyBitmap) {
    bitmap bm1(10, true);
    bitmap bm2;
    bm1 = bm2;
    EXPECT_TRUE(bm1.empty());
}

TEST(BitmapTest, MoveEmptyBitmap) {
    bitmap bm1;
    bitmap bm2(move(bm1));
    EXPECT_TRUE(bm2.empty());
}

TEST(BitsetTest, DefaultConstructor) {
    bitset<8> bs;
    EXPECT_EQ(bs.size(), 8);
    EXPECT_FALSE(bs.empty());
    EXPECT_TRUE(bs.none());
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, DefaultConstructorZeroSize) {
    bitset<0> bs;
    EXPECT_EQ(bs.size(), 0);
    EXPECT_TRUE(bs.empty());
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, IntegerConstructor) {
    bitset<8> bs(0b10100101u);
    EXPECT_TRUE(bs.test(0));
    EXPECT_FALSE(bs.test(1));
    EXPECT_TRUE(bs.test(2));
    EXPECT_FALSE(bs.test(3));
    EXPECT_FALSE(bs.test(4));
    EXPECT_TRUE(bs.test(5));
    EXPECT_FALSE(bs.test(6));
    EXPECT_TRUE(bs.test(7));
}

TEST(BitsetTest, IntegerConstructorLarge) {
    bitset<64> bs(0xDEADBEEFCAFEBABEuLL);
    EXPECT_EQ(bs.to_ullong(), 0xDEADBEEFCAFEBABEuLL);
}

TEST(BitsetTest, IntegerConstructorTruncation) {
    bitset<4> bs(0xFFu);
    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(1));
    EXPECT_TRUE(bs.test(2));
    EXPECT_TRUE(bs.test(3));
}

TEST(BitsetTest, StringConstructor) {
    bitset<8> bs("10100101"_s);
    EXPECT_TRUE(bs.test(7));
    EXPECT_FALSE(bs.test(6));
    EXPECT_TRUE(bs.test(5));
    EXPECT_FALSE(bs.test(4));
    EXPECT_FALSE(bs.test(3));
    EXPECT_TRUE(bs.test(2));
    EXPECT_FALSE(bs.test(1));
    EXPECT_TRUE(bs.test(0));
}

TEST(BitsetTest, StringConstructorShort) {
    bitset<8> bs("101"_s);
    EXPECT_TRUE(bs.test(2));
    EXPECT_FALSE(bs.test(1));
    EXPECT_TRUE(bs.test(0));
    EXPECT_FALSE(bs.test(3));
    EXPECT_FALSE(bs.test(7));
}

TEST(BitsetTest, StringConstructorLong) {
    bitset<4> bs("10101010"_s);
    EXPECT_TRUE(bs.test(3));
    EXPECT_FALSE(bs.test(2));
    EXPECT_TRUE(bs.test(1));
    EXPECT_FALSE(bs.test(0));
}

TEST(BitsetTest, StringConstructorCustomChars) {
    bitset<8> bs("baabbaab"_sv, 'a', 'b');
    EXPECT_TRUE(bs.test(7));
    EXPECT_FALSE(bs.test(6));
    EXPECT_FALSE(bs.test(5));
    EXPECT_TRUE(bs.test(4));
    EXPECT_TRUE(bs.test(3));
    EXPECT_FALSE(bs.test(2));
    EXPECT_FALSE(bs.test(1));
    EXPECT_TRUE(bs.test(0));
}

TEST(BitsetTest, StringConstructorExact) {
    bitset<8> bs(string_view{"11110000"});
    EXPECT_TRUE(bs.test(7));
    EXPECT_TRUE(bs.test(6));
    EXPECT_TRUE(bs.test(5));
    EXPECT_TRUE(bs.test(4));
    EXPECT_FALSE(bs.test(3));
    EXPECT_FALSE(bs.test(2));
    EXPECT_FALSE(bs.test(1));
    EXPECT_FALSE(bs.test(0));
}

TEST(BitsetTest, CStringConstructor) {
    bitset<8> bs("10101010");
    EXPECT_TRUE(bs.test(7));
    EXPECT_FALSE(bs.test(6));
    EXPECT_TRUE(bs.test(5));
    EXPECT_FALSE(bs.test(4));
    EXPECT_TRUE(bs.test(3));
    EXPECT_FALSE(bs.test(2));
    EXPECT_TRUE(bs.test(1));
    EXPECT_FALSE(bs.test(0));
}

TEST(BitsetTest, SetAll) {
    bitset<8> bs;
    bs.set();
    EXPECT_TRUE(bs.all());
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_TRUE(bs.test(i));
    }
}

TEST(BitsetTest, SetSingleBit) {
    bitset<8> bs;
    bs.set(3);
    EXPECT_TRUE(bs.test(3));
    EXPECT_FALSE(bs.test(2));
    EXPECT_EQ(bs.count(), 1);
}

TEST(BitsetTest, SetSingleBitToFalse) {
    bitset<8> bs(0xFFu);
    bs.set(3, false);
    EXPECT_FALSE(bs.test(3));
    EXPECT_TRUE(bs.test(2));
    EXPECT_EQ(bs.count(), 7);
}

TEST(BitsetTest, ResetAll) {
    bitset<8> bs(0xFFu);
    bs.reset();
    EXPECT_TRUE(bs.none());
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_FALSE(bs.test(i));
    }
}

TEST(BitsetTest, ResetSingleBit) {
    bitset<8> bs(0xFFu);
    bs.reset(5);
    EXPECT_FALSE(bs.test(5));
    EXPECT_TRUE(bs.test(4));
    EXPECT_EQ(bs.count(), 7);
}

TEST(BitsetTest, FlipAll) {
    bitset<8> bs(0x0Fu);
    bs.flip();
    EXPECT_EQ(bs.to_ulong(), 0xF0u);
}

TEST(BitsetTest, FlipSingleBit) {
    bitset<8> bs;
    bs.flip(2);
    EXPECT_TRUE(bs.test(2));
    bs.flip(2);
    EXPECT_FALSE(bs.test(2));
}

TEST(BitsetTest, ConstSubscript) {
    const bitset<8> bs(0x0Fu);
    EXPECT_TRUE(bs[0]);
    EXPECT_TRUE(bs[1]);
    EXPECT_TRUE(bs[2]);
    EXPECT_TRUE(bs[3]);
    EXPECT_FALSE(bs[4]);
}

TEST(BitsetTest, NonConstSubscript) {
    bitset<8> bs;
    bs[3] = true;
    EXPECT_TRUE(bs.test(3));
    bs[3] = false;
    EXPECT_FALSE(bs.test(3));
}

TEST(BitsetTest, ReferenceAssignFromReference) {
    bitset<8> bs1(0x01u);
    bitset<8> bs2(0x80u);
    bs1[0] = bs2[7];
    EXPECT_TRUE(bs1.test(0));
}

TEST(BitsetTest, ReferenceFlip) {
    bitset<8> bs;
    bs[5].flip();
    EXPECT_TRUE(bs.test(5));
    bs[5].flip();
    EXPECT_FALSE(bs.test(5));
}

TEST(BitsetTest, ReferenceBoolConversion) {
    bitset<8> bs(0x04u);
    bool val = static_cast<bool>(bs[2]);
    EXPECT_TRUE(val);
}

TEST(BitsetTest, Test) {
    bitset<8> bs(0xAAu);
    EXPECT_TRUE(bs.test(1));
    EXPECT_TRUE(bs.test(3));
    EXPECT_TRUE(bs.test(5));
    EXPECT_TRUE(bs.test(7));
    EXPECT_FALSE(bs.test(0));
    EXPECT_FALSE(bs.test(2));
}

TEST(BitsetTest, AndAssignment) {
    bitset<8> bs1(0x0Fu);
    bitset<8> bs2(0xF0u);
    bs1 &= bs2;
    EXPECT_EQ(bs1.to_ulong(), 0u);
}

TEST(BitsetTest, OrAssignment) {
    bitset<8> bs1(0x0Fu);
    bitset<8> bs2(0xF0u);
    bs1 |= bs2;
    EXPECT_EQ(bs1.to_ulong(), 0xFFu);
}

TEST(BitsetTest, XorAssignment) {
    bitset<8> bs1(0xFFu);
    bitset<8> bs2(0x0Fu);
    bs1 ^= bs2;
    EXPECT_EQ(bs1.to_ulong(), 0xF0u);
}

TEST(BitsetTest, BitwiseNot) {
    bitset<8> bs(0x0Fu);
    auto res = ~bs;
    EXPECT_EQ(res.to_ulong(), 0xF0u);
}

TEST(BitsetTest, LeftShiftAssignment) {
    bitset<8> bs(0x0Fu);
    bs <<= 2;
    EXPECT_EQ(bs.to_ulong(), 0x3Cu);
}

TEST(BitsetTest, LeftShiftAssignmentLarge) {
    bitset<8> bs(0xFFu);
    bs <<= 10;
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, LeftShiftAssignmentExactSize) {
    bitset<8> bs(0xFFu);
    bs <<= 8;
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, RightShiftAssignment) {
    bitset<8> bs(0xF0u);
    bs >>= 2;
    EXPECT_EQ(bs.to_ulong(), 0x3Cu);
}

TEST(BitsetTest, RightShiftAssignmentLarge) {
    bitset<8> bs(0xFFu);
    bs >>= 10;
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, RightShiftAssignmentExactSize) {
    bitset<8> bs(0xFFu);
    bs >>= 8;
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, LeftShiftAssignmentCrossBlock) {
    bitset<128> bs;
    bs.set(0);
    bs.set(63);
    bs <<= 64;
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(127));
    EXPECT_FALSE(bs.test(0));
    EXPECT_FALSE(bs.test(63));
}

TEST(BitsetTest, RightShiftAssignmentCrossBlock) {
    bitset<128> bs;
    bs.set(64);
    bs.set(127);
    bs >>= 64;
    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));
    EXPECT_FALSE(bs.test(64));
    EXPECT_FALSE(bs.test(127));
}

TEST(BitsetTest, Count) {
    bitset<8> bs(0xAAu);
    EXPECT_EQ(bs.count(), 4);
}

TEST(BitsetTest, CountAllOnes) {
    bitset<8> bs(0xFFu);
    EXPECT_EQ(bs.count(), 8);
}

TEST(BitsetTest, CountAllZeros) {
    bitset<8> bs;
    EXPECT_EQ(bs.count(), 0);
}

TEST(BitsetTest, All) {
    bitset<8> bs(0xFFu);
    EXPECT_TRUE(bs.all());
    bs.reset(0);
    EXPECT_FALSE(bs.all());
}

TEST(BitsetTest, Any) {
    bitset<8> bs;
    EXPECT_FALSE(bs.any());
    bs.set(4);
    EXPECT_TRUE(bs.any());
}

TEST(BitsetTest, None) {
    bitset<8> bs;
    EXPECT_TRUE(bs.none());
    bs.set(0);
    EXPECT_FALSE(bs.none());
}

TEST(BitsetTest, ToUlong) {
    bitset<8> bs(0xABu);
    EXPECT_EQ(bs.to_ulong(), 0xABuL);
}

TEST(BitsetTest, ToUlongLarge) {
    bitset<32> bs(0xDEADBEEFuL);
    EXPECT_EQ(bs.to_ulong(), 0xDEADBEEFuL);
}

TEST(BitsetTest, ToUllong) {
    bitset<64> bs(0xDEADBEEFCAFEBABEuLL);
    EXPECT_EQ(bs.to_ullong(), 0xDEADBEEFCAFEBABEuLL);
}

TEST(BitsetTest, EqualTo) {
    bitset<8> bs1(0xAAu);
    bitset<8> bs2(0xAAu);
    EXPECT_TRUE(bs1.equal_to(bs2));
}

TEST(BitsetTest, NotEqualTo) {
    bitset<8> bs1(0xAAu);
    bitset<8> bs2(0x55u);
    EXPECT_FALSE(bs1.equal_to(bs2));
}

TEST(BitsetTest, LessThan) {
    bitset<8> bs1(0x0Fu);
    bitset<8> bs2(0xF0u);
    EXPECT_TRUE(bs1.less_than(bs2));
    EXPECT_FALSE(bs2.less_than(bs1));
}

TEST(BitsetTest, EqualityOperator) {
    bitset<8> bs1(0xAAu);
    bitset<8> bs2(0xAAu);
    bitset<8> bs3(0x55u);
    EXPECT_TRUE(bs1 == bs2);
    EXPECT_FALSE(bs1 == bs3);
}

TEST(BitsetTest, InequalityOperator) {
    bitset<8> bs1(0xAAu);
    bitset<8> bs2(0x55u);
    EXPECT_TRUE(bs1 != bs2);
}

TEST(BitsetTest, LessThanOperator) {
    bitset<8> bs1(0x0Fu);
    bitset<8> bs2(0xF0u);
    EXPECT_TRUE(bs1 < bs2);
}

TEST(BitsetTest, GreaterThanOperator) {
    bitset<8> bs1(0xF0u);
    bitset<8> bs2(0x0Fu);
    EXPECT_TRUE(bs1 > bs2);
}

TEST(BitsetTest, LessThanOrEqualOperator) {
    bitset<8> bs1(0x0Fu);
    bitset<8> bs2(0x0Fu);
    bitset<8> bs3(0xF0u);
    EXPECT_TRUE(bs1 <= bs2);
    EXPECT_TRUE(bs1 <= bs3);
}

TEST(BitsetTest, GreaterThanOrEqualOperator) {
    bitset<8> bs1(0xF0u);
    bitset<8> bs2(0xF0u);
    bitset<8> bs3(0x0Fu);
    EXPECT_TRUE(bs1 >= bs2);
    EXPECT_TRUE(bs1 >= bs3);
}

TEST(BitsetTest, ToStringDefault) {
    bitset<8> bs(0xAAu);
    string str = bs.to_string();
    EXPECT_EQ(str, "10101010"_s);
}

TEST(BitsetTest, ToStringCustomChars) {
    bitset<8> bs(0xAAu);
    string str = bs.to_string('F', 'T');
    EXPECT_EQ(str, "TFTFTFTF"_s);
}

TEST(BitsetTest, ToStringZero) {
    bitset<8> bs;
    string str = bs.to_string();
    EXPECT_EQ(str, "00000000"_s);
}

TEST(BitsetTest, ToStringAllOnes) {
    bitset<8> bs(0xFFu);
    string str = bs.to_string();
    EXPECT_EQ(str, "11111111"_s);
}

TEST(BitsetTest, Swap) {
    bitset<8> bs1(0xAAu);
    bitset<8> bs2(0x55u);
    bs1.swap(bs2);
    EXPECT_EQ(bs1.to_ulong(), 0x55u);
    EXPECT_EQ(bs2.to_ulong(), 0xAAu);
}

TEST(BitsetTest, LargeBitset) {
    bitset<1000> bs;
    EXPECT_EQ(bs.size(), 1000);
    EXPECT_TRUE(bs.none());
    bs.set(999);
    EXPECT_TRUE(bs.test(999));
    bs.set(0);
    EXPECT_TRUE(bs.test(0));
    EXPECT_EQ(bs.count(), 2);
}

TEST(BitsetTest, CrossWordBoundarySet) {
    bitset<128> bs;
    bs.set(0);
    bs.set(63);
    bs.set(64);
    bs.set(127);
    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(127));
}

TEST(BitsetTest, CrossWordBoundaryReset) {
    bitset<128> bs;
    bs.set();
    bs.reset(0);
    bs.reset(63);
    bs.reset(64);
    bs.reset(127);
    EXPECT_FALSE(bs.test(0));
    EXPECT_FALSE(bs.test(63));
    EXPECT_FALSE(bs.test(64));
    EXPECT_FALSE(bs.test(127));
    EXPECT_EQ(bs.count(), 124);
}

TEST(BitsetTest, CrossWordBoundaryFlip) {
    bitset<128> bs;
    bs.flip(0);
    bs.flip(63);
    bs.flip(64);
    bs.flip(127);
    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(127));
}

TEST(BitsetTest, ReferenceLargeBitset) {
    bitset<256> bs;
    bs[200] = true;
    EXPECT_TRUE(bs.test(200));
    bool val = static_cast<bool>(bs[200]);
    EXPECT_TRUE(val);
}

TEST(BitsetTest, AndAssignmentLarge) {
    bitset<128> bs1;
    bs1.set();
    bitset<128> bs2;
    bs2.set(0);
    bs2.set(64);
    bs1 &= bs2;
    EXPECT_TRUE(bs1.test(0));
    EXPECT_TRUE(bs1.test(64));
    EXPECT_FALSE(bs1.test(1));
    EXPECT_FALSE(bs1.test(63));
}

TEST(BitsetTest, OrAssignmentLarge) {
    bitset<128> bs1;
    bs1.set(0);
    bitset<128> bs2;
    bs2.set(64);
    bs1 |= bs2;
    EXPECT_TRUE(bs1.test(0));
    EXPECT_TRUE(bs1.test(64));
}

TEST(BitsetTest, XorAssignmentLarge) {
    bitset<128> bs1;
    bs1.set();
    bitset<128> bs2;
    bs2.set(0);
    bs2.set(64);
    bs1 ^= bs2;
    EXPECT_FALSE(bs1.test(0));
    EXPECT_FALSE(bs1.test(64));
    EXPECT_TRUE(bs1.test(1));
    EXPECT_TRUE(bs1.test(63));
}

TEST(BitsetTest, BitwiseNotLarge) {
    bitset<128> bs;
    bs.set(0);
    bs.set(64);
    auto res = ~bs;
    EXPECT_FALSE(res.test(0));
    EXPECT_FALSE(res.test(64));
    EXPECT_TRUE(res.test(1));
    EXPECT_TRUE(res.test(63));
}

TEST(BitsetTest, MaskCorrectness) {
    bitset<10> bs;
    bs.set();
    EXPECT_TRUE(bs.test(9));
    EXPECT_EQ(bs.count(), 10);
    bs.flip();
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, NonMultipleOfWordSize) {
    bitset<10> bs(0x3FFu);
    EXPECT_TRUE(bs.all());
    EXPECT_EQ(bs.count(), 10);
    bs >>= 2;
    EXPECT_FALSE(bs.test(9));
    EXPECT_FALSE(bs.test(8));
    EXPECT_TRUE(bs.test(7));
}

TEST(BitsetTest, SingleWordBitset) {
    bitset<32> bs(0xFFFFFFFFu);
    EXPECT_TRUE(bs.all());
    bs.set(31, false);
    EXPECT_EQ(bs.count(), 31);
}

TEST(BitsetTest, ExactWordBitset) {
    bitset<64> bs;
    bs.set();
    EXPECT_TRUE(bs.all());
    EXPECT_EQ(bs.count(), 64);
}

TEST(BitsetTest, ZeroSizeBitset) {
    bitset<0> bs;
    EXPECT_EQ(bs.size(), 0);
    EXPECT_TRUE(bs.empty());
    EXPECT_TRUE(bs.none());
    EXPECT_FALSE(bs.any());
    EXPECT_TRUE(bs.all());
    EXPECT_EQ(bs.count(), 0);
}

TEST(BitsetTest, ZeroSizeBitsetEquality) {
    bitset<0> bs1;
    bitset<0> bs2;
    EXPECT_TRUE(bs1 == bs2);
    EXPECT_FALSE(bs1 < bs2);
}

TEST(BitsetTest, ToStringLarge) {
    bitset<5> bs(0b11001u);
    string str = bs.to_string();
    EXPECT_EQ(str, "11001"_s);
}

TEST(BitsetTest, StringConstructorEmpty) {
    bitset<4> bs(string_view{""});
    EXPECT_TRUE(bs.none());
}

TEST(BitsetTest, StringConstructorAllSameChar) {
    bitset<8> bs("oooooooo"_sv, 'x', 'o');
    EXPECT_TRUE(bs.all());
}

#ifdef NEFORCE_STATE_DEBUG
TEST(BitsetDeathTest, OutOfRangeSet) {
    bitset<8> bs;
    EXPECT_DEBUG_DEATH(bs.set(8), "");
}

TEST(BitsetDeathTest, OutOfRangeReset) {
    bitset<8> bs;
    EXPECT_DEBUG_DEATH(bs.reset(8), "");
}

TEST(BitsetDeathTest, OutOfRangeFlip) {
    bitset<8> bs;
    EXPECT_DEBUG_DEATH(bs.flip(8), "");
}

TEST(BitsetDeathTest, OutOfRangeTest) {
    bitset<8> bs;
    EXPECT_DEBUG_DEATH(ignore = bs.test(8), "");
}

TEST(BitsetDeathTest, OutOfRangeSubscript) {
    bitset<8> bs;
    EXPECT_DEBUG_DEATH(ignore = bs[8], "");
}

TEST(BitsetDeathTest, OutOfRangeConstSubscript) {
    const bitset<8> bs;
    EXPECT_DEBUG_DEATH(ignore = bs[8], "");
}
#endif

class BloomFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BloomFilterTest, ConstructorWithExpectedInsertionsAndFpp) {
    bloom_filter<int> bf(1000, 0.01);
    EXPECT_GT(bf.bit_size(), 0);
    EXPECT_GT(bf.hash_count(), 0);
    EXPECT_TRUE(bf.empty());
}

TEST_F(BloomFilterTest, ConstructorWithDirectParameters) {
    bloom_filter<int> bf(static_cast<size_t>(10000), static_cast<size_t>(7));
    EXPECT_EQ(bf.bit_size(), 10000);
    EXPECT_EQ(bf.hash_count(), 7);
    EXPECT_TRUE(bf.empty());
}

TEST_F(BloomFilterTest, ConstructorWithZeroExpectedInsertions) {
    EXPECT_THROW(bloom_filter<int>(0, 0.01), value_exception);
}

TEST_F(BloomFilterTest, ConstructorWithInvalidFppZero) { EXPECT_THROW(bloom_filter<int>(1000, 0.0), exception); }

TEST_F(BloomFilterTest, ConstructorWithInvalidFppOne) { EXPECT_THROW(bloom_filter<int>(1000, 1.0), value_exception); }

TEST_F(BloomFilterTest, ConstructorWithInvalidFppNegative) { EXPECT_THROW(bloom_filter<int>(1000, -0.1), exception); }

TEST_F(BloomFilterTest, ConstructorWithInvalidFppGreaterThanOne) {
    EXPECT_THROW(bloom_filter<int>(1000, 1.5), exception);
}

TEST_F(BloomFilterTest, ConstructorWithZeroM) {
    EXPECT_THROW(bloom_filter<int>(static_cast<size_t>(0), static_cast<size_t>(3)), value_exception);
}

TEST_F(BloomFilterTest, ConstructorWithZeroK) {
    EXPECT_THROW(bloom_filter<int>(static_cast<size_t>(1000), static_cast<size_t>(0)), value_exception);
}

TEST_F(BloomFilterTest, InsertAndContains) {
    bloom_filter<int> bf(1000, 0.01);
    bf.insert(42);
    EXPECT_TRUE(bf.contains(42));
}

TEST_F(BloomFilterTest, ContainsNonInsertedElement) {
    bloom_filter<int> bf(1000, 0.01);
    bf.insert(42);
    EXPECT_FALSE(bf.contains(100));
}

TEST_F(BloomFilterTest, MultipleInsertions) {
    bloom_filter<int> bf(10000, 0.001);
    for (int i = 0; i < 100; ++i) {
        bf.insert(i);
    }
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(bf.contains(i));
    }
}

TEST_F(BloomFilterTest, NoFalseNegatives) {
    bloom_filter<int> bf(10000, 0.001);
    for (int i = 0; i < 500; ++i) {
        bf.insert(i);
    }
    for (int i = 0; i < 500; ++i) {
        EXPECT_TRUE(bf.contains(i)) << "False negative for " << i;
    }
}

TEST_F(BloomFilterTest, FalsePositiveRateWithinBounds) {
    const size_t n = 1000;
    bloom_filter<int> bf(n, 0.01);
    for (size_t i = 0; i < n; ++i) {
        bf.insert(static_cast<int>(i));
    }
    const size_t test_count = 10000;
    size_t false_positives = 0;
    for (size_t i = n; i < n + test_count; ++i) {
        if (bf.contains(static_cast<int>(i))) {
            ++false_positives;
        }
    }
    double actual_fpr = static_cast<double>(false_positives) / test_count;
    EXPECT_LT(actual_fpr, 0.05);
}

TEST_F(BloomFilterTest, Empty) {
    bloom_filter<int> bf(1000, 0.01);
    EXPECT_TRUE(bf.empty());
    bf.insert(1);
    EXPECT_FALSE(bf.empty());
}

TEST_F(BloomFilterTest, Clear) {
    bloom_filter<int> bf(1000, 0.01);
    bf.insert(1);
    bf.insert(2);
    bf.insert(3);
    EXPECT_FALSE(bf.empty());
    bf.clear();
    EXPECT_TRUE(bf.empty());
    EXPECT_FALSE(bf.contains(1));
    EXPECT_FALSE(bf.contains(2));
    EXPECT_FALSE(bf.contains(3));
}

TEST_F(BloomFilterTest, BitSize) {
    bloom_filter<int> bf(1000, 0.01);
    EXPECT_GT(bf.bit_size(), 0);
}

TEST_F(BloomFilterTest, HashCount) {
    bloom_filter<int> bf(1000, 0.01);
    EXPECT_GE(bf.hash_count(), 1);
}

TEST_F(BloomFilterTest, Capacity) {
    bloom_filter<int> bf(1000, 0.01);
    size_t cap = bf.capacity();
    EXPECT_GT(cap, 0);
}

TEST_F(BloomFilterTest, ApproximateCount) {
    bloom_filter<int> bf(10000, 0.001);
    for (int i = 0; i < 500; ++i) {
        bf.insert(i);
    }
    size_t approx = bf.approximate_count();
    EXPECT_GT(approx, 0);
    EXPECT_LT(static_cast<double>(abs(static_cast<long>(approx) - 500)) / 500, 0.5);
}

TEST_F(BloomFilterTest, FalsePositiveRateEstimation) {
    bloom_filter<int> bf(10000, 0.001);
    for (int i = 0; i < 500; ++i) {
        bf.insert(i);
    }
    double fpr = bf.false_positive_rate();
    EXPECT_GE(fpr, 0.0);
    EXPECT_LE(fpr, 1.0);
}

TEST_F(BloomFilterTest, Merge) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf1.insert(1);
    bf1.insert(2);
    bf2.insert(2);
    bf2.insert(3);
    bf1.merge(bf2);
    EXPECT_TRUE(bf1.contains(1));
    EXPECT_TRUE(bf1.contains(2));
    EXPECT_TRUE(bf1.contains(3));
}

TEST_F(BloomFilterTest, MergeDifferentParameters) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bloom_filter<int> bf2(static_cast<size_t>(2000), static_cast<size_t>(5));
    EXPECT_THROW(bf1.merge(bf2), value_exception);
}

TEST_F(BloomFilterTest, Intersect) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf1.insert(1);
    bf1.insert(2);
    bf2.insert(2);
    bf2.insert(3);
    auto bf3 = bf1.intersect(bf2);
    EXPECT_FALSE(bf3.contains(1));
    EXPECT_TRUE(bf3.contains(2));
    EXPECT_FALSE(bf3.contains(3));
}

TEST_F(BloomFilterTest, IntersectDifferentParameters) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bloom_filter<int> bf2(static_cast<size_t>(2000), static_cast<size_t>(5));
    EXPECT_THROW(bf1.intersect(bf2), value_exception);
}

TEST_F(BloomFilterTest, Unite) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf1.insert(1);
    bf1.insert(2);
    bf2.insert(2);
    bf2.insert(3);
    auto bf3 = bf1.unite(bf2);
    EXPECT_TRUE(bf3.contains(1));
    EXPECT_TRUE(bf3.contains(2));
    EXPECT_TRUE(bf3.contains(3));
}

TEST_F(BloomFilterTest, UniteDoesNotModifyOriginal) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf1.insert(1);
    bf2.insert(2);
    auto bf3 = bf1.unite(bf2);
    EXPECT_TRUE(bf1.contains(1));
    EXPECT_FALSE(bf1.contains(2));
    EXPECT_TRUE(bf2.contains(2));
    EXPECT_FALSE(bf2.contains(1));
    EXPECT_TRUE(bf3.contains(1));
    EXPECT_TRUE(bf3.contains(2));
}

TEST_F(BloomFilterTest, ToBytesAndFromBytes) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf1.insert(1);
    bf1.insert(2);
    bf1.insert(3);
    auto bytes = bf1.to_bytes();
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf2.from_bytes(bytes);
    EXPECT_TRUE(bf2.contains(1));
    EXPECT_TRUE(bf2.contains(2));
    EXPECT_TRUE(bf2.contains(3));
}

TEST_F(BloomFilterTest, FromBytesInsufficientData) {
    bloom_filter<int> bf(static_cast<size_t>(1000000), static_cast<size_t>(3));
    byte_vector bytes(1);
    EXPECT_THROW(bf.from_bytes(bytes), value_exception);
}

TEST_F(BloomFilterTest, CopyConstructor) {
    bloom_filter<int> bf1(1000, 0.01);
    bf1.insert(42);
    bloom_filter<int> bf2(bf1);
    EXPECT_EQ(bf1.bit_size(), bf2.bit_size());
    EXPECT_EQ(bf1.hash_count(), bf2.hash_count());
    EXPECT_TRUE(bf2.contains(42));
}

TEST_F(BloomFilterTest, CopyAssignment) {
    bloom_filter<int> bf1(1000, 0.01);
    bf1.insert(42);
    bloom_filter<int> bf2(2000, 0.1);
    bf2 = bf1;
    EXPECT_EQ(bf2.bit_size(), bf1.bit_size());
    EXPECT_EQ(bf2.hash_count(), bf1.hash_count());
    EXPECT_TRUE(bf2.contains(42));
}

TEST_F(BloomFilterTest, MoveConstructor) {
    bloom_filter<int> bf1(1000, 0.01);
    bf1.insert(42);
    size_t m = bf1.bit_size();
    size_t k = bf1.hash_count();
    bloom_filter<int> bf2(move(bf1));
    EXPECT_EQ(bf2.bit_size(), m);
    EXPECT_EQ(bf2.hash_count(), k);
    EXPECT_TRUE(bf2.contains(42));
}

TEST_F(BloomFilterTest, MoveAssignment) {
    bloom_filter<int> bf1(1000, 0.01);
    bf1.insert(42);
    size_t m = bf1.bit_size();
    size_t k = bf1.hash_count();
    bloom_filter<int> bf2(2000, 0.1);
    bf2 = move(bf1);
    EXPECT_EQ(bf2.bit_size(), m);
    EXPECT_EQ(bf2.hash_count(), k);
    EXPECT_TRUE(bf2.contains(42));
}

TEST_F(BloomFilterTest, StringType) {
    bloom_filter<string> bf(10000, 0.001);
    bf.insert("hello");
    bf.insert("world");
    EXPECT_TRUE(bf.contains("hello"));
    EXPECT_TRUE(bf.contains("world"));
    EXPECT_FALSE(bf.contains("foo"));
}

TEST_F(BloomFilterTest, StringTypeMultipleInsertions) {
    bloom_filter<string> bf(100000, 0.0001);
    set<string> inserted;
    for (int i = 0; i < 1000; ++i) {
        string s = "key_" + neforce::to_string(i);
        bf.insert(s);
        inserted.insert(s);
    }
    for (const auto& s: inserted) {
        EXPECT_TRUE(bf.contains(s)) << "False negative for " << s.data();
    }
}

TEST_F(BloomFilterTest, LargeScaleTest) {
    bloom_filter<int> bf(1000000, 0.001);
    const int n = 100000;
    for (int i = 0; i < n; ++i) {
        bf.insert(i);
    }
    for (int i = 0; i < n; ++i) {
        EXPECT_TRUE(bf.contains(i));
    }
}

TEST_F(BloomFilterTest, ClearAndReinsert) {
    bloom_filter<int> bf(1000, 0.01);
    bf.insert(42);
    EXPECT_TRUE(bf.contains(42));
    bf.clear();
    EXPECT_FALSE(bf.contains(42));
    bf.insert(42);
    EXPECT_TRUE(bf.contains(42));
}

TEST_F(BloomFilterTest, VeryLowFpp) {
    bloom_filter<int> bf(1000, 0.0001);
    for (int i = 0; i < 1000; ++i) {
        bf.insert(i);
    }
    const size_t test_count = 10000;
    size_t false_positives = 0;
    for (size_t i = 1000; i < 1000 + test_count; ++i) {
        if (bf.contains(static_cast<int>(i))) {
            ++false_positives;
        }
    }
    double actual_fpr = static_cast<double>(false_positives) / test_count;
    EXPECT_LT(actual_fpr, 0.02);
}

TEST_F(BloomFilterTest, ToBytesRoundTripEmpty) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    auto bytes = bf1.to_bytes();
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf2.from_bytes(bytes);
    EXPECT_TRUE(bf2.empty());
}

TEST_F(BloomFilterTest, ToBytesRoundTripFull) {
    bloom_filter<int> bf1(static_cast<size_t>(1000), static_cast<size_t>(3));
    for (int i = 0; i < 100; ++i) {
        bf1.insert(i);
    }
    auto bytes = bf1.to_bytes();
    bloom_filter<int> bf2(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf2.from_bytes(bytes);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(bf2.contains(i));
    }
}

TEST_F(BloomFilterTest, EmptyApproximateCount) {
    bloom_filter<int> bf(10000, 0.01);
    size_t approx = bf.approximate_count();
    EXPECT_EQ(approx, 0);
}

TEST_F(BloomFilterTest, EmptyFalsePositiveRate) {
    bloom_filter<int> bf(10000, 0.01);
    double fpr = bf.false_positive_rate();
    EXPECT_EQ(fpr, 0.0);
}

TEST_F(BloomFilterTest, OptimalParametersCalculation) {
    bloom_filter<int> bf(1000000, 0.01);
    EXPECT_GT(bf.bit_size(), 0);
    EXPECT_GE(bf.hash_count(), 1);
    double fpr = bf.false_positive_rate();
    EXPECT_LT(fpr, 0.01);
}

TEST_F(BloomFilterTest, MergeWithSelf) {
    bloom_filter<int> bf(static_cast<size_t>(1000), static_cast<size_t>(3));
    bf.insert(1);
    auto& result = bf.merge(bf);
    EXPECT_EQ(&result, &bf);
    EXPECT_TRUE(bf.contains(1));
}

class DequeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(DequeTest, DefaultConstructor) {
    deque<int> dq;
    EXPECT_TRUE(dq.empty());
    EXPECT_EQ(dq.size(), 0);
}

TEST_F(DequeTest, ConstructorWithSize) {
    deque<int> dq(5);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_FALSE(dq.empty());
    for (size_t i = 0; i < dq.size(); ++i) {
        EXPECT_EQ(dq[i], 0);
    }
}

TEST_F(DequeTest, ConstructorWithSizeAndValue) {
    deque<int> dq(5, 42);
    EXPECT_EQ(dq.size(), 5);
    for (size_t i = 0; i < dq.size(); ++i) {
        EXPECT_EQ(dq[i], 42);
    }
}

TEST_F(DequeTest, ConstructorWithZeroSize) {
    deque<int> dq(0);
    EXPECT_TRUE(dq.empty());
    EXPECT_EQ(dq.size(), 0);
}

TEST_F(DequeTest, InitializerListConstructor) {
    deque<int> dq = {1, 2, 3, 4, 5};
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[1], 2);
    EXPECT_EQ(dq[2], 3);
    EXPECT_EQ(dq[3], 4);
    EXPECT_EQ(dq[4], 5);
}

TEST_F(DequeTest, InitializerListAssignment) {
    deque<int> dq = {1, 2, 3};
    dq = {4, 5, 6, 7};
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[0], 4);
    EXPECT_EQ(dq[1], 5);
    EXPECT_EQ(dq[2], 6);
    EXPECT_EQ(dq[3], 7);
}

TEST_F(DequeTest, RangeConstructor) {
    vector<int> vec = {10, 20, 30, 40, 50};
    deque<int> dq(vec.begin(), vec.end());
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[0], 10);
    EXPECT_EQ(dq[4], 50);
}

TEST_F(DequeTest, RangeConstructorWithList) {
    list<int> lst = {1, 2, 3};
    deque<int> dq(lst.begin(), lst.end());
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[2], 3);
}

TEST_F(DequeTest, RangeConstructorEmpty) {
    vector<int> vec;
    deque<int> dq(vec.begin(), vec.end());
    EXPECT_TRUE(dq.empty());
}

TEST_F(DequeTest, CopyConstructor) {
    deque<int> dq1 = {1, 2, 3, 4, 5};
    deque<int> dq2(dq1);
    EXPECT_EQ(dq2.size(), 5);
    EXPECT_EQ(dq2[0], 1);
    EXPECT_EQ(dq2[4], 5);
}

TEST_F(DequeTest, CopyAssignment) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {4, 5, 6, 7, 8};
    dq2 = dq1;
    EXPECT_EQ(dq2.size(), 3);
    EXPECT_EQ(dq2[0], 1);
    EXPECT_EQ(dq2[2], 3);
}

TEST_F(DequeTest, CopyAssignmentSelf) {
    deque<int> dq = {1, 2, 3};
    dq = dq;
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], 1);
}

TEST_F(DequeTest, MoveConstructor) {
    deque<int> dq1 = {1, 2, 3, 4, 5};
    deque<int> dq2(move(dq1));
    EXPECT_EQ(dq2.size(), 5);
    EXPECT_EQ(dq2[0], 1);
    EXPECT_EQ(dq2[4], 5);
}

TEST_F(DequeTest, MoveAssignment) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {4, 5, 6, 7};
    dq2 = move(dq1);
    EXPECT_EQ(dq2.size(), 3);
    EXPECT_EQ(dq2[0], 1);
    EXPECT_EQ(dq2[2], 3);
}

TEST_F(DequeTest, MoveAssignmentSelf) {
    deque<int> dq = {1, 2, 3};
    dq = move(dq);
    EXPECT_EQ(dq.size(), 3);
}

TEST_F(DequeTest, BeginEnd) {
    deque<int> dq = {1, 2, 3, 4, 5};
    auto it = dq.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    auto end_it = dq.end();
    EXPECT_EQ(end_it - dq.begin(), 5);
}

TEST_F(DequeTest, ConstBeginEnd) {
    const deque<int> dq = {1, 2, 3, 4, 5};
    auto it = dq.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(DequeTest, CbeginCend) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.cbegin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(DequeTest, ReverseBeginEnd) {
    deque<int> dq = {1, 2, 3};
    auto rit = dq.rbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
}

TEST_F(DequeTest, ConstReverseBeginEnd) {
    const deque<int> dq = {1, 2, 3};
    auto rit = dq.rbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(DequeTest, CrbeginCrend) {
    deque<int> dq = {1, 2, 3};
    auto rit = dq.crbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
}

TEST_F(DequeTest, Size) {
    deque<int> dq;
    EXPECT_EQ(dq.size(), 0);
    dq.push_back(1);
    EXPECT_EQ(dq.size(), 1);
    dq.push_back(2);
    EXPECT_EQ(dq.size(), 2);
}

TEST_F(DequeTest, MaxSize) {
    deque<int> dq;
    EXPECT_GT(dq.max_size(), 0);
}

TEST_F(DequeTest, Empty) {
    deque<int> dq;
    EXPECT_TRUE(dq.empty());
    dq.push_back(1);
    EXPECT_FALSE(dq.empty());
}

TEST_F(DequeTest, Front) {
    deque<int> dq = {10, 20, 30};
    EXPECT_EQ(dq.front(), 10);
    dq.front() = 100;
    EXPECT_EQ(dq.front(), 100);
}

TEST_F(DequeTest, ConstFront) {
    const deque<int> dq = {10, 20, 30};
    EXPECT_EQ(dq.front(), 10);
}

TEST_F(DequeTest, Back) {
    deque<int> dq = {10, 20, 30};
    EXPECT_EQ(dq.back(), 30);
    dq.back() = 300;
    EXPECT_EQ(dq.back(), 300);
}

TEST_F(DequeTest, ConstBack) {
    const deque<int> dq = {10, 20, 30};
    EXPECT_EQ(dq.back(), 30);
}

TEST_F(DequeTest, At) {
    deque<int> dq = {1, 2, 3, 4, 5};
    EXPECT_EQ(dq.at(0), 1);
    EXPECT_EQ(dq.at(4), 5);
    dq.at(2) = 10;
    EXPECT_EQ(dq.at(2), 10);
}

TEST_F(DequeTest, ConstAt) {
    const deque<int> dq = {1, 2, 3, 4, 5};
    EXPECT_EQ(dq.at(0), 1);
    EXPECT_EQ(dq.at(4), 5);
}

TEST_F(DequeTest, SubscriptOperator) {
    deque<int> dq = {1, 2, 3, 4, 5};
    EXPECT_EQ(dq[0], 1);
    dq[2] = 10;
    EXPECT_EQ(dq[2], 10);
}

TEST_F(DequeTest, ConstSubscriptOperator) {
    const deque<int> dq = {1, 2, 3};
    EXPECT_EQ(dq[1], 2);
}

TEST_F(DequeTest, PushBack) {
    deque<int> dq;
    dq.push_back(1);
    dq.push_back(2);
    dq.push_back(3);
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[1], 2);
    EXPECT_EQ(dq[2], 3);
}

TEST_F(DequeTest, PushFront) {
    deque<int> dq;
    dq.push_front(1);
    dq.push_front(2);
    dq.push_front(3);
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], 3);
    EXPECT_EQ(dq[1], 2);
    EXPECT_EQ(dq[2], 1);
}

TEST_F(DequeTest, PushBackRvalue) {
    deque<string> dq;
    string s = "hello";
    dq.push_back(move(s));
    EXPECT_EQ(dq.size(), 1);
    EXPECT_EQ(dq[0], "hello");
}

TEST_F(DequeTest, PushFrontRvalue) {
    deque<string> dq;
    string s = "world";
    dq.push_front(move(s));
    EXPECT_EQ(dq.size(), 1);
    EXPECT_EQ(dq[0], "world");
}

TEST_F(DequeTest, PopBack) {
    deque<int> dq = {1, 2, 3};
    dq.pop_back();
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq.back(), 2);
    dq.pop_back();
    EXPECT_EQ(dq.size(), 1);
    EXPECT_EQ(dq.back(), 1);
}

TEST_F(DequeTest, PopFront) {
    deque<int> dq = {1, 2, 3};
    dq.pop_front();
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq.front(), 2);
    dq.pop_front();
    EXPECT_EQ(dq.size(), 1);
    EXPECT_EQ(dq.front(), 3);
}

TEST_F(DequeTest, EmplaceBack) {
    deque<string> dq;
    dq.emplace_back("hello");
    dq.emplace_back("world");
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq[0], "hello");
    EXPECT_EQ(dq[1], "world");
}

TEST_F(DequeTest, EmplaceFront) {
    deque<string> dq;
    dq.emplace_front("hello");
    dq.emplace_front("world");
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq[0], "world");
    EXPECT_EQ(dq[1], "hello");
}

TEST_F(DequeTest, Emplace) {
    deque<int> dq = {1, 2, 4, 5};
    auto it = dq.emplace(dq.begin() + 2, 3);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[2], 3);
    EXPECT_EQ(*it, 3);
}

TEST_F(DequeTest, EmplaceAtBegin) {
    deque<int> dq = {2, 3, 4};
    auto it = dq.emplace(dq.begin(), 1);
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(it, dq.begin());
}

TEST_F(DequeTest, EmplaceAtEnd) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.emplace(dq.end(), 4);
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[3], 4);
    EXPECT_EQ(*it, 4);
}

TEST_F(DequeTest, InsertSingleElement) {
    deque<int> dq = {1, 2, 4, 5};
    auto it = dq.insert(dq.begin() + 2, 3);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[2], 3);
    EXPECT_EQ(*it, 3);
}

TEST_F(DequeTest, InsertSingleElementAtBegin) {
    deque<int> dq = {2, 3, 4};
    auto it = dq.insert(dq.begin(), 1);
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[0], 1);
}

TEST_F(DequeTest, InsertSingleElementAtEnd) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.insert(dq.end(), 4);
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[3], 4);
}

TEST_F(DequeTest, InsertRvalue) {
    deque<string> dq = {"hello", "world"};
    string s = "beautiful";
    dq.insert(dq.begin() + 1, move(s));
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[1], "beautiful");
}

TEST_F(DequeTest, InsertNValues) {
    deque<int> dq = {1, 5};
    dq.insert(dq.begin() + 1, 3, 9);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[1], 9);
    EXPECT_EQ(dq[2], 9);
    EXPECT_EQ(dq[3], 9);
    EXPECT_EQ(dq[4], 5);
}

TEST_F(DequeTest, InsertNValuesAtBegin) {
    deque<int> dq = {4, 5};
    dq.insert(dq.begin(), 2, 9);
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[0], 9);
    EXPECT_EQ(dq[1], 9);
    EXPECT_EQ(dq[2], 4);
}

TEST_F(DequeTest, InsertNValuesAtEnd) {
    deque<int> dq = {1, 2};
    dq.insert(dq.end(), 3, 9);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[2], 9);
    EXPECT_EQ(dq[3], 9);
    EXPECT_EQ(dq[4], 9);
}

TEST_F(DequeTest, InsertRange) {
    deque<int> dq = {1, 5};
    vector<int> vals = {2, 3, 4};
    dq.insert(dq.begin() + 1, vals.begin(), vals.end());
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[1], 2);
    EXPECT_EQ(dq[2], 3);
    EXPECT_EQ(dq[3], 4);
    EXPECT_EQ(dq[4], 5);
}

TEST_F(DequeTest, InsertRangeAtBegin) {
    deque<int> dq = {3, 4, 5};
    vector<int> vals = {1, 2};
    dq.insert(dq.begin(), vals.begin(), vals.end());
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[1], 2);
}

TEST_F(DequeTest, InsertRangeAtEnd) {
    deque<int> dq = {1, 2, 3};
    vector<int> vals = {4, 5};
    dq.insert(dq.end(), vals.begin(), vals.end());
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[3], 4);
    EXPECT_EQ(dq[4], 5);
}

TEST_F(DequeTest, InsertListRange) {
    deque<int> dq = {1, 5};
    list<int> lst = {2, 3, 4};
    dq.insert(dq.begin() + 1, lst.begin(), lst.end());
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[1], 2);
    EXPECT_EQ(dq[3], 4);
}

TEST_F(DequeTest, EraseSingleElement) {
    deque<int> dq = {1, 2, 3, 4, 5};
    auto it = dq.erase(dq.begin() + 2);
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[2], 4);
    EXPECT_EQ(*it, 4);
}

TEST_F(DequeTest, EraseSingleElementAtBegin) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.erase(dq.begin());
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq[0], 2);
    EXPECT_EQ(it, dq.begin());
}

TEST_F(DequeTest, EraseSingleElementAtEnd) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.erase(dq.end() - 1);
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq[1], 2);
    EXPECT_EQ(it, dq.end());
}

TEST_F(DequeTest, EraseRange) {
    deque<int> dq = {1, 2, 3, 4, 5};
    auto it = dq.erase(dq.begin() + 1, dq.begin() + 4);
    EXPECT_EQ(dq.size(), 2);
    EXPECT_EQ(dq[0], 1);
    EXPECT_EQ(dq[1], 5);
    EXPECT_EQ(it, dq.begin() + 1);
}

TEST_F(DequeTest, EraseAll) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.erase(dq.begin(), dq.end());
    EXPECT_TRUE(dq.empty());
    EXPECT_EQ(it, dq.end());
}

TEST_F(DequeTest, ResizeSmaller) {
    deque<int> dq = {1, 2, 3, 4, 5};
    dq.resize(3);
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[2], 3);
}

TEST_F(DequeTest, ResizeLarger) {
    deque<int> dq = {1, 2, 3};
    dq.resize(5, 42);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[3], 42);
    EXPECT_EQ(dq[4], 42);
}

TEST_F(DequeTest, ResizeLargerDefault) {
    deque<int> dq = {1, 2, 3};
    dq.resize(5);
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[3], 0);
    EXPECT_EQ(dq[4], 0);
}

TEST_F(DequeTest, AssignNValues) {
    deque<int> dq = {1, 2, 3, 4, 5};
    dq.assign(3, 42);
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], 42);
    EXPECT_EQ(dq[1], 42);
    EXPECT_EQ(dq[2], 42);
}

TEST_F(DequeTest, AssignRange) {
    deque<int> dq = {1, 2, 3};
    vector<int> vals = {10, 20, 30, 40};
    dq.assign(vals.begin(), vals.end());
    EXPECT_EQ(dq.size(), 4);
    EXPECT_EQ(dq[0], 10);
    EXPECT_EQ(dq[3], 40);
}

TEST_F(DequeTest, AssignInitializerList) {
    deque<int> dq = {1, 2, 3};
    dq.assign({10, 20, 30, 40, 50});
    EXPECT_EQ(dq.size(), 5);
    EXPECT_EQ(dq[0], 10);
    EXPECT_EQ(dq[4], 50);
}

TEST_F(DequeTest, Clear) {
    deque<int> dq = {1, 2, 3, 4, 5};
    dq.clear();
    EXPECT_TRUE(dq.empty());
    EXPECT_EQ(dq.size(), 0);
}

TEST_F(DequeTest, ShrinkToFit) {
    deque<int> dq;
    for (int i = 0; i < 1000; ++i) {
        dq.push_back(i);
    }
    for (int i = 0; i < 500; ++i) {
        dq.pop_front();
    }
    dq.shrink_to_fit();
    EXPECT_EQ(dq.size(), 500);
}

TEST_F(DequeTest, Swap) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {4, 5, 6, 7};
    dq1.swap(dq2);
    EXPECT_EQ(dq1.size(), 4);
    EXPECT_EQ(dq1[0], 4);
    EXPECT_EQ(dq2.size(), 3);
    EXPECT_EQ(dq2[0], 1);
}

TEST_F(DequeTest, SwapSelf) {
    deque<int> dq = {1, 2, 3};
    dq.swap(dq);
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], 1);
}

TEST_F(DequeTest, EqualTo) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {1, 2, 3};
    deque<int> dq3 = {1, 2, 4};
    EXPECT_TRUE(dq1.equal_to(dq2));
    EXPECT_FALSE(dq1.equal_to(dq3));
}

TEST_F(DequeTest, LessThan) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {1, 2, 4};
    EXPECT_TRUE(dq1.less_than(dq2));
    EXPECT_FALSE(dq2.less_than(dq1));
}

TEST_F(DequeTest, EqualityOperator) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {1, 2, 3};
    deque<int> dq3 = {1, 2, 4};
    EXPECT_TRUE(dq1 == dq2);
    EXPECT_FALSE(dq1 == dq3);
}

TEST_F(DequeTest, InequalityOperator) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {4, 5, 6};
    EXPECT_TRUE(dq1 != dq2);
}

TEST_F(DequeTest, LessThanOperator) {
    deque<int> dq1 = {1, 2, 3};
    deque<int> dq2 = {1, 2, 4};
    EXPECT_TRUE(dq1 < dq2);
}

TEST_F(DequeTest, GreaterThanOperator) {
    deque<int> dq1 = {1, 2, 4};
    deque<int> dq2 = {1, 2, 3};
    EXPECT_TRUE(dq1 > dq2);
}

TEST_F(DequeTest, LargeNumberOfElements) {
    deque<int> dq;
    const size_t count = 10000;
    for (size_t i = 0; i < count; ++i) {
        dq.push_back(static_cast<int>(i));
    }
    EXPECT_EQ(dq.size(), count);
    EXPECT_EQ(dq.front(), 0);
    EXPECT_EQ(dq.back(), static_cast<int>(count - 1));
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(dq[i], static_cast<int>(i));
    }
}

TEST_F(DequeTest, PushFrontBackAlternating) {
    deque<int> dq;
    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0) {
            dq.push_back(i);
        } else {
            dq.push_front(i);
        }
    }
    EXPECT_EQ(dq.size(), 100);
}

TEST_F(DequeTest, CrossBufferBoundary) {
    deque<int> dq;
    for (int i = 0; i < 1000; ++i) {
        dq.push_back(i);
    }
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(dq[static_cast<size_t>(i)], i);
    }
}

TEST_F(DequeTest, CrossBufferErase) {
    deque<int> dq;
    for (int i = 0; i < 500; ++i) {
        dq.push_back(i);
    }
    dq.erase(dq.begin() + 100, dq.begin() + 400);
    EXPECT_EQ(dq.size(), 200);
    EXPECT_EQ(dq[0], 0);
    EXPECT_EQ(dq[99], 99);
    EXPECT_EQ(dq[100], 400);
}

TEST_F(DequeTest, StringType) {
    deque<string> dq;
    dq.push_back("hello");
    dq.push_front("world");
    dq.emplace_back("foo");
    EXPECT_EQ(dq.size(), 3);
    EXPECT_EQ(dq[0], "world");
    EXPECT_EQ(dq[1], "hello");
    EXPECT_EQ(dq[2], "foo");
}

TEST_F(DequeTest, StringCopy) {
    deque<string> dq1 = {"abc", "def"};
    deque<string> dq2 = dq1;
    EXPECT_EQ(dq2.size(), 2);
    EXPECT_EQ(dq2[0], "abc");
    EXPECT_EQ(dq2[1], "def");
}

TEST_F(DequeTest, StringMove) {
    deque<string> dq1 = {"abc", "def"};
    deque<string> dq2 = move(dq1);
    EXPECT_EQ(dq2.size(), 2);
    EXPECT_EQ(dq2[0], "abc");
}

TEST_F(DequeTest, RangeBasedForLoop) {
    deque<int> dq = {1, 2, 3, 4, 5};
    int sum = 0;
    for (auto val: dq) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST_F(DequeTest, StdSortCompatibility) {
    deque<int> dq = {5, 3, 1, 4, 2};
    sort(dq.begin(), dq.end());
    for (size_t i = 0; i < dq.size(); ++i) {
        EXPECT_EQ(dq[i], static_cast<int>(i + 1));
    }
}

TEST_F(DequeTest, StdFindCompatibility) {
    deque<int> dq = {1, 2, 3, 4, 5};
    auto it = find(dq.begin(), dq.end(), 3);
    EXPECT_NE(it, dq.end());
    EXPECT_EQ(*it, 3);
}

TEST_F(DequeTest, IteratorIncrement) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.begin();
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(DequeTest, IteratorDecrement) {
    deque<int> dq = {1, 2, 3};
    auto it = dq.end();
    --it;
    EXPECT_EQ(*it, 3);
}

TEST_F(DequeTest, IteratorAdvance) {
    deque<int> dq;
    for (int i = 0; i < 1000; ++i) {
        dq.push_back(i);
    }
    auto it = dq.begin();
    it += 500;
    EXPECT_EQ(*it, 500);
    it -= 300;
    EXPECT_EQ(*it, 200);
}

TEST_F(DequeTest, IteratorDistance) {
    deque<int> dq;
    for (int i = 0; i < 100; ++i) {
        dq.push_back(i);
    }
    EXPECT_EQ(dq.end() - dq.begin(), 100);
}

TEST_F(DequeTest, IteratorSubscript) {
    deque<int> dq;
    for (int i = 0; i < 1000; ++i) {
        dq.push_back(i);
    }
    auto it = dq.begin();
    EXPECT_EQ(it[500], 500);
    EXPECT_EQ(it[999], 999);
}

#ifdef NEFORCE_STATE_DEBUG
TEST_F(DequeTest, PopBackOnEmpty) {
    deque<int> dq;
    EXPECT_DEBUG_DEATH(dq.pop_back(), "");
}

TEST_F(DequeTest, PopFrontOnEmpty) {
    deque<int> dq;
    EXPECT_DEBUG_DEATH(dq.pop_front(), "");
}

TEST_F(DequeTest, FrontOnEmpty) {
    deque<int> dq;
    EXPECT_DEBUG_DEATH(ignore = dq.front(), "");
}

TEST_F(DequeTest, BackOnEmpty) {
    deque<int> dq;
    EXPECT_DEBUG_DEATH(ignore = dq.back(), "");
}
#endif

class ListTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ListTest, DefaultConstructor) {
    list<int> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0);
}

TEST_F(ListTest, ConstructorWithSize) {
    list<int> l(5);
    EXPECT_EQ(l.size(), 5);
    EXPECT_FALSE(l.empty());
}

TEST_F(ListTest, ConstructorWithSizeAndValue) {
    list<int> l(5, 42);
    EXPECT_EQ(l.size(), 5);
    for (auto it = l.begin(); it != l.end(); ++it) {
        EXPECT_EQ(*it, 42);
    }
}

TEST_F(ListTest, ConstructorWithZeroSize) {
    list<int> l(0);
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0);
}

TEST_F(ListTest, InitializerListConstructor) {
    list<int> l = {1, 2, 3, 4, 5};
    EXPECT_EQ(l.size(), 5);
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, InitializerListAssignment) {
    list<int> l = {1, 2, 3};
    l = {4, 5, 6, 7};
    EXPECT_EQ(l.size(), 4);
    auto it = l.begin();
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 6);
    EXPECT_EQ(*it++, 7);
}

TEST_F(ListTest, RangeConstructor) {
    vector<int> vec = {10, 20, 30, 40, 50};
    list<int> l(vec.begin(), vec.end());
    EXPECT_EQ(l.size(), 5);
    auto it = l.begin();
    EXPECT_EQ(*it++, 10);
    EXPECT_EQ(*it++, 20);
}

TEST_F(ListTest, RangeConstructorEmpty) {
    vector<int> vec;
    list<int> l(vec.begin(), vec.end());
    EXPECT_TRUE(l.empty());
}

TEST_F(ListTest, CopyConstructor) {
    list<int> l1 = {1, 2, 3, 4, 5};
    list<int> l2(l1);
    EXPECT_EQ(l2.size(), 5);
    auto it1 = l1.begin();
    auto it2 = l2.begin();
    while (it1 != l1.end()) {
        EXPECT_EQ(*it1, *it2);
        ++it1;
        ++it2;
    }
}

TEST_F(ListTest, CopyAssignment) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {4, 5, 6, 7, 8};
    l2 = l1;
    EXPECT_EQ(l2.size(), 3);
    auto it = l2.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
}

TEST_F(ListTest, CopyAssignmentSelf) {
    list<int> l = {1, 2, 3};
    l = l;
    EXPECT_EQ(l.size(), 3);
}

TEST_F(ListTest, MoveConstructor) {
    list<int> l1 = {1, 2, 3, 4, 5};
    list<int> l2(move(l1));
    EXPECT_EQ(l2.size(), 5);
    auto it = l2.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, MoveAssignment) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {4, 5, 6, 7};
    l2 = move(l1);
    EXPECT_EQ(l2.size(), 3);
}

TEST_F(ListTest, MoveAssignmentSelf) {
    list<int> l = {1, 2, 3};
    l = move(l);
    EXPECT_EQ(l.size(), 3);
}

TEST_F(ListTest, BeginEnd) {
    list<int> l = {1, 2, 3};
    auto it = l.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, l.end());
}

TEST_F(ListTest, ConstBeginEnd) {
    const list<int> l = {1, 2, 3};
    auto it = l.begin();
    EXPECT_EQ(*it, 1);
}

TEST_F(ListTest, CbeginCend) {
    list<int> l = {1, 2, 3};
    auto it = l.cbegin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(ListTest, ReverseBeginEnd) {
    list<int> l = {1, 2, 3};
    auto rit = l.rbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
    ++rit;
    EXPECT_EQ(*rit, 1);
    ++rit;
    EXPECT_EQ(rit, l.rend());
}

TEST_F(ListTest, ConstReverseBeginEnd) {
    const list<int> l = {1, 2, 3};
    auto rit = l.rbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(ListTest, Size) {
    list<int> l;
    EXPECT_EQ(l.size(), 0);
    l.push_back(1);
    EXPECT_EQ(l.size(), 1);
    l.push_back(2);
    EXPECT_EQ(l.size(), 2);
}

TEST_F(ListTest, MaxSize) {
    list<int> l;
    EXPECT_GT(l.max_size(), 0);
}

TEST_F(ListTest, Empty) {
    list<int> l;
    EXPECT_TRUE(l.empty());
    l.push_back(1);
    EXPECT_FALSE(l.empty());
    l.pop_front();
    EXPECT_TRUE(l.empty());
}

TEST_F(ListTest, Front) {
    list<int> l = {10, 20, 30};
    EXPECT_EQ(l.front(), 10);
    l.front() = 100;
    EXPECT_EQ(l.front(), 100);
}

TEST_F(ListTest, ConstFront) {
    const list<int> l = {10, 20, 30};
    EXPECT_EQ(l.front(), 10);
}

TEST_F(ListTest, Back) {
    list<int> l = {10, 20, 30};
    EXPECT_EQ(l.back(), 30);
    l.back() = 300;
    EXPECT_EQ(l.back(), 300);
}

TEST_F(ListTest, ConstBack) {
    const list<int> l = {10, 20, 30};
    EXPECT_EQ(l.back(), 30);
}

TEST_F(ListTest, At) {
    list<int> l = {1, 2, 3, 4, 5};
    EXPECT_EQ(l.at(0), 1);
    EXPECT_EQ(l.at(2), 3);
    EXPECT_EQ(l.at(4), 5);
    l.at(2) = 10;
    EXPECT_EQ(l.at(2), 10);
}

TEST_F(ListTest, ConstAt) {
    const list<int> l = {1, 2, 3, 4, 5};
    EXPECT_EQ(l.at(0), 1);
    EXPECT_EQ(l.at(4), 5);
}

TEST_F(ListTest, SubscriptOperator) {
    list<int> l = {1, 2, 3, 4, 5};
    EXPECT_EQ(l[0], 1);
    EXPECT_EQ(l[2], 3);
    l[2] = 10;
    EXPECT_EQ(l[2], 10);
}

TEST_F(ListTest, ConstSubscriptOperator) {
    const list<int> l = {1, 2, 3};
    EXPECT_EQ(l[1], 2);
}

TEST_F(ListTest, PushBack) {
    list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    EXPECT_EQ(l.size(), 3);
    EXPECT_EQ(l.front(), 1);
    EXPECT_EQ(l.back(), 3);
}

TEST_F(ListTest, PushFront) {
    list<int> l;
    l.push_front(1);
    l.push_front(2);
    l.push_front(3);
    EXPECT_EQ(l.size(), 3);
    EXPECT_EQ(l.front(), 3);
    EXPECT_EQ(l.back(), 1);
}

TEST_F(ListTest, PushBackRvalue) {
    list<string> l;
    string s = "hello";
    l.push_back(move(s));
    EXPECT_EQ(l.front(), "hello");
}

TEST_F(ListTest, PushFrontRvalue) {
    list<string> l;
    string s = "world";
    l.push_front(move(s));
    EXPECT_EQ(l.front(), "world");
}

TEST_F(ListTest, PopBack) {
    list<int> l = {1, 2, 3};
    l.pop_back();
    EXPECT_EQ(l.size(), 2);
    EXPECT_EQ(l.back(), 2);
    l.pop_back();
    EXPECT_EQ(l.size(), 1);
    EXPECT_EQ(l.back(), 1);
}

TEST_F(ListTest, PopFront) {
    list<int> l = {1, 2, 3};
    l.pop_front();
    EXPECT_EQ(l.size(), 2);
    EXPECT_EQ(l.front(), 2);
    l.pop_front();
    EXPECT_EQ(l.size(), 1);
    EXPECT_EQ(l.front(), 3);
}

TEST_F(ListTest, EmplaceBack) {
    list<string> l;
    l.emplace_back("hello");
    l.emplace_back("world");
    EXPECT_EQ(l.size(), 2);
    EXPECT_EQ(l.front(), "hello");
    EXPECT_EQ(l.back(), "world");
}

TEST_F(ListTest, EmplaceFront) {
    list<string> l;
    l.emplace_front("hello");
    l.emplace_front("world");
    EXPECT_EQ(l.front(), "world");
    EXPECT_EQ(l.back(), "hello");
}

TEST_F(ListTest, Emplace) {
    list<int> l = {1, 2, 4, 5};
    auto it = l.emplace(++(++l.begin()), 3);
    EXPECT_EQ(l.size(), 5);
    EXPECT_EQ(*it, 3);
    auto check = l.begin();
    EXPECT_EQ(*check++, 1);
    EXPECT_EQ(*check++, 2);
    EXPECT_EQ(*check++, 3);
    EXPECT_EQ(*check++, 4);
    EXPECT_EQ(*check++, 5);
}

TEST_F(ListTest, EmplaceAtBegin) {
    list<int> l = {2, 3, 4};
    l.emplace(l.begin(), 1);
    EXPECT_EQ(l.front(), 1);
}

TEST_F(ListTest, EmplaceAtEnd) {
    list<int> l = {1, 2, 3};
    l.emplace(l.end(), 4);
    EXPECT_EQ(l.back(), 4);
}

TEST_F(ListTest, InsertSingleElement) {
    list<int> l = {1, 2, 4, 5};
    auto it = l.insert(++(++l.begin()), 3);
    EXPECT_EQ(l.size(), 5);
    EXPECT_EQ(*it, 3);
}

TEST_F(ListTest, InsertSingleElementAtBegin) {
    list<int> l = {2, 3, 4};
    l.insert(l.begin(), 1);
    EXPECT_EQ(l.front(), 1);
}

TEST_F(ListTest, InsertSingleElementAtEnd) {
    list<int> l = {1, 2, 3};
    l.insert(l.end(), 4);
    EXPECT_EQ(l.back(), 4);
}

TEST_F(ListTest, InsertRvalue) {
    list<string> l = {"hello", "world"};
    string s = "beautiful";
    l.insert(++l.begin(), move(s));
    EXPECT_EQ(l.size(), 3);
    auto it = l.begin();
    EXPECT_EQ(*it++, "hello");
    EXPECT_EQ(*it++, "beautiful");
    EXPECT_EQ(*it++, "world");
}

TEST_F(ListTest, InsertNValues) {
    list<int> l = {1, 5};
    l.insert(++l.begin(), 3, 9);
    EXPECT_EQ(l.size(), 5);
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 9);
    EXPECT_EQ(*it++, 9);
    EXPECT_EQ(*it++, 9);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, InsertNZeroValues) {
    list<int> l = {1, 2};
    l.insert(l.begin(), 0, 9);
    EXPECT_EQ(l.size(), 2);
}

TEST_F(ListTest, InsertRange) {
    list<int> l = {1, 5};
    vector<int> vals = {2, 3, 4};
    l.insert(++l.begin(), vals.begin(), vals.end());
    EXPECT_EQ(l.size(), 5);
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, InsertEmptyRange) {
    list<int> l = {1, 2, 3};
    vector<int> vals;
    l.insert(l.begin(), vals.begin(), vals.end());
    EXPECT_EQ(l.size(), 3);
}

TEST_F(ListTest, InsertInitializerList) {
    list<int> l = {1, 5};
    l.insert(++l.begin(), {2, 3, 4});
    EXPECT_EQ(l.size(), 5);
}

TEST_F(ListTest, EraseSingleElement) {
    list<int> l = {1, 2, 3, 4, 5};
    auto it = l.erase(++(++l.begin()));
    EXPECT_EQ(l.size(), 4);
    EXPECT_EQ(*it, 4);
}

TEST_F(ListTest, EraseSingleElementAtBegin) {
    list<int> l = {1, 2, 3};
    auto it = l.erase(l.begin());
    EXPECT_EQ(l.size(), 2);
    EXPECT_EQ(l.front(), 2);
    EXPECT_EQ(it, l.begin());
}

TEST_F(ListTest, EraseSingleElementAtEnd) {
    list<int> l = {1, 2, 3};
    auto it = l.erase(--l.end());
    EXPECT_EQ(l.size(), 2);
    EXPECT_EQ(l.back(), 2);
    EXPECT_EQ(it, l.end());
}

TEST_F(ListTest, EraseRange) {
    list<int> l = {1, 2, 3, 4, 5};
    auto it = l.erase(++l.begin(), --l.end());
    EXPECT_EQ(l.size(), 2);
    EXPECT_EQ(l.front(), 1);
    EXPECT_EQ(l.back(), 5);
    EXPECT_EQ(it, --l.end());
}

TEST_F(ListTest, EraseAll) {
    list<int> l = {1, 2, 3};
    auto it = l.erase(l.begin(), l.end());
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(it, l.end());
}

TEST_F(ListTest, AssignNValues) {
    list<int> l = {1, 2, 3, 4, 5};
    l.assign(3, 42);
    EXPECT_EQ(l.size(), 3);
    for (auto val: l) {
        EXPECT_EQ(val, 42);
    }
}

TEST_F(ListTest, AssignRange) {
    list<int> l = {1, 2, 3};
    vector<int> vals = {10, 20, 30, 40};
    l.assign(vals.begin(), vals.end());
    EXPECT_EQ(l.size(), 4);
}

TEST_F(ListTest, AssignInitializerList) {
    list<int> l = {1, 2, 3};
    l.assign({10, 20, 30, 40, 50});
    EXPECT_EQ(l.size(), 5);
}

TEST_F(ListTest, Clear) {
    list<int> l = {1, 2, 3, 4, 5};
    l.clear();
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0);
}

TEST_F(ListTest, Swap) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {4, 5, 6, 7};
    l1.swap(l2);
    EXPECT_EQ(l1.size(), 4);
    EXPECT_EQ(l2.size(), 3);
    EXPECT_EQ(l1.front(), 4);
    EXPECT_EQ(l2.front(), 1);
}

TEST_F(ListTest, SwapSelf) {
    list<int> l = {1, 2, 3};
    l.swap(l);
    EXPECT_EQ(l.size(), 3);
}

TEST_F(ListTest, Transfer) {
    list<int> l1 = {1, 2, 3, 4, 5, 6};
    auto first = ++l1.begin();
    auto last = --(--l1.end());
    l1.transfer(l1.end(), first, last);

    auto it = l1.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 6);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
}

TEST_F(ListTest, TransferSelf) {
    list<int> l = {1, 2, 3, 4, 5};
    auto first = ++l.begin();
    auto last = --l.end();
    l.transfer(l.end(), first, last);
    EXPECT_EQ(l.size(), 5);
}

TEST_F(ListTest, RemoveIf) {
    list<int> l = {1, 2, 3, 4, 5, 6};
    l.remove_if([](int x) { return x % 2 == 0; });
    EXPECT_EQ(l.size(), 3);
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, Remove) {
    list<int> l = {1, 2, 3, 2, 4, 2, 5};
    l.remove(2);
    EXPECT_EQ(l.size(), 4);
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, SpliceWhole) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {4, 5, 6};
    l1.splice(l1.end(), l2);
    EXPECT_EQ(l1.size(), 6);
    EXPECT_TRUE(l2.empty());
    auto it = l1.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 6);
}

TEST_F(ListTest, SpliceSingle) {
    list<int> l1 = {1, 2, 5};
    list<int> l2 = {3, 4};
    auto it = l2.begin();
    l1.splice(++(++l1.begin()), l2, it);
    EXPECT_EQ(l1.size(), 4);
    EXPECT_EQ(l2.size(), 1);
}

TEST_F(ListTest, SpliceRange) {
    list<int> l1 = {1, 5};
    list<int> l2 = {2, 3, 4};
    l1.splice(++l1.begin(), l2, l2.begin(), l2.end());
    EXPECT_EQ(l1.size(), 5);
    EXPECT_TRUE(l2.empty());
}

TEST_F(ListTest, Merge) {
    list<int> l1 = {1, 3, 5, 7};
    list<int> l2 = {2, 4, 6, 8};
    l1.merge(l2);
    EXPECT_EQ(l1.size(), 8);
    EXPECT_TRUE(l2.empty());
    int expected = 1;
    for (auto val: l1) {
        EXPECT_EQ(val, expected++);
    }
}

TEST_F(ListTest, MergeIf) {
    list<int> l1 = {8, 6, 4, 2};
    list<int> l2 = {7, 5, 3, 1};
    l1.merge_if(l2, greater<int>());
    EXPECT_EQ(l1.size(), 8);
    EXPECT_TRUE(l2.empty());
    int expected = 8;
    for (auto val: l1) {
        EXPECT_EQ(val, expected--);
    }
}

TEST_F(ListTest, Reverse) {
    list<int> l = {1, 2, 3, 4, 5};
    l.reverse();
    auto it = l.begin();
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 1);
}

TEST_F(ListTest, ReverseEmpty) {
    list<int> l;
    l.reverse();
    EXPECT_TRUE(l.empty());
}

TEST_F(ListTest, ReverseSingleElement) {
    list<int> l = {42};
    l.reverse();
    EXPECT_EQ(l.front(), 42);
}

TEST_F(ListTest, Unique) {
    list<int> l = {1, 1, 2, 2, 2, 3, 3, 3, 3, 4};
    l.unique();
    EXPECT_EQ(l.size(), 4);
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
}

TEST_F(ListTest, UniqueIf) {
    list<int> l = {1, 1, 2, 2, 3, 3};
    l.unique_if(equal_to<int>());
    EXPECT_EQ(l.size(), 3);
}

TEST_F(ListTest, Sort) {
    list<int> l = {5, 3, 1, 4, 2};
    l.sort();
    auto it = l.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 5);
}

TEST_F(ListTest, SortIf) {
    list<int> l = {1, 2, 3, 4, 5};
    l.sort_if(greater<int>());
    auto it = l.begin();
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 1);
}

TEST_F(ListTest, SortEmpty) {
    list<int> l;
    l.sort();
    EXPECT_TRUE(l.empty());
}

TEST_F(ListTest, SortSingleElement) {
    list<int> l = {42};
    l.sort();
    EXPECT_EQ(l.front(), 42);
}

TEST_F(ListTest, EqualTo) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {1, 2, 3};
    list<int> l3 = {1, 2, 4};
    EXPECT_TRUE(l1.equal_to(l2));
    EXPECT_FALSE(l1.equal_to(l3));
}

TEST_F(ListTest, LessThan) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {1, 2, 4};
    EXPECT_TRUE(l1.less_than(l2));
    EXPECT_FALSE(l2.less_than(l1));
}

TEST_F(ListTest, EqualityOperator) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {1, 2, 3};
    list<int> l3 = {1, 2, 4};
    EXPECT_TRUE(l1 == l2);
    EXPECT_FALSE(l1 == l3);
}

TEST_F(ListTest, InequalityOperator) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {4, 5, 6};
    EXPECT_TRUE(l1 != l2);
}

TEST_F(ListTest, LessThanOperator) {
    list<int> l1 = {1, 2, 3};
    list<int> l2 = {1, 2, 4};
    EXPECT_TRUE(l1 < l2);
}

TEST_F(ListTest, GreaterThanOperator) {
    list<int> l1 = {1, 2, 4};
    list<int> l2 = {1, 2, 3};
    EXPECT_TRUE(l1 > l2);
}

TEST_F(ListTest, LargeNumberOfElements) {
    list<int> l;
    const size_t count = 10000;
    for (size_t i = 0; i < count; ++i) {
        l.push_back(static_cast<int>(i));
    }
    EXPECT_EQ(l.size(), count);
    EXPECT_EQ(l.front(), 0);
    EXPECT_EQ(l.back(), static_cast<int>(count - 1));
}

TEST_F(ListTest, StringType) {
    list<string> l;
    l.push_back("hello");
    l.push_front("world");
    l.emplace_back("foo");
    EXPECT_EQ(l.size(), 3);
    auto it = l.begin();
    EXPECT_EQ(*it++, "world");
    EXPECT_EQ(*it++, "hello");
    EXPECT_EQ(*it++, "foo");
}

TEST_F(ListTest, StringCopy) {
    list<string> l1 = {"abc", "def"};
    list<string> l2 = l1;
    EXPECT_EQ(l2.size(), 2);
    auto it = l2.begin();
    EXPECT_EQ(*it++, "abc");
    EXPECT_EQ(*it++, "def");
}

TEST_F(ListTest, RangeBasedForLoop) {
    list<int> l = {1, 2, 3, 4, 5};
    int sum = 0;
    for (auto val: l) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST_F(ListTest, StdFindCompatibility) {
    list<int> l = {1, 2, 3, 4, 5};
    auto it = find(l.begin(), l.end(), 3);
    EXPECT_NE(it, l.end());
    EXPECT_EQ(*it, 3);
}

TEST_F(ListTest, IteratorIncrementDecrement) {
    list<int> l = {1, 2, 3};
    auto it = l.begin();
    ++it;
    EXPECT_EQ(*it, 2);
    --it;
    EXPECT_EQ(*it, 1);
}

TEST_F(ListTest, ClearOnEmpty) {
    list<int> l;
    l.clear();
    EXPECT_TRUE(l.empty());
}

TEST_F(ListTest, EraseOnEmpty) {
    list<int> l;
    auto it = l.erase(l.begin());
    EXPECT_EQ(it, l.end());
}

#ifndef NDEBUG
TEST_F(ListTest, FrontOnEmpty) {
    list<int> l;
    EXPECT_DEBUG_DEATH(ignore = l.front(), "");
}

TEST_F(ListTest, BackOnEmpty) {
    list<int> l;
    EXPECT_DEBUG_DEATH(ignore = l.back(), "");
}
#endif

TEST_F(ListTest, IteratorEquality) {
    list<int> l = {1, 2, 3};
    auto it1 = l.begin();
    auto it2 = l.begin();
    auto it3 = l.end();
    EXPECT_TRUE(it1 == it2);
    EXPECT_FALSE(it1 == it3);
}

TEST_F(ListTest, IteratorBase) {
    list<int> l = {1, 2, 3};
    auto it = l.begin();
    EXPECT_NE(it.base(), nullptr);
}

TEST_F(ListTest, IteratorContainer) {
    list<int> l;
    auto it = l.begin();
    EXPECT_EQ(it.container(), &l);
}

class VectorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(VectorTest, DefaultConstructor) {
    vector<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0);
    EXPECT_GE(v.capacity(), 1);
}

TEST_F(VectorTest, ConstructorWithSize) {
    vector<int> v(5);
    EXPECT_EQ(v.size(), 5);
    EXPECT_FALSE(v.empty());
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], 0);
    }
}

TEST_F(VectorTest, ConstructorWithSizeAndValue) {
    vector<int> v(5, 42);
    EXPECT_EQ(v.size(), 5);
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], 42);
    }
}

TEST_F(VectorTest, ConstructorWithInt32Size) {
    vector<int> v(static_cast<int32_t>(3), 10);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 10);
}

TEST_F(VectorTest, ConstructorWithInt64Size) {
    vector<int> v(static_cast<int64_t>(4), 20);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[0], 20);
}

TEST_F(VectorTest, ConstructorWithZeroSize) {
    vector<int> v(0);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0);
}

TEST_F(VectorTest, InitializerListConstructor) {
    vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);
}

TEST_F(VectorTest, InitializerListAssignment) {
    vector<int> v = {1, 2, 3};
    v = {4, 5, 6, 7};
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[1], 5);
    EXPECT_EQ(v[2], 6);
    EXPECT_EQ(v[3], 7);
}

TEST_F(VectorTest, InitializerListAssignmentSmaller) {
    vector<int> v = {1, 2, 3, 4, 5};
    v = {10, 20};
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
}

TEST_F(VectorTest, InitializerListAssignmentLarger) {
    vector<int> v = {1, 2};
    v = {10, 20, 30, 40};
    EXPECT_EQ(v.size(), 4);
}

TEST_F(VectorTest, RangeConstructor) {
    vector<int> vec = {10, 20, 30, 40, 50};
    vector<int> v(vec.begin(), vec.end());
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[4], 50);
}

TEST_F(VectorTest, RangeConstructorWithList) {
    list<int> lst = {1, 2, 3};
    vector<int> v(lst.begin(), lst.end());
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[2], 3);
}

TEST_F(VectorTest, RangeConstructorWithIteratorAndSize) {
    vector<int> vec = {1, 2, 3, 4, 5};
    vector<int> v(vec.begin(), 3);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[2], 3);
}

TEST_F(VectorTest, RangeConstructorEmpty) {
    vector<int> vec;
    vector<int> v(vec.begin(), vec.end());
    EXPECT_TRUE(v.empty());
}

TEST_F(VectorTest, CopyConstructor) {
    vector<int> v1 = {1, 2, 3, 4, 5};
    vector<int> v2(v1);
    EXPECT_EQ(v2.size(), 5);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[4], 5);
}

TEST_F(VectorTest, CopyAssignment) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6, 7, 8};
    v2 = v1;
    EXPECT_EQ(v2.size(), 3);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[2], 3);
}

TEST_F(VectorTest, CopyAssignmentSelf) {
    vector<int> v = {1, 2, 3};
    v = v;
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
}

TEST_F(VectorTest, MoveConstructor) {
    vector<int> v1 = {1, 2, 3, 4, 5};
    vector<int> v2(move(v1));
    EXPECT_EQ(v2.size(), 5);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[4], 5);
}

TEST_F(VectorTest, MoveAssignment) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6, 7};
    v2 = move(v1);
    EXPECT_EQ(v2.size(), 3);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[2], 3);
}

TEST_F(VectorTest, MoveAssignmentSelf) {
    vector<int> v = {1, 2, 3};
    v = move(v);
    EXPECT_EQ(v.size(), 3);
}

TEST_F(VectorTest, BeginEnd) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto it = v.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    auto end_it = v.end();
    EXPECT_EQ(end_it - v.begin(), 5);
}

TEST_F(VectorTest, ConstBeginEnd) {
    const vector<int> v = {1, 2, 3, 4, 5};
    auto it = v.begin();
    EXPECT_EQ(*it, 1);
}

TEST_F(VectorTest, CbeginCend) {
    vector<int> v = {1, 2, 3};
    auto it = v.cbegin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(VectorTest, ReverseBeginEnd) {
    vector<int> v = {1, 2, 3};
    auto rit = v.rbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
}

TEST_F(VectorTest, ConstReverseBeginEnd) {
    const vector<int> v = {1, 2, 3};
    auto rit = v.rbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(VectorTest, Size) {
    vector<int> v;
    EXPECT_EQ(v.size(), 0);
    v.push_back(1);
    EXPECT_EQ(v.size(), 1);
    v.push_back(2);
    EXPECT_EQ(v.size(), 2);
}

TEST_F(VectorTest, MaxSize) {
    vector<int> v;
    EXPECT_GT(v.max_size(), 0);
}

TEST_F(VectorTest, Capacity) {
    vector<int> v;
    EXPECT_GE(v.capacity(), 1);
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100);
}

TEST_F(VectorTest, Empty) {
    vector<int> v;
    EXPECT_TRUE(v.empty());
    v.push_back(1);
    EXPECT_FALSE(v.empty());
    v.pop_back();
    EXPECT_TRUE(v.empty());
}

TEST_F(VectorTest, Data) {
    vector<int> v = {1, 2, 3, 4, 5};
    int* ptr = v.data();
    EXPECT_EQ(*ptr, 1);
    EXPECT_EQ(*(ptr + 4), 5);
}

TEST_F(VectorTest, ConstData) {
    const vector<int> v = {1, 2, 3};
    const int* ptr = v.data();
    EXPECT_EQ(*ptr, 1);
}

TEST_F(VectorTest, View) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto view = v.view();
    EXPECT_EQ(view.size(), 5);
    EXPECT_EQ(view[0], 1);
}

TEST_F(VectorTest, ConstView) {
    const vector<int> v = {1, 2, 3};
    auto view = v.view();
    EXPECT_EQ(view.size(), 3);
}

TEST_F(VectorTest, ViewWithOffsetAndCount) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto view = v.view(1, 3);
    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view[0], 2);
    EXPECT_EQ(view[2], 4);
}

TEST_F(VectorTest, ConstViewWithOffsetAndCount) {
    const vector<int> v = {1, 2, 3, 4, 5};
    auto view = v.view(2, 2);
    EXPECT_EQ(view.size(), 2);
    EXPECT_EQ(view[0], 3);
}

TEST_F(VectorTest, ViewWithNpos) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto view = v.view(2, vector<int>::npos);
    EXPECT_EQ(view.size(), 3);
}

TEST_F(VectorTest, Front) {
    vector<int> v = {10, 20, 30};
    EXPECT_EQ(v.front(), 10);
    v.front() = 100;
    EXPECT_EQ(v.front(), 100);
}

TEST_F(VectorTest, ConstFront) {
    const vector<int> v = {10, 20, 30};
    EXPECT_EQ(v.front(), 10);
}

TEST_F(VectorTest, Back) {
    vector<int> v = {10, 20, 30};
    EXPECT_EQ(v.back(), 30);
    v.back() = 300;
    EXPECT_EQ(v.back(), 300);
}

TEST_F(VectorTest, ConstBack) {
    const vector<int> v = {10, 20, 30};
    EXPECT_EQ(v.back(), 30);
}

TEST_F(VectorTest, At) {
    vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(v.at(0), 1);
    EXPECT_EQ(v.at(4), 5);
    v.at(2) = 10;
    EXPECT_EQ(v.at(2), 10);
}

TEST_F(VectorTest, ConstAt) {
    const vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(v.at(0), 1);
    EXPECT_EQ(v.at(4), 5);
}

TEST_F(VectorTest, SubscriptOperator) {
    vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_EQ(v[0], 1);
    v[2] = 10;
    EXPECT_EQ(v[2], 10);
}

TEST_F(VectorTest, ConstSubscriptOperator) {
    const vector<int> v = {1, 2, 3};
    EXPECT_EQ(v[1], 2);
}

TEST_F(VectorTest, Reserve) {
    vector<int> v;
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100);
    EXPECT_EQ(v.size(), 0);
}

TEST_F(VectorTest, ReserveSmallerThanCapacity) {
    vector<int> v(50);
    size_t cap = v.capacity();
    v.reserve(30);
    EXPECT_EQ(v.capacity(), cap);
}

TEST_F(VectorTest, ResizeSmaller) {
    vector<int> v = {1, 2, 3, 4, 5};
    v.resize(3);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[2], 3);
}

TEST_F(VectorTest, ResizeLarger) {
    vector<int> v = {1, 2, 3};
    v.resize(5, 42);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[3], 42);
    EXPECT_EQ(v[4], 42);
}

TEST_F(VectorTest, ResizeLargerDefault) {
    vector<int> v = {1, 2, 3};
    v.resize(5);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[3], 0);
    EXPECT_EQ(v[4], 0);
}

TEST_F(VectorTest, PushBack) {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST_F(VectorTest, PushBackRvalue) {
    vector<string> v;
    string s = "hello";
    v.push_back(move(s));
    EXPECT_EQ(v[0], "hello");
}

TEST_F(VectorTest, PopBack) {
    vector<int> v = {1, 2, 3};
    v.pop_back();
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(v.back(), 2);
}

TEST_F(VectorTest, PopBackValue) {
    vector<int> v = {1, 2, 3};
    int val = v.pop_back_v();
    EXPECT_EQ(val, 3);
    EXPECT_EQ(v.size(), 2);
}

TEST_F(VectorTest, EmplaceBack) {
    vector<string> v;
    v.emplace_back("hello");
    v.emplace_back("world");
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(v[0], "hello");
    EXPECT_EQ(v[1], "world");
}

TEST_F(VectorTest, Emplace) {
    vector<int> v = {1, 2, 4, 5};
    v.emplace(v.begin() + 2, 3);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[2], 3);
}

TEST_F(VectorTest, EmplaceAtEnd) {
    vector<int> v = {1, 2, 3};
    v.emplace(v.end(), 4);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[3], 4);
}

TEST_F(VectorTest, InsertSingleElement) {
    vector<int> v = {1, 2, 4, 5};
    auto it = v.insert(v.begin() + 2, 3);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(*it, 3);
}

TEST_F(VectorTest, InsertSingleElementAtBegin) {
    vector<int> v = {2, 3, 4};
    auto it = v.insert(v.begin(), 1);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(it, v.begin());
}

TEST_F(VectorTest, InsertSingleElementAtEnd) {
    vector<int> v = {1, 2, 3};
    auto it = v.insert(v.end(), 4);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[3], 4);
}

TEST_F(VectorTest, InsertRvalue) {
    vector<string> v = {"hello", "world"};
    string s = "beautiful";
    v.insert(v.begin() + 1, move(s));
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[1], "beautiful");
}

TEST_F(VectorTest, InsertDefault) {
    vector<int> v = {1, 2, 3};
    auto it = v.insert(v.begin() + 1);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[1], 0);
}

TEST_F(VectorTest, InsertNValues) {
    vector<int> v = {1, 5};
    v.insert(v.begin() + 1, 3, 9);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 9);
    EXPECT_EQ(v[2], 9);
    EXPECT_EQ(v[3], 9);
    EXPECT_EQ(v[4], 5);
}

TEST_F(VectorTest, InsertNZeroValues) {
    vector<int> v = {1, 2};
    v.insert(v.begin(), 0, 9);
    EXPECT_EQ(v.size(), 2);
}

TEST_F(VectorTest, InsertRange) {
    vector<int> v = {1, 5};
    vector<int> vals = {2, 3, 4};
    v.insert(v.begin() + 1, vals.begin(), vals.end());
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);
}

TEST_F(VectorTest, InsertRangeWithList) {
    vector<int> v = {1, 5};
    list<int> lst = {2, 3, 4};
    v.insert(v.begin() + 1, lst.begin(), lst.end());
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[1], 2);
}

TEST_F(VectorTest, InsertInitializerList) {
    vector<int> v = {1, 5};
    v.insert(v.begin() + 1, {2, 3, 4});
    EXPECT_EQ(v.size(), 5);
}

TEST_F(VectorTest, EraseSingleElement) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto it = v.erase(v.begin() + 2);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[2], 4);
    EXPECT_EQ(*it, 4);
}

TEST_F(VectorTest, EraseSingleElementAtEnd) {
    vector<int> v = {1, 2, 3};
    auto it = v.erase(v.end() - 1);
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(it, v.end());
}

TEST_F(VectorTest, EraseRange) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto it = v.erase(v.begin() + 1, v.begin() + 4);
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 5);
    EXPECT_EQ(it, v.begin() + 1);
}

TEST_F(VectorTest, EraseAll) {
    vector<int> v = {1, 2, 3};
    auto it = v.erase(v.begin(), v.end());
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(it, v.end());
}

TEST_F(VectorTest, ShrinkToFit) {
    vector<int> v;
    v.reserve(100);
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    size_t old_cap = v.capacity();
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());
}

TEST_F(VectorTest, ShrinkToFitEmpty) {
    vector<int> v;
    v.shrink_to_fit();
    EXPECT_TRUE(v.empty());
}

TEST_F(VectorTest, Clear) {
    vector<int> v = {1, 2, 3, 4, 5};
    size_t cap = v.capacity();
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(v.capacity(), cap);
}

TEST_F(VectorTest, Swap) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6, 7};
    v1.swap(v2);
    EXPECT_EQ(v1.size(), 4);
    EXPECT_EQ(v2.size(), 3);
    EXPECT_EQ(v1[0], 4);
    EXPECT_EQ(v2[0], 1);
}

TEST_F(VectorTest, SwapSelf) {
    vector<int> v = {1, 2, 3};
    v.swap(v);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
}

TEST_F(VectorTest, AssignNValues) {
    vector<int> v = {1, 2, 3, 4, 5};
    v.assign(3, 42);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 42);
    EXPECT_EQ(v[1], 42);
    EXPECT_EQ(v[2], 42);
}

TEST_F(VectorTest, AssignNValuesLarger) {
    vector<int> v = {1, 2, 3};
    v.assign(10, 42);
    EXPECT_EQ(v.size(), 10);
}

TEST_F(VectorTest, AssignRange) {
    vector<int> v = {1, 2, 3};
    vector<int> vals = {10, 20, 30, 40};
    v.assign(vals.begin(), vals.end());
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[3], 40);
}

TEST_F(VectorTest, AssignInitializerList) {
    vector<int> v = {1, 2, 3};
    v.assign({10, 20, 30, 40, 50});
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[4], 50);
}

TEST_F(VectorTest, EqualTo) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {1, 2, 3};
    vector<int> v3 = {1, 2, 4};
    EXPECT_TRUE(v1.equal_to(v2));
    EXPECT_FALSE(v1.equal_to(v3));
}

TEST_F(VectorTest, LessThan) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {1, 2, 4};
    EXPECT_TRUE(v1.less_than(v2));
    EXPECT_FALSE(v2.less_than(v1));
}

TEST_F(VectorTest, EqualityOperator) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {1, 2, 3};
    vector<int> v3 = {1, 2, 4};
    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
}

TEST_F(VectorTest, InequalityOperator) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6};
    EXPECT_TRUE(v1 != v2);
}

TEST_F(VectorTest, LessThanOperator) {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {1, 2, 4};
    EXPECT_TRUE(v1 < v2);
}

TEST_F(VectorTest, GreaterThanOperator) {
    vector<int> v1 = {1, 2, 4};
    vector<int> v2 = {1, 2, 3};
    EXPECT_TRUE(v1 > v2);
}

TEST_F(VectorTest, LargeNumberOfElements) {
    vector<int> v;
    const size_t count = 10000;
    for (size_t i = 0; i < count; ++i) {
        v.push_back(static_cast<int>(i));
    }
    EXPECT_EQ(v.size(), count);
    EXPECT_EQ(v.front(), 0);
    EXPECT_EQ(v.back(), static_cast<int>(count - 1));
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(v[i], static_cast<int>(i));
    }
}

TEST_F(VectorTest, StringType) {
    vector<string> v;
    v.push_back("hello");
    v.emplace_back("world");
    v.push_back("foo");
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], "hello");
    EXPECT_EQ(v[1], "world");
    EXPECT_EQ(v[2], "foo");
}

TEST_F(VectorTest, StringCopy) {
    vector<string> v1 = {"abc", "def"};
    vector<string> v2 = v1;
    EXPECT_EQ(v2.size(), 2);
    EXPECT_EQ(v2[0], "abc");
    EXPECT_EQ(v2[1], "def");
}

TEST_F(VectorTest, RangeBasedForLoop) {
    vector<int> v = {1, 2, 3, 4, 5};
    int sum = 0;
    for (auto val: v) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST_F(VectorTest, StdSortCompatibility) {
    vector<int> v = {5, 3, 1, 4, 2};
    sort(v.begin(), v.end());
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], static_cast<int>(i + 1));
    }
}

TEST_F(VectorTest, StdFindCompatibility) {
    vector<int> v = {1, 2, 3, 4, 5};
    auto it = find(v.begin(), v.end(), 3);
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 3);
}

TEST_F(VectorTest, IteratorIncrement) {
    vector<int> v = {1, 2, 3};
    auto it = v.begin();
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(VectorTest, IteratorDecrement) {
    vector<int> v = {1, 2, 3};
    auto it = v.end();
    --it;
    EXPECT_EQ(*it, 3);
}

TEST_F(VectorTest, IteratorAdvance) {
    vector<int> v;
    for (int i = 0; i < 1000; ++i) {
        v.push_back(i);
    }
    auto it = v.begin();
    it += 500;
    EXPECT_EQ(*it, 500);
    it -= 300;
    EXPECT_EQ(*it, 200);
}

TEST_F(VectorTest, IteratorDistance) {
    vector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    EXPECT_EQ(v.end() - v.begin(), 100);
}

TEST_F(VectorTest, IteratorSubscript) {
    vector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    auto it = v.begin();
    EXPECT_EQ(it[50], 50);
}

TEST_F(VectorTest, IteratorBase) {
    vector<int> v = {1, 2, 3};
    auto it = v.begin();
    EXPECT_EQ(*(it.base()), 1);
}

TEST_F(VectorTest, IteratorContainer) {
    vector<int> v;
    auto it = v.begin();
    EXPECT_EQ(it.container(), &v);
}

TEST_F(VectorTest, Npos) { EXPECT_EQ(vector<int>::npos, static_cast<size_t>(-1)); }

#ifdef NEFORCE_STATE_DEBUG
TEST_F(VectorTest, PopBackOnEmpty) {
    vector<int> v;
    EXPECT_DEBUG_DEATH(v.pop_back(), "");
}

TEST_F(VectorTest, FrontOnEmpty) {
    vector<int> v;
    EXPECT_DEBUG_DEATH(ignore = v.front(), "");
}

TEST_F(VectorTest, BackOnEmpty) {
    vector<int> v;
    EXPECT_DEBUG_DEATH(ignore = v.back(), "");
}

TEST_F(VectorTest, AtOutOfRange) {
    vector<int> v = {1, 2, 3};
    EXPECT_DEBUG_DEATH(ignore = v.at(3), "");
}

TEST_F(VectorTest, SubscriptOutOfRange) {
    vector<int> v = {1, 2, 3};
    EXPECT_DEBUG_DEATH(ignore = v[3], "");
}
#endif

TEST_F(VectorTest, ByteVector) {
    byte_vector bv;
    bv.push_back(byte_t{0x01});
    bv.push_back(byte_t{0xFF});
    EXPECT_EQ(bv.size(), 2);
}

class MapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MapTest, DefaultConstructor) {
    map<int, string> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(MapTest, ConstructorWithCompare) {
    map<int, string, greater<int>> m;
    EXPECT_TRUE(m.empty());
    m.insert({3, "three"});
    m.insert({1, "one"});
    m.insert({2, "two"});
    auto it = m.begin();
    EXPECT_EQ(it->first, 3);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 1);
}

TEST_F(MapTest, InitializerListConstructor) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
    EXPECT_EQ(m[2], "two");
    EXPECT_EQ(m[3], "three");
}

TEST_F(MapTest, InitializerListConstructorWithCompare) {
    map<int, string, greater<int>> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.size(), 2);
}

TEST_F(MapTest, InitializerListAssignment) {
    map<int, string> m;
    m = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    map<int, string> m(vec.begin(), vec.end());
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, RangeConstructorWithCompare) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    map<int, string, greater<int>> m(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(m.size(), 2);
}

TEST_F(MapTest, CopyConstructor) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}, {3, "three"}};
    map<int, string> m2(m1);
    EXPECT_EQ(m2.size(), 3);
    EXPECT_EQ(m2[1], "one");
    EXPECT_EQ(m2[2], "two");
    EXPECT_EQ(m2[3], "three");
}

TEST_F(MapTest, CopyAssignment) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}};
    map<int, string> m2 = {{3, "three"}, {4, "four"}, {5, "five"}};
    m2 = m1;
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(MapTest, CopyAssignmentSelf) {
    map<int, string> m = {{1, "one"}, {2, "two"}};
    m = m;
    EXPECT_EQ(m.size(), 2);
}

TEST_F(MapTest, MoveConstructor) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}, {3, "three"}};
    map<int, string> m2(move(m1));
    EXPECT_EQ(m2.size(), 3);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(MapTest, MoveAssignment) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}};
    map<int, string> m2 = {{3, "three"}};
    m2 = move(m1);
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(MapTest, MoveAssignmentSelf) {
    map<int, string> m = {{1, "one"}};
    m = move(m);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(MapTest, BeginEnd) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = m.begin();
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 3);
    ++it;
    EXPECT_EQ(it, m.end());
}

TEST_F(MapTest, ConstBeginEnd) {
    const map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.begin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(MapTest, CbeginCend) {
    map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.cbegin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(MapTest, ReverseBeginEnd) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto rit = m.rbegin();
    EXPECT_EQ(rit->first, 3);
    ++rit;
    EXPECT_EQ(rit->first, 2);
    ++rit;
    EXPECT_EQ(rit->first, 1);
    ++rit;
    EXPECT_EQ(rit, m.rend());
}

TEST_F(MapTest, ConstReverseBeginEnd) {
    const map<int, string> m = {{1, "one"}, {2, "two"}};
    auto rit = m.rbegin();
    EXPECT_EQ(rit->first, 2);
}

TEST_F(MapTest, CrbeginCrend) {
    map<int, string> m = {{1, "one"}, {2, "two"}};
    auto rit = m.crbegin();
    EXPECT_EQ(rit->first, 2);
}

TEST_F(MapTest, Size) {
    map<int, int> m;
    EXPECT_EQ(m.size(), 0);
    m.insert({1, 10});
    EXPECT_EQ(m.size(), 1);
    m.insert({2, 20});
    EXPECT_EQ(m.size(), 2);
}

TEST_F(MapTest, MaxSize) {
    map<int, int> m;
    EXPECT_GT(m.max_size(), 0);
}

TEST_F(MapTest, Empty) {
    map<int, int> m;
    EXPECT_TRUE(m.empty());
    m.insert({1, 10});
    EXPECT_FALSE(m.empty());
    m.erase(1);
    EXPECT_TRUE(m.empty());
}

TEST_F(MapTest, KeyComp) {
    map<int, string> m;
    auto comp = m.key_comp();
    EXPECT_TRUE(comp(1, 2));
    EXPECT_FALSE(comp(2, 1));
    EXPECT_FALSE(comp(1, 1));
}

TEST_F(MapTest, ValueComp) {
    map<int, string> m;
    auto comp = m.value_comp();
    pair<const int, string> p1(1, "a");
    pair<const int, string> p2(2, "b");
    EXPECT_TRUE(comp(p1, p2));
}

TEST_F(MapTest, InsertValue) {
    map<int, string> m;
    auto result = m.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
}

TEST_F(MapTest, InsertDuplicateKey) {
    map<int, string> m;
    m.insert({1, "one"});
    auto result = m.insert({1, "uno"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, InsertRvalue) {
    map<int, string> m;
    pair<int, string> p(1, "one");
    m.insert(move(p));
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, InsertWithHint) {
    map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.insert(m.begin(), {2, "two"});
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(m.size(), 3);
}

TEST_F(MapTest, InsertWithHintRvalue) {
    map<int, string> m = {{1, "one"}, {3, "three"}};
    pair<int, string> p(2, "two");
    auto it = m.insert(m.begin(), move(p));
    EXPECT_EQ(it->first, 2);
}

TEST_F(MapTest, InsertRange) {
    map<int, string> m;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    m.insert(vec.begin(), vec.end());
    EXPECT_EQ(m.size(), 3);
}

TEST_F(MapTest, Emplace) {
    map<int, string> m;
    auto result = m.emplace(1, "one");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, EmplaceDuplicate) {
    map<int, string> m;
    m.emplace(1, "one");
    auto result = m.emplace(1, "uno");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, EmplaceHint) {
    map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.emplace_hint(m.begin(), 2, "two");
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(m.size(), 3);
}

TEST_F(MapTest, EraseByIterator) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = m.find(2);
    m.erase(it);
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m.find(2), m.end());
}

TEST_F(MapTest, EraseByKey) {
    map<int, string> m = {{1, "one"}, {2, "two"}};
    size_t count = m.erase(1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(MapTest, EraseByNonExistentKey) {
    map<int, string> m = {{1, "one"}};
    size_t count = m.erase(99);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(MapTest, EraseRange) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
    auto first = m.find(2);
    auto last = m.find(4);
    m.erase(first, last);
    EXPECT_EQ(m.size(), 2);
    EXPECT_NE(m.find(1), m.end());
    EXPECT_NE(m.find(4), m.end());
}

TEST_F(MapTest, Clear) {
    map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    m.clear();
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(MapTest, Find) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = m.find(2);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, "two");
}

TEST_F(MapTest, FindNonExistent) {
    map<int, string> m = {{1, "one"}};
    auto it = m.find(99);
    EXPECT_EQ(it, m.end());
}

TEST_F(MapTest, ConstFind) {
    const map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    EXPECT_NE(it, m.end());
}

TEST_F(MapTest, Count) {
    map<int, string> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.count(1), 1);
    EXPECT_EQ(m.count(99), 0);
}

TEST_F(MapTest, LowerBound) {
    map<int, string> m = {{1, "one"}, {3, "three"}, {5, "five"}};
    auto it = m.lower_bound(3);
    EXPECT_EQ(it->first, 3);
    it = m.lower_bound(4);
    EXPECT_EQ(it->first, 5);
    it = m.lower_bound(6);
    EXPECT_EQ(it, m.end());
}

TEST_F(MapTest, ConstLowerBound) {
    const map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.lower_bound(2);
    EXPECT_EQ(it->first, 3);
}

TEST_F(MapTest, UpperBound) {
    map<int, string> m = {{1, "one"}, {3, "three"}, {5, "five"}};
    auto it = m.upper_bound(3);
    EXPECT_EQ(it->first, 5);
    it = m.upper_bound(5);
    EXPECT_EQ(it, m.end());
}

TEST_F(MapTest, ConstUpperBound) {
    const map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.upper_bound(1);
    EXPECT_EQ(it->first, 3);
}

TEST_F(MapTest, EqualRange) {
    map<int, string> m = {{1, "one"}, {3, "three"}, {5, "five"}};
    auto range = m.equal_range(3);
    EXPECT_EQ(range.first->first, 3);
    EXPECT_EQ(range.second->first, 5);
}

TEST_F(MapTest, ConstEqualRange) {
    const map<int, string> m = {{1, "one"}, {3, "three"}};
    auto range = m.equal_range(1);
    EXPECT_EQ(range.first->first, 1);
}

TEST_F(MapTest, SubscriptOperator) {
    map<int, string> m;
    m[1] = "one";
    EXPECT_EQ(m[1], "one");
    m[1] = "uno";
    EXPECT_EQ(m[1], "uno");
}

TEST_F(MapTest, SubscriptOperatorInsertDefault) {
    map<int, int> m;
    EXPECT_EQ(m[1], 0);
    m[1] = 42;
    EXPECT_EQ(m[1], 42);
}

TEST_F(MapTest, SubscriptOperatorRvalueKey) {
    map<int, string> m;
    m[1] = "one";
    EXPECT_EQ(m[1], "one");
}

TEST_F(MapTest, At) {
    map<int, string> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.at(1), "one");
    m.at(1) = "uno";
    EXPECT_EQ(m.at(1), "uno");
}

TEST_F(MapTest, ConstAt) {
    const map<int, string> m = {{1, "one"}};
    EXPECT_EQ(m.at(1), "one");
}

TEST_F(MapTest, Swap) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}};
    map<int, string> m2 = {{3, "three"}, {4, "four"}, {5, "five"}};
    m1.swap(m2);
    EXPECT_EQ(m1.size(), 3);
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(MapTest, EqualTo) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}};
    map<int, string> m2 = {{1, "one"}, {2, "two"}};
    map<int, string> m3 = {{1, "one"}, {2, "deux"}};
    EXPECT_TRUE(m1.equal_to(m2));
    EXPECT_FALSE(m1.equal_to(m3));
}

TEST_F(MapTest, LessThan) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}};
    map<int, string> m2 = {{1, "one"}, {3, "three"}};
    EXPECT_TRUE(m1.less_than(m2));
    EXPECT_FALSE(m2.less_than(m1));
}

TEST_F(MapTest, EqualityOperator) {
    map<int, string> m1 = {{1, "one"}, {2, "two"}};
    map<int, string> m2 = {{1, "one"}, {2, "two"}};
    map<int, string> m3 = {{1, "one"}};
    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 == m3);
}

TEST_F(MapTest, InequalityOperator) {
    map<int, string> m1 = {{1, "one"}};
    map<int, string> m2 = {{2, "two"}};
    EXPECT_TRUE(m1 != m2);
}

TEST_F(MapTest, LessThanOperator) {
    map<int, string> m1 = {{1, "one"}};
    map<int, string> m2 = {{2, "two"}};
    EXPECT_TRUE(m1 < m2);
}

TEST_F(MapTest, GreaterThanOperator) {
    map<int, string> m1 = {{2, "two"}};
    map<int, string> m2 = {{1, "one"}};
    EXPECT_TRUE(m1 > m2);
}

TEST_F(MapTest, StringKey) {
    map<string, int> m;
    m["hello"] = 1;
    m["world"] = 2;
    EXPECT_EQ(m["hello"], 1);
    EXPECT_EQ(m["world"], 2);
    EXPECT_EQ(m.size(), 2);
}

TEST_F(MapTest, LargeInsert) {
    map<int, int> m;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        m.insert({i, i * 10});
    }
    EXPECT_EQ(m.size(), count);
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(m[i], i * 10);
    }
}

TEST_F(MapTest, OrderPreservation) {
    map<int, string> m;
    m.insert({5, "five"});
    m.insert({1, "one"});
    m.insert({3, "three"});
    m.insert({2, "two"});
    m.insert({4, "four"});
    int expected = 1;
    for (const auto& p: m) {
        EXPECT_EQ(p.first, expected);
        ++expected;
    }
}

TEST_F(MapTest, RangeBasedForLoop) {
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    int sum = 0;
    for (const auto& p: m) {
        sum += p.first;
    }
    EXPECT_EQ(sum, 6);
}

class PriorityQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PriorityQueueTest, DefaultConstructor) {
    priority_queue<int> pq;
    EXPECT_TRUE(pq.empty());
    EXPECT_EQ(pq.size(), 0);
}

TEST_F(PriorityQueueTest, ConstructorWithCompare) {
    priority_queue<int, vector<int>, greater<int>> pq;
    EXPECT_TRUE(pq.empty());
}

TEST_F(PriorityQueueTest, RangeConstructor) {
    vector<int> vec = {1, 5, 3, 4, 2};
    priority_queue<int> pq(vec.begin(), vec.end());
    EXPECT_EQ(pq.size(), 5);
    EXPECT_EQ(pq.top(), 5);
}

TEST_F(PriorityQueueTest, RangeConstructorWithCompare) {
    vector<int> vec = {1, 5, 3, 4, 2};
    priority_queue<int, vector<int>, greater<int>> pq(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(pq.size(), 5);
    EXPECT_EQ(pq.top(), 1);
}

TEST_F(PriorityQueueTest, RangeConstructorWithSequence) {
    vector<int> vec = {5, 3, 1};
    vector<int> seq = {2, 4};
    priority_queue<int> pq(vec.begin(), vec.end(), seq);
    EXPECT_GE(pq.size(), 5);
}

TEST_F(PriorityQueueTest, RangeConstructorWithCompareAndSequence) {
    vector<int> vec = {1, 2};
    vector<int> seq = {5, 3};
    priority_queue<int, vector<int>, greater<int>> pq(vec.begin(), vec.end(), greater<int>(), seq);
    EXPECT_GE(pq.size(), 4);
}

TEST_F(PriorityQueueTest, Empty) {
    priority_queue<int> pq;
    EXPECT_TRUE(pq.empty());
    pq.push(1);
    EXPECT_FALSE(pq.empty());
}

TEST_F(PriorityQueueTest, Size) {
    priority_queue<int> pq;
    EXPECT_EQ(pq.size(), 0);
    pq.push(1);
    EXPECT_EQ(pq.size(), 1);
    pq.push(2);
    EXPECT_EQ(pq.size(), 2);
    pq.pop();
    EXPECT_EQ(pq.size(), 1);
}

TEST_F(PriorityQueueTest, Top) {
    priority_queue<int> pq;
    pq.push(10);
    pq.push(30);
    pq.push(20);
    EXPECT_EQ(pq.top(), 30);
}

TEST_F(PriorityQueueTest, TopMinHeap) {
    priority_queue<int, vector<int>, greater<int>> pq;
    pq.push(10);
    pq.push(30);
    pq.push(20);
    EXPECT_EQ(pq.top(), 10);
}

TEST_F(PriorityQueueTest, PushCopy) {
    priority_queue<int> pq;
    int val = 42;
    pq.push(val);
    EXPECT_EQ(pq.top(), 42);
}

TEST_F(PriorityQueueTest, PushMove) {
    priority_queue<string> pq;
    string s = "hello";
    pq.push(move(s));
    EXPECT_EQ(pq.top(), "hello");
}

TEST_F(PriorityQueueTest, PushMultiple) {
    priority_queue<int> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    pq.push(7);
    EXPECT_EQ(pq.top(), 9);
}

TEST_F(PriorityQueueTest, Pop) {
    priority_queue<int> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    pq.push(7);
    EXPECT_EQ(pq.top(), 9);
    pq.pop();
    EXPECT_EQ(pq.top(), 7);
    pq.pop();
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
    pq.pop();
    EXPECT_EQ(pq.top(), 1);
}

TEST_F(PriorityQueueTest, PopMinHeap) {
    priority_queue<int, vector<int>, greater<int>> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    pq.push(7);
    EXPECT_EQ(pq.top(), 1);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
    pq.pop();
    EXPECT_EQ(pq.top(), 5);
}

TEST_F(PriorityQueueTest, Emplace) {
    priority_queue<string> pq;
    pq.emplace("world");
    pq.emplace("hello");
    pq.emplace("zebra");
    EXPECT_EQ(pq.top(), "zebra");
}

TEST_F(PriorityQueueTest, EmplaceMultipleArgs) {
    priority_queue<string> pq;
    pq.emplace(3, 'a');
    pq.emplace(1, 'b');
    pq.emplace(5, 'c');
    EXPECT_EQ(pq.top(), "ccccc");
}

TEST_F(PriorityQueueTest, Swap) {
    priority_queue<int> pq1;
    pq1.push(1);
    pq1.push(2);
    pq1.push(3);
    priority_queue<int> pq2;
    pq2.push(10);
    pq2.push(20);
    pq1.swap(pq2);
    EXPECT_EQ(pq1.size(), 2);
    EXPECT_EQ(pq1.top(), 20);
    EXPECT_EQ(pq2.size(), 3);
    EXPECT_EQ(pq2.top(), 3);
}

TEST_F(PriorityQueueTest, EqualTo) {
    priority_queue<int> pq1;
    pq1.push(1);
    pq1.push(2);
    pq1.push(3);
    priority_queue<int> pq2;
    pq2.push(1);
    pq2.push(2);
    pq2.push(3);
    priority_queue<int> pq3;
    pq3.push(4);
    pq3.push(5);
    EXPECT_TRUE(pq1.equal_to(pq2));
    EXPECT_FALSE(pq1.equal_to(pq3));
}

TEST_F(PriorityQueueTest, LessThan) {
    priority_queue<int> pq1;
    pq1.push(1);
    pq1.push(2);
    priority_queue<int> pq2;
    pq2.push(1);
    pq2.push(3);
    EXPECT_TRUE(pq1.less_than(pq2));
}

TEST_F(PriorityQueueTest, EqualityOperator) {
    priority_queue<int> pq1;
    pq1.push(1);
    pq1.push(2);
    priority_queue<int> pq2;
    pq2.push(1);
    pq2.push(2);
    EXPECT_TRUE(pq1 == pq2);
}

TEST_F(PriorityQueueTest, InequalityOperator) {
    priority_queue<int> pq1;
    pq1.push(1);
    pq1.push(2);
    priority_queue<int> pq2;
    pq2.push(3);
    pq2.push(4);
    EXPECT_TRUE(pq1 != pq2);
}

TEST_F(PriorityQueueTest, LessThanOperator) {
    priority_queue<int> pq1;
    pq1.push(1);
    priority_queue<int> pq2;
    pq2.push(2);
    EXPECT_TRUE(pq1 < pq2);
}

TEST_F(PriorityQueueTest, GreaterThanOperator) {
    priority_queue<int> pq1;
    pq1.push(3);
    priority_queue<int> pq2;
    pq2.push(1);
    EXPECT_TRUE(pq1 > pq2);
}

TEST_F(PriorityQueueTest, LargeNumberOfElements) {
    priority_queue<int> pq;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        pq.push(i);
    }
    EXPECT_EQ(pq.size(), count);
    EXPECT_EQ(pq.top(), count - 1);
}

TEST_F(PriorityQueueTest, SortedOrderMaxHeap) {
    priority_queue<int> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    pq.push(7);
    vector<int> result;
    while (!pq.empty()) {
        result.push_back(pq.top());
        pq.pop();
    }
    for (size_t i = 1; i < result.size(); ++i) {
        EXPECT_GE(result[i - 1], result[i]);
    }
}

TEST_F(PriorityQueueTest, SortedOrderMinHeap) {
    priority_queue<int, vector<int>, greater<int>> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    pq.push(7);
    vector<int> result;
    while (!pq.empty()) {
        result.push_back(pq.top());
        pq.pop();
    }
    for (size_t i = 1; i < result.size(); ++i) {
        EXPECT_LE(result[i - 1], result[i]);
    }
}

TEST_F(PriorityQueueTest, StringType) {
    priority_queue<string> pq;
    pq.push("apple");
    pq.push("zebra");
    pq.push("banana");
    EXPECT_EQ(pq.top(), "zebra");
    pq.pop();
    EXPECT_EQ(pq.top(), "banana");
    pq.pop();
    EXPECT_EQ(pq.top(), "apple");
}

TEST_F(PriorityQueueTest, PairType) {
    using Pair = pair<int, string>;
    auto comp = [](const Pair& a, const Pair& b) { return a.first < b.first; };
    priority_queue<Pair, vector<Pair>, decltype(comp)> pq(comp);
    pq.push({1, "one"});
    pq.push({3, "three"});
    pq.push({2, "two"});
    EXPECT_EQ(pq.top().first, 3);
}

TEST_F(PriorityQueueTest, ClearViaPop) {
    priority_queue<int> pq;
    pq.push(1);
    pq.push(2);
    pq.push(3);
    while (!pq.empty()) {
        pq.pop();
    }
    EXPECT_TRUE(pq.empty());
    EXPECT_EQ(pq.size(), 0);
}

TEST_F(PriorityQueueTest, DuplicateElements) {
    priority_queue<int> pq;
    pq.push(5);
    pq.push(5);
    pq.push(3);
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
}

class QueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(QueueTest, DefaultConstructor) {
    queue<int> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}

TEST_F(QueueTest, ConstructorWithSequence) {
    deque<int> dq = {1, 2, 3};
    queue<int> q(dq);
    EXPECT_EQ(q.size(), 3);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);
}

TEST_F(QueueTest, MoveConstructorWithSequence) {
    deque<int> dq = {1, 2, 3};
    queue<int> q(move(dq));
    EXPECT_EQ(q.size(), 3);
    EXPECT_EQ(q.front(), 1);
}

TEST_F(QueueTest, ConstructorWithList) {
    list<int> lst = {10, 20, 30};
    queue<int, list<int>> q(lst);
    EXPECT_EQ(q.size(), 3);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 30);
}

TEST_F(QueueTest, Empty) {
    queue<int> q;
    EXPECT_TRUE(q.empty());
    q.push(1);
    EXPECT_FALSE(q.empty());
}

TEST_F(QueueTest, Size) {
    queue<int> q;
    EXPECT_EQ(q.size(), 0);
    q.push(1);
    EXPECT_EQ(q.size(), 1);
    q.push(2);
    EXPECT_EQ(q.size(), 2);
    q.pop();
    EXPECT_EQ(q.size(), 1);
}

TEST_F(QueueTest, Front) {
    queue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);
    EXPECT_EQ(q.front(), 10);
}

TEST_F(QueueTest, ConstFront) {
    deque<int> dq = {10, 20, 30};
    const queue<int> q(dq);
    EXPECT_EQ(q.front(), 10);
}

TEST_F(QueueTest, FrontReference) {
    queue<int> q;
    q.push(10);
    q.front() = 100;
    EXPECT_EQ(q.front(), 100);
}

TEST_F(QueueTest, Back) {
    queue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);
    EXPECT_EQ(q.back(), 30);
}

TEST_F(QueueTest, ConstBack) {
    deque<int> dq = {10, 20, 30};
    const queue<int> q(dq);
    EXPECT_EQ(q.back(), 30);
}

TEST_F(QueueTest, BackReference) {
    queue<int> q;
    q.push(10);
    q.push(20);
    q.back() = 200;
    EXPECT_EQ(q.back(), 200);
}

TEST_F(QueueTest, PushCopy) {
    queue<int> q;
    int val = 42;
    q.push(val);
    EXPECT_EQ(q.front(), 42);
}

TEST_F(QueueTest, PushMove) {
    queue<string> q;
    string s = "hello";
    q.push(move(s));
    EXPECT_EQ(q.front(), "hello");
}

TEST_F(QueueTest, Pop) {
    queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.front(), 1);
    q.pop();
    EXPECT_EQ(q.front(), 2);
    q.pop();
    EXPECT_EQ(q.front(), 3);
    q.pop();
    EXPECT_TRUE(q.empty());
}

TEST_F(QueueTest, Emplace) {
    queue<string> q;
    q.emplace("hello");
    q.emplace("world");
    EXPECT_EQ(q.front(), "hello");
    EXPECT_EQ(q.back(), "world");
}

TEST_F(QueueTest, FifoOrder) {
    queue<int> q;
    for (int i = 0; i < 10; ++i) {
        q.push(i);
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(q.front(), i);
        q.pop();
    }
    EXPECT_TRUE(q.empty());
}

TEST_F(QueueTest, Swap) {
    queue<int> q1;
    q1.push(1);
    q1.push(2);
    q1.push(3);
    queue<int> q2;
    q2.push(10);
    q2.push(20);
    q1.swap(q2);
    EXPECT_EQ(q1.size(), 2);
    EXPECT_EQ(q1.front(), 10);
    EXPECT_EQ(q2.size(), 3);
    EXPECT_EQ(q2.front(), 1);
}

TEST_F(QueueTest, EqualTo) {
    queue<int> q1;
    q1.push(1);
    q1.push(2);
    q1.push(3);
    queue<int> q2;
    q2.push(1);
    q2.push(2);
    q2.push(3);
    queue<int> q3;
    q3.push(4);
    q3.push(5);
    EXPECT_TRUE(q1.equal_to(q2));
    EXPECT_FALSE(q1.equal_to(q3));
}

TEST_F(QueueTest, LessThan) {
    queue<int> q1;
    q1.push(1);
    q1.push(2);
    queue<int> q2;
    q2.push(1);
    q2.push(3);
    EXPECT_TRUE(q1.less_than(q2));
}

TEST_F(QueueTest, EqualityOperator) {
    queue<int> q1;
    q1.push(1);
    q1.push(2);
    queue<int> q2;
    q2.push(1);
    q2.push(2);
    EXPECT_TRUE(q1 == q2);
}

TEST_F(QueueTest, InequalityOperator) {
    queue<int> q1;
    q1.push(1);
    queue<int> q2;
    q2.push(2);
    EXPECT_TRUE(q1 != q2);
}

TEST_F(QueueTest, LessThanOperator) {
    queue<int> q1;
    q1.push(1);
    queue<int> q2;
    q2.push(2);
    EXPECT_TRUE(q1 < q2);
}

TEST_F(QueueTest, GreaterThanOperator) {
    queue<int> q1;
    q1.push(3);
    queue<int> q2;
    q2.push(1);
    EXPECT_TRUE(q1 > q2);
}

TEST_F(QueueTest, StringType) {
    queue<string> q;
    q.push("first");
    q.push("second");
    q.push("third");
    EXPECT_EQ(q.front(), "first");
    q.pop();
    EXPECT_EQ(q.front(), "second");
    q.pop();
    EXPECT_EQ(q.front(), "third");
}

TEST_F(QueueTest, LargeNumberOfElements) {
    queue<int> q;
    const int count = 10000;
    for (int i = 0; i < count; ++i) {
        q.push(i);
    }
    EXPECT_EQ(q.size(), count);
    EXPECT_EQ(q.front(), 0);
    EXPECT_EQ(q.back(), count - 1);
}

TEST_F(QueueTest, IteratorSupport) {
    queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    auto it = q.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, q.end());
}

TEST_F(QueueTest, ConstIteratorSupport) {
    deque<int> dq = {1, 2, 3};
    const queue<int> q(dq);
    auto it = q.begin();
    EXPECT_EQ(*it, 1);
}

TEST_F(QueueTest, RangeBasedForLoop) {
    queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    int sum = 0;
    for (auto val: q) {
        sum += val;
    }
    EXPECT_EQ(sum, 6);
}

class StackTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StackTest, DefaultConstructor) {
    stack<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(StackTest, ConstructorWithSequence) {
    deque<int> dq = {1, 2, 3};
    stack<int> s(dq);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 3);
}

TEST_F(StackTest, MoveConstructorWithSequence) {
    deque<int> dq = {1, 2, 3};
    stack<int> s(move(dq));
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 3);
}

TEST_F(StackTest, ConstructorWithVector) {
    vector<int> vec = {10, 20, 30};
    stack<int, vector<int>> s(vec);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 30);
}

TEST_F(StackTest, ConstructorWithList) {
    list<int> lst = {100, 200, 300};
    stack<int, list<int>> s(lst);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 300);
}

TEST_F(StackTest, Empty) {
    stack<int> s;
    EXPECT_TRUE(s.empty());
    s.push(1);
    EXPECT_FALSE(s.empty());
}

TEST_F(StackTest, Size) {
    stack<int> s;
    EXPECT_EQ(s.size(), 0);
    s.push(1);
    EXPECT_EQ(s.size(), 1);
    s.push(2);
    EXPECT_EQ(s.size(), 2);
    s.pop();
    EXPECT_EQ(s.size(), 1);
}

TEST_F(StackTest, Top) {
    stack<int> s;
    s.push(10);
    s.push(20);
    s.push(30);
    EXPECT_EQ(s.top(), 30);
}

TEST_F(StackTest, ConstTop) {
    deque<int> dq = {10, 20, 30};
    const stack<int> s(dq);
    EXPECT_EQ(s.top(), 30);
}

TEST_F(StackTest, TopReference) {
    stack<int> s;
    s.push(10);
    s.top() = 100;
    EXPECT_EQ(s.top(), 100);
}

TEST_F(StackTest, PushCopy) {
    stack<int> s;
    int val = 42;
    s.push(val);
    EXPECT_EQ(s.top(), 42);
}

TEST_F(StackTest, PushMove) {
    stack<string> s;
    string str = "hello";
    s.push(move(str));
    EXPECT_EQ(s.top(), "hello");
}

TEST_F(StackTest, Pop) {
    stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    EXPECT_EQ(s.top(), 3);
    s.pop();
    EXPECT_EQ(s.top(), 2);
    s.pop();
    EXPECT_EQ(s.top(), 1);
    s.pop();
    EXPECT_TRUE(s.empty());
}

TEST_F(StackTest, Emplace) {
    stack<string> s;
    s.emplace("hello");
    s.emplace("world");
    EXPECT_EQ(s.top(), "world");
    EXPECT_EQ(s.size(), 2);
}

TEST_F(StackTest, LifoOrder) {
    stack<int> s;
    for (int i = 0; i < 10; ++i) {
        s.push(i);
    }
    for (int i = 9; i >= 0; --i) {
        EXPECT_EQ(s.top(), i);
        s.pop();
    }
    EXPECT_TRUE(s.empty());
}

TEST_F(StackTest, Swap) {
    stack<int> s1;
    s1.push(1);
    s1.push(2);
    s1.push(3);
    stack<int> s2;
    s2.push(10);
    s2.push(20);
    s1.swap(s2);
    EXPECT_EQ(s1.size(), 2);
    EXPECT_EQ(s1.top(), 20);
    EXPECT_EQ(s2.size(), 3);
    EXPECT_EQ(s2.top(), 3);
}

TEST_F(StackTest, EqualTo) {
    stack<int> s1;
    s1.push(1);
    s1.push(2);
    s1.push(3);
    stack<int> s2;
    s2.push(1);
    s2.push(2);
    s2.push(3);
    stack<int> s3;
    s3.push(4);
    s3.push(5);
    EXPECT_TRUE(s1.equal_to(s2));
    EXPECT_FALSE(s1.equal_to(s3));
}

TEST_F(StackTest, LessThan) {
    stack<int> s1;
    s1.push(1);
    s1.push(2);
    stack<int> s2;
    s2.push(1);
    s2.push(3);
    EXPECT_TRUE(s1.less_than(s2));
}

TEST_F(StackTest, EqualityOperator) {
    stack<int> s1;
    s1.push(1);
    s1.push(2);
    stack<int> s2;
    s2.push(1);
    s2.push(2);
    EXPECT_TRUE(s1 == s2);
}

TEST_F(StackTest, InequalityOperator) {
    stack<int> s1;
    s1.push(1);
    stack<int> s2;
    s2.push(2);
    EXPECT_TRUE(s1 != s2);
}

TEST_F(StackTest, LessThanOperator) {
    stack<int> s1;
    s1.push(1);
    stack<int> s2;
    s2.push(2);
    EXPECT_TRUE(s1 < s2);
}

TEST_F(StackTest, GreaterThanOperator) {
    stack<int> s1;
    s1.push(3);
    stack<int> s2;
    s2.push(1);
    EXPECT_TRUE(s1 > s2);
}

TEST_F(StackTest, StringType) {
    stack<string> s;
    s.push("first");
    s.push("second");
    s.push("third");
    EXPECT_EQ(s.top(), "third");
    s.pop();
    EXPECT_EQ(s.top(), "second");
    s.pop();
    EXPECT_EQ(s.top(), "first");
}

TEST_F(StackTest, LargeNumberOfElements) {
    stack<int> s;
    const int count = 10000;
    for (int i = 0; i < count; ++i) {
        s.push(i);
    }
    EXPECT_EQ(s.size(), count);
    EXPECT_EQ(s.top(), count - 1);
}

TEST_F(StackTest, RecursivePop) {
    stack<int> s;
    for (int i = 0; i < 100; ++i) {
        s.push(i);
    }
    int expected = 99;
    while (!s.empty()) {
        EXPECT_EQ(s.top(), expected);
        s.pop();
        --expected;
    }
    EXPECT_EQ(expected, -1);
}

class SetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SetTest, DefaultConstructor) {
    set<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(SetTest, ConstructorWithCompare) {
    set<int, greater<int>> s;
    EXPECT_TRUE(s.empty());
    s.insert(3);
    s.insert(1);
    s.insert(2);
    auto it = s.begin();
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 1);
}

TEST_F(SetTest, InitializerListConstructor) {
    set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(s.size(), 5);
    EXPECT_NE(s.find(1), s.end());
    EXPECT_NE(s.find(5), s.end());
}

TEST_F(SetTest, InitializerListConstructorWithCompare) {
    set<int, greater<int>> s = {1, 2, 3};
    EXPECT_EQ(s.size(), 3);
    auto it = s.begin();
    EXPECT_EQ(*it, 3);
}

TEST_F(SetTest, InitializerListAssignment) {
    set<int> s;
    s = {10, 20, 30};
    EXPECT_EQ(s.size(), 3);
    EXPECT_NE(s.find(10), s.end());
}

TEST_F(SetTest, RangeConstructor) {
    vector<int> vec = {5, 2, 8, 1, 9};
    set<int> s(vec.begin(), vec.end());
    EXPECT_EQ(s.size(), 5);
    vector<int> expected = {1, 2, 5, 8, 9};
    auto it = expected.begin();
    for (auto val: s) {
        EXPECT_EQ(val, *it);
        ++it;
    }
}

TEST_F(SetTest, RangeConstructorWithCompare) {
    vector<int> vec = {1, 2, 3};
    set<int, greater<int>> s(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(s.size(), 3);
}

TEST_F(SetTest, CopyConstructor) {
    set<int> s1 = {1, 2, 3};
    set<int> s2(s1);
    EXPECT_EQ(s2.size(), 3);
    EXPECT_NE(s2.find(1), s2.end());
}

TEST_F(SetTest, CopyAssignment) {
    set<int> s1 = {1, 2, 3};
    set<int> s2 = {4, 5};
    s2 = s1;
    EXPECT_EQ(s2.size(), 3);
    EXPECT_NE(s2.find(1), s2.end());
}

TEST_F(SetTest, MoveConstructor) {
    set<int> s1 = {1, 2, 3};
    set<int> s2(move(s1));
    EXPECT_EQ(s2.size(), 3);
    EXPECT_NE(s2.find(2), s2.end());
}

TEST_F(SetTest, MoveAssignment) {
    set<int> s1 = {1, 2, 3};
    set<int> s2 = {4, 5};
    s2 = move(s1);
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(SetTest, BeginEnd) {
    set<int> s = {3, 1, 2};
    auto it = s.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, s.end());
}

TEST_F(SetTest, CbeginCend) {
    set<int> s = {3, 1, 2};
    auto it = s.cbegin();
    EXPECT_EQ(*it, 1);
}

TEST_F(SetTest, ReverseBeginEnd) {
    set<int> s = {1, 2, 3};
    auto rit = s.rbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
    ++rit;
    EXPECT_EQ(*rit, 1);
    ++rit;
    EXPECT_EQ(rit, s.rend());
}

TEST_F(SetTest, CrbeginCrend) {
    set<int> s = {1, 2, 3};
    auto rit = s.crbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(SetTest, Size) {
    set<int> s;
    EXPECT_EQ(s.size(), 0);
    s.insert(1);
    EXPECT_EQ(s.size(), 1);
    s.insert(2);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(SetTest, MaxSize) {
    set<int> s;
    EXPECT_GT(s.max_size(), 0);
}

TEST_F(SetTest, Empty) {
    set<int> s;
    EXPECT_TRUE(s.empty());
    s.insert(1);
    EXPECT_FALSE(s.empty());
    s.erase(1);
    EXPECT_TRUE(s.empty());
}

TEST_F(SetTest, KeyComp) {
    set<int> s;
    auto comp = s.key_comp();
    EXPECT_TRUE(comp(1, 2));
    EXPECT_FALSE(comp(2, 1));
}

TEST_F(SetTest, ValueComp) {
    set<int> s;
    auto comp = s.value_comp();
    EXPECT_TRUE(comp(1, 2));
}

TEST_F(SetTest, InsertValue) {
    set<int> s;
    auto result = s.insert(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(SetTest, InsertDuplicate) {
    set<int> s;
    s.insert(42);
    auto result = s.insert(42);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(s.size(), 1);
}

TEST_F(SetTest, InsertRvalue) {
    set<string> s;
    string str = "hello";
    s.insert(move(str));
    EXPECT_EQ(s.size(), 1);
}

TEST_F(SetTest, InsertWithHint) {
    set<int> s = {1, 3, 5};
    auto it = s.insert(s.begin(), 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(s.size(), 4);
}

TEST_F(SetTest, InsertWithHintRvalue) {
    set<int> s = {1, 3};
    auto it = s.insert(s.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SetTest, InsertRange) {
    set<int> s;
    vector<int> vec = {5, 2, 8, 1, 9};
    s.insert(vec.begin(), vec.end());
    EXPECT_EQ(s.size(), 5);
}

TEST_F(SetTest, Emplace) {
    set<int> s;
    auto result = s.emplace(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(SetTest, EmplaceDuplicate) {
    set<int> s;
    s.emplace(42);
    auto result = s.emplace(42);
    EXPECT_FALSE(result.second);
}

TEST_F(SetTest, EmplaceHint) {
    set<int> s = {1, 3, 5};
    auto it = s.emplace_hint(s.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SetTest, EraseByIterator) {
    set<int> s = {1, 2, 3};
    auto it = s.find(2);
    s.erase(it);
    EXPECT_EQ(s.size(), 2);
    EXPECT_EQ(s.find(2), s.end());
}

TEST_F(SetTest, EraseByKey) {
    set<int> s = {1, 2, 3};
    size_t count = s.erase(2);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(SetTest, EraseByNonExistentKey) {
    set<int> s = {1, 2};
    size_t count = s.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(SetTest, EraseRange) {
    set<int> s = {1, 2, 3, 4, 5};
    auto first = s.find(2);
    auto last = s.find(4);
    s.erase(first, last);
    EXPECT_EQ(s.size(), 3);
    EXPECT_NE(s.find(1), s.end());
    EXPECT_NE(s.find(4), s.end());
    EXPECT_NE(s.find(5), s.end());
}

TEST_F(SetTest, Clear) {
    set<int> s = {1, 2, 3};
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(SetTest, Find) {
    set<int> s = {1, 2, 3};
    auto it = s.find(2);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 2);
}

TEST_F(SetTest, FindNonExistent) {
    set<int> s = {1, 2};
    auto it = s.find(99);
    EXPECT_EQ(it, s.end());
}

TEST_F(SetTest, ConstFind) {
    const set<int> s = {1, 2, 3};
    auto it = s.find(1);
    EXPECT_NE(it, s.end());
}

TEST_F(SetTest, Count) {
    set<int> s = {1, 2, 3};
    EXPECT_EQ(s.count(2), 1);
    EXPECT_EQ(s.count(99), 0);
}

TEST_F(SetTest, LowerBound) {
    set<int> s = {1, 3, 5, 7};
    auto it = s.lower_bound(3);
    EXPECT_EQ(*it, 3);
    it = s.lower_bound(4);
    EXPECT_EQ(*it, 5);
    it = s.lower_bound(8);
    EXPECT_EQ(it, s.end());
}

TEST_F(SetTest, ConstLowerBound) {
    const set<int> s = {1, 3, 5};
    auto it = s.lower_bound(2);
    EXPECT_EQ(*it, 3);
}

TEST_F(SetTest, UpperBound) {
    set<int> s = {1, 3, 5, 7};
    auto it = s.upper_bound(3);
    EXPECT_EQ(*it, 5);
    it = s.upper_bound(7);
    EXPECT_EQ(it, s.end());
}

TEST_F(SetTest, ConstUpperBound) {
    const set<int> s = {1, 3, 5};
    auto it = s.upper_bound(1);
    EXPECT_EQ(*it, 3);
}

TEST_F(SetTest, EqualRange) {
    set<int> s = {1, 3, 5, 7};
    auto range = s.equal_range(3);
    EXPECT_EQ(*range.first, 3);
    EXPECT_EQ(*range.second, 5);
}

TEST_F(SetTest, ConstEqualRange) {
    const set<int> s = {1, 3, 5};
    auto range = s.equal_range(1);
    EXPECT_EQ(*range.first, 1);
}

TEST_F(SetTest, Swap) {
    set<int> s1 = {1, 2, 3};
    set<int> s2 = {4, 5, 6, 7};
    s1.swap(s2);
    EXPECT_EQ(s1.size(), 4);
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(SetTest, EqualTo) {
    set<int> s1 = {1, 2, 3};
    set<int> s2 = {1, 2, 3};
    set<int> s3 = {1, 2, 4};
    EXPECT_TRUE(s1.equal_to(s2));
    EXPECT_FALSE(s1.equal_to(s3));
}

TEST_F(SetTest, LessThan) {
    set<int> s1 = {1, 2, 3};
    set<int> s2 = {1, 2, 4};
    EXPECT_TRUE(s1.less_than(s2));
}

TEST_F(SetTest, EqualityOperator) {
    set<int> s1 = {1, 2, 3};
    set<int> s2 = {1, 2, 3};
    set<int> s3 = {1, 2};
    EXPECT_TRUE(s1 == s2);
    EXPECT_FALSE(s1 == s3);
}

TEST_F(SetTest, InequalityOperator) {
    set<int> s1 = {1, 2};
    set<int> s2 = {3, 4};
    EXPECT_TRUE(s1 != s2);
}

TEST_F(SetTest, LessThanOperator) {
    set<int> s1 = {1, 2};
    set<int> s2 = {1, 3};
    EXPECT_TRUE(s1 < s2);
}

TEST_F(SetTest, GreaterThanOperator) {
    set<int> s1 = {1, 3};
    set<int> s2 = {1, 2};
    EXPECT_TRUE(s1 > s2);
}

TEST_F(SetTest, StringKey) {
    set<string> s;
    s.insert("banana");
    s.insert("apple");
    s.insert("cherry");
    auto it = s.begin();
    EXPECT_EQ(*it, "apple");
    ++it;
    EXPECT_EQ(*it, "banana");
    ++it;
    EXPECT_EQ(*it, "cherry");
}

TEST_F(SetTest, LargeInsert) {
    set<int> s;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        s.insert(i * 2);
    }
    EXPECT_EQ(s.size(), count);
    EXPECT_EQ(*s.begin(), 0);
    EXPECT_EQ(*s.rbegin(), (count - 1) * 2);
}

TEST_F(SetTest, DuplicateValuesIgnored) {
    set<int> s;
    s.insert(1);
    s.insert(1);
    s.insert(1);
    EXPECT_EQ(s.size(), 1);
}

TEST_F(SetTest, RangeBasedForLoop) {
    set<int> s = {1, 2, 3, 4, 5};
    int sum = 0;
    for (auto val: s) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

class MultimapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MultimapTest, DefaultConstructor) {
    multimap<int, string> mm;
    EXPECT_TRUE(mm.empty());
    EXPECT_EQ(mm.size(), 0);
}

TEST_F(MultimapTest, ConstructorWithCompare) {
    multimap<int, string, greater<int>> mm;
    EXPECT_TRUE(mm.empty());
    mm.insert({3, "three"});
    mm.insert({1, "one"});
    mm.insert({2, "two"});
    auto it = mm.begin();
    EXPECT_EQ(it->first, 3);
}

TEST_F(MultimapTest, InitializerListConstructor) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    EXPECT_EQ(mm.size(), 4);
}

TEST_F(MultimapTest, InitializerListAssignment) {
    multimap<int, string> mm;
    mm = {{1, "one"}, {2, "two"}, {2, "second"}};
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(MultimapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    multimap<int, string> mm(vec.begin(), vec.end());
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(MultimapTest, RangeConstructorWithCompare) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    multimap<int, string, greater<int>> mm(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(mm.size(), 2);
}

TEST_F(MultimapTest, CopyConstructor) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    multimap<int, string> mm2(mm1);
    EXPECT_EQ(mm2.size(), 3);
}

TEST_F(MultimapTest, CopyAssignment) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    multimap<int, string> mm2 = {{3, "three"}};
    mm2 = mm1;
    EXPECT_EQ(mm2.size(), 2);
}

TEST_F(MultimapTest, MoveConstructor) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    multimap<int, string> mm2(move(mm1));
    EXPECT_EQ(mm2.size(), 2);
}

TEST_F(MultimapTest, MoveAssignment) {
    multimap<int, string> mm1 = {{1, "one"}};
    multimap<int, string> mm2 = {{2, "two"}};
    mm2 = move(mm1);
    EXPECT_EQ(mm2.size(), 1);
}

TEST_F(MultimapTest, BeginEnd) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = mm.begin();
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 3);
    ++it;
    EXPECT_EQ(it, mm.end());
}

TEST_F(MultimapTest, CbeginCend) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}};
    auto it = mm.cbegin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(MultimapTest, ReverseBeginEnd) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto rit = mm.rbegin();
    EXPECT_EQ(rit->first, 3);
    ++rit;
    EXPECT_EQ(rit->first, 2);
}

TEST_F(MultimapTest, Size) {
    multimap<int, string> mm;
    EXPECT_EQ(mm.size(), 0);
    mm.insert({1, "one"});
    EXPECT_EQ(mm.size(), 1);
}

TEST_F(MultimapTest, MaxSize) {
    multimap<int, int> mm;
    EXPECT_GT(mm.max_size(), 0);
}

TEST_F(MultimapTest, Empty) {
    multimap<int, string> mm;
    EXPECT_TRUE(mm.empty());
    mm.insert({1, "one"});
    EXPECT_FALSE(mm.empty());
}

TEST_F(MultimapTest, KeyComp) {
    multimap<int, string> mm;
    auto comp = mm.key_comp();
    EXPECT_TRUE(comp(1, 2));
}

TEST_F(MultimapTest, ValueComp) {
    multimap<int, string> mm;
    auto comp = mm.value_comp();
    pair<const int, string> p1(1, "a");
    pair<const int, string> p2(2, "b");
    EXPECT_TRUE(comp(p1, p2));
}

TEST_F(MultimapTest, InsertValue) {
    multimap<int, string> mm;
    auto it = mm.insert({1, "one"});
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(MultimapTest, InsertDuplicateKeys) {
    multimap<int, string> mm;
    mm.insert({1, "one"});
    mm.insert({1, "uno"});
    mm.insert({1, "eins"});
    EXPECT_EQ(mm.size(), 3);
    EXPECT_EQ(mm.count(1), 3);
}

TEST_F(MultimapTest, InsertRvalue) {
    multimap<int, string> mm;
    pair<int, string> p(1, "one");
    mm.insert(move(p));
    EXPECT_EQ(mm.size(), 1);
}

TEST_F(MultimapTest, InsertWithHint) {
    multimap<int, string> mm = {{1, "one"}, {3, "three"}};
    auto it = mm.insert(mm.begin(), {2, "two"});
    EXPECT_EQ(it->first, 2);
}

TEST_F(MultimapTest, InsertRange) {
    multimap<int, string> mm;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    mm.insert(vec.begin(), vec.end());
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(MultimapTest, Emplace) {
    multimap<int, string> mm;
    auto it = mm.emplace(1, "one");
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(MultimapTest, EmplaceDuplicate) {
    multimap<int, string> mm;
    mm.emplace(1, "one");
    auto it = mm.emplace(1, "uno");
    EXPECT_EQ(mm.size(), 2);
    EXPECT_EQ(it->second, "uno");
}

TEST_F(MultimapTest, EmplaceHint) {
    multimap<int, string> mm = {{1, "one"}, {3, "three"}};
    auto it = mm.emplace_hint(mm.begin(), 2, "two");
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(MultimapTest, EraseByIterator) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = mm.find(2);
    mm.erase(it);
    EXPECT_EQ(mm.size(), 2);
}

TEST_F(MultimapTest, EraseByKey) {
    multimap<int, string> mm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    size_t count = mm.erase(1);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(mm.size(), 1);
}

TEST_F(MultimapTest, EraseByNonExistentKey) {
    multimap<int, string> mm = {{1, "one"}};
    size_t count = mm.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(MultimapTest, EraseRange) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto range = mm.equal_range(2);
    mm.erase(range.first, range.second);
    EXPECT_EQ(mm.size(), 2);
    EXPECT_EQ(mm.count(2), 0);
}

TEST_F(MultimapTest, Clear) {
    multimap<int, int> mm = {{1, 10}, {2, 20}, {2, 30}};
    mm.clear();
    EXPECT_TRUE(mm.empty());
}

TEST_F(MultimapTest, Find) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}};
    auto it = mm.find(2);
    EXPECT_NE(it, mm.end());
    EXPECT_EQ(it->first, 2);
}

TEST_F(MultimapTest, FindNonExistent) {
    multimap<int, string> mm = {{1, "one"}};
    auto it = mm.find(99);
    EXPECT_EQ(it, mm.end());
}

TEST_F(MultimapTest, ConstFind) {
    const multimap<int, string> mm = {{1, "one"}, {2, "two"}};
    auto it = mm.find(1);
    EXPECT_NE(it, mm.end());
}

TEST_F(MultimapTest, Count) {
    multimap<int, string> mm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    EXPECT_EQ(mm.count(1), 2);
    EXPECT_EQ(mm.count(2), 1);
    EXPECT_EQ(mm.count(99), 0);
}

TEST_F(MultimapTest, LowerBound) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto it = mm.lower_bound(2);
    EXPECT_EQ(it->first, 2);
    it = mm.lower_bound(0);
    EXPECT_EQ(it->first, 1);
}

TEST_F(MultimapTest, UpperBound) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto it = mm.upper_bound(2);
    EXPECT_EQ(it->first, 3);
}

TEST_F(MultimapTest, EqualRange) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto range = mm.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(it->first, 2);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(MultimapTest, ConstEqualRange) {
    const multimap<int, string> mm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    auto range = mm.equal_range(1);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(MultimapTest, Swap) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    multimap<int, string> mm2 = {{3, "three"}, {4, "four"}};
    mm1.swap(mm2);
    EXPECT_EQ(mm1.size(), 2);
    EXPECT_EQ(mm2.size(), 2);
}

TEST_F(MultimapTest, EqualTo) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    multimap<int, string> mm2 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    multimap<int, string> mm3 = {{1, "one"}, {2, "two"}};
    EXPECT_TRUE(mm1.equal_to(mm2));
    EXPECT_FALSE(mm1.equal_to(mm3));
}

TEST_F(MultimapTest, LessThan) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    multimap<int, string> mm2 = {{1, "one"}, {3, "three"}};
    EXPECT_TRUE(mm1.less_than(mm2));
}

TEST_F(MultimapTest, EqualityOperator) {
    multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    multimap<int, string> mm2 = {{1, "one"}, {2, "two"}};
    EXPECT_TRUE(mm1 == mm2);
}

TEST_F(MultimapTest, LessThanOperator) {
    multimap<int, string> mm1 = {{1, "one"}};
    multimap<int, string> mm2 = {{2, "two"}};
    EXPECT_TRUE(mm1 < mm2);
}

TEST_F(MultimapTest, DuplicateKeyOrderPreserved) {
    multimap<int, string> mm;
    mm.insert({1, "first"});
    mm.insert({1, "second"});
    mm.insert({1, "third"});
    auto range = mm.equal_range(1);
    auto it = range.first;
    EXPECT_EQ(it->second, "first");
    ++it;
    EXPECT_EQ(it->second, "second");
    ++it;
    EXPECT_EQ(it->second, "third");
}

TEST_F(MultimapTest, StringKey) {
    multimap<string, int> mm;
    mm.insert({"apple", 1});
    mm.insert({"apple", 2});
    mm.insert({"banana", 3});
    EXPECT_EQ(mm.size(), 3);
    EXPECT_EQ(mm.count("apple"), 2);
    EXPECT_EQ(mm.count("banana"), 1);
}

TEST_F(MultimapTest, LargeInsertWithDuplicates) {
    multimap<int, int> mm;
    const int count = 500;
    for (int i = 0; i < count; ++i) {
        mm.insert({i % 10, i});
    }
    EXPECT_EQ(mm.size(), count);
    EXPECT_EQ(mm.count(0), 50);
    EXPECT_EQ(mm.count(5), 50);
}

TEST_F(MultimapTest, RangeBasedForLoop) {
    multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}};
    int keySum = 0;
    for (const auto& p: mm) {
        keySum += p.first;
    }
    EXPECT_EQ(keySum, 5);
}

class MultisetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MultisetTest, DefaultConstructor) {
    multiset<int> ms;
    EXPECT_TRUE(ms.empty());
    EXPECT_EQ(ms.size(), 0);
}

TEST_F(MultisetTest, ConstructorWithCompare) {
    multiset<int, greater<int>> ms;
    EXPECT_TRUE(ms.empty());
    ms.insert(3);
    ms.insert(1);
    ms.insert(2);
    auto it = ms.begin();
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 1);
}

TEST_F(MultisetTest, InitializerListConstructor) {
    multiset<int> ms = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(ms.size(), 6);
}

TEST_F(MultisetTest, InitializerListConstructorWithCompare) {
    multiset<int, greater<int>> ms = {1, 2, 2, 3};
    EXPECT_EQ(ms.size(), 4);
    auto it = ms.begin();
    EXPECT_EQ(*it, 3);
}

TEST_F(MultisetTest, InitializerListAssignment) {
    multiset<int> ms;
    ms = {10, 20, 20, 30};
    EXPECT_EQ(ms.size(), 4);
}

TEST_F(MultisetTest, RangeConstructor) {
    vector<int> vec = {5, 2, 8, 2, 1, 9};
    multiset<int> ms(vec.begin(), vec.end());
    EXPECT_EQ(ms.size(), 6);
    auto it = ms.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(MultisetTest, RangeConstructorWithCompare) {
    vector<int> vec = {1, 2, 2, 3};
    multiset<int, greater<int>> ms(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(ms.size(), 4);
}

TEST_F(MultisetTest, CopyConstructor) {
    multiset<int> ms1 = {1, 2, 2, 3};
    multiset<int> ms2(ms1);
    EXPECT_EQ(ms2.size(), 4);
    EXPECT_EQ(ms2.count(2), 2);
}

TEST_F(MultisetTest, CopyAssignment) {
    multiset<int> ms1 = {1, 2, 2};
    multiset<int> ms2 = {4, 5};
    ms2 = ms1;
    EXPECT_EQ(ms2.size(), 3);
    EXPECT_EQ(ms2.count(2), 2);
}

TEST_F(MultisetTest, MoveConstructor) {
    multiset<int> ms1 = {1, 2, 2, 3};
    multiset<int> ms2(move(ms1));
    EXPECT_EQ(ms2.size(), 4);
}

TEST_F(MultisetTest, MoveAssignment) {
    multiset<int> ms1 = {1, 2, 2};
    multiset<int> ms2 = {4, 5};
    ms2 = move(ms1);
    EXPECT_EQ(ms2.size(), 3);
}

TEST_F(MultisetTest, BeginEnd) {
    multiset<int> ms = {3, 1, 2, 2};
    auto it = ms.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, ms.end());
}

TEST_F(MultisetTest, CbeginCend) {
    multiset<int> ms = {3, 1, 2};
    auto it = ms.cbegin();
    EXPECT_EQ(*it, 1);
}

TEST_F(MultisetTest, ReverseBeginEnd) {
    multiset<int> ms = {1, 2, 2, 3};
    auto rit = ms.rbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
    ++rit;
    EXPECT_EQ(*rit, 2);
    ++rit;
    EXPECT_EQ(*rit, 1);
    ++rit;
    EXPECT_EQ(rit, ms.rend());
}

TEST_F(MultisetTest, CrbeginCrend) {
    multiset<int> ms = {1, 2, 3};
    auto rit = ms.crbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(MultisetTest, Size) {
    multiset<int> ms;
    EXPECT_EQ(ms.size(), 0);
    ms.insert(1);
    EXPECT_EQ(ms.size(), 1);
    ms.insert(1);
    EXPECT_EQ(ms.size(), 2);
}

TEST_F(MultisetTest, MaxSize) {
    multiset<int> ms;
    EXPECT_GT(ms.max_size(), 0);
}

TEST_F(MultisetTest, Empty) {
    multiset<int> ms;
    EXPECT_TRUE(ms.empty());
    ms.insert(1);
    EXPECT_FALSE(ms.empty());
    ms.erase(1);
    EXPECT_TRUE(ms.empty());
}

TEST_F(MultisetTest, KeyComp) {
    multiset<int> ms;
    auto comp = ms.key_comp();
    EXPECT_TRUE(comp(1, 2));
    EXPECT_FALSE(comp(2, 1));
}

TEST_F(MultisetTest, ValueComp) {
    multiset<int> ms;
    auto comp = ms.value_comp();
    EXPECT_TRUE(comp(1, 2));
}

TEST_F(MultisetTest, InsertValue) {
    multiset<int> ms;
    auto it = ms.insert(42);
    EXPECT_EQ(*it, 42);
    EXPECT_EQ(ms.size(), 1);
}

TEST_F(MultisetTest, InsertDuplicate) {
    multiset<int> ms;
    ms.insert(42);
    ms.insert(42);
    ms.insert(42);
    EXPECT_EQ(ms.size(), 3);
    EXPECT_EQ(ms.count(42), 3);
}

TEST_F(MultisetTest, InsertRvalue) {
    multiset<string> ms;
    string str = "hello";
    ms.insert(move(str));
    EXPECT_EQ(ms.size(), 1);
}

TEST_F(MultisetTest, InsertWithHint) {
    multiset<int> ms = {1, 2, 3};
    auto it = ms.insert(ms.begin(), 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(ms.size(), 4);
}

TEST_F(MultisetTest, InsertWithHintRvalue) {
    multiset<int> ms = {1, 3};
    auto it = ms.insert(ms.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(MultisetTest, InsertRange) {
    multiset<int> ms;
    vector<int> vec = {5, 2, 8, 2, 1, 9};
    ms.insert(vec.begin(), vec.end());
    EXPECT_EQ(ms.size(), 6);
    EXPECT_EQ(ms.count(2), 2);
}

TEST_F(MultisetTest, Emplace) {
    multiset<int> ms;
    auto it = ms.emplace(42);
    EXPECT_EQ(*it, 42);
}

TEST_F(MultisetTest, EmplaceDuplicate) {
    multiset<int> ms;
    ms.emplace(42);
    ms.emplace(42);
    EXPECT_EQ(ms.size(), 2);
}

TEST_F(MultisetTest, EmplaceHint) {
    multiset<int> ms = {1, 3};
    auto it = ms.emplace_hint(ms.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(MultisetTest, EraseByIterator) {
    multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.find(2);
    ms.erase(it);
    EXPECT_EQ(ms.size(), 3);
    EXPECT_EQ(ms.count(2), 1);
}

TEST_F(MultisetTest, EraseByKey) {
    multiset<int> ms = {1, 2, 2, 3, 3, 3};
    size_t count = ms.erase(2);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(ms.size(), 4);
    EXPECT_EQ(ms.count(2), 0);
}

TEST_F(MultisetTest, EraseByNonExistentKey) {
    multiset<int> ms = {1, 2};
    size_t count = ms.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(MultisetTest, EraseRange) {
    multiset<int> ms = {1, 2, 2, 2, 3, 3, 4};
    auto range = ms.equal_range(2);
    ms.erase(range.first, range.second);
    EXPECT_EQ(ms.size(), 4);
    EXPECT_EQ(ms.count(2), 0);
}

TEST_F(MultisetTest, Clear) {
    multiset<int> ms = {1, 2, 2, 3};
    ms.clear();
    EXPECT_TRUE(ms.empty());
    EXPECT_EQ(ms.size(), 0);
}

TEST_F(MultisetTest, Find) {
    multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.find(2);
    EXPECT_NE(it, ms.end());
    EXPECT_EQ(*it, 2);
}

TEST_F(MultisetTest, FindNonExistent) {
    multiset<int> ms = {1, 2};
    auto it = ms.find(99);
    EXPECT_EQ(it, ms.end());
}

TEST_F(MultisetTest, ConstFind) {
    const multiset<int> ms = {1, 2, 2};
    auto it = ms.find(2);
    EXPECT_NE(it, ms.end());
}

TEST_F(MultisetTest, Count) {
    multiset<int> ms = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(ms.count(1), 1);
    EXPECT_EQ(ms.count(2), 2);
    EXPECT_EQ(ms.count(3), 3);
    EXPECT_EQ(ms.count(99), 0);
}

TEST_F(MultisetTest, LowerBound) {
    multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.lower_bound(2);
    EXPECT_EQ(*it, 2);
    it = ms.lower_bound(0);
    EXPECT_EQ(*it, 1);
    it = ms.lower_bound(4);
    EXPECT_EQ(it, ms.end());
}

TEST_F(MultisetTest, ConstLowerBound) {
    const multiset<int> ms = {1, 2, 3};
    auto it = ms.lower_bound(2);
    EXPECT_EQ(*it, 2);
}

TEST_F(MultisetTest, UpperBound) {
    multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.upper_bound(2);
    EXPECT_EQ(*it, 3);
    it = ms.upper_bound(3);
    EXPECT_EQ(it, ms.end());
}

TEST_F(MultisetTest, ConstUpperBound) {
    const multiset<int> ms = {1, 2, 3};
    auto it = ms.upper_bound(2);
    EXPECT_EQ(*it, 3);
}

TEST_F(MultisetTest, EqualRange) {
    multiset<int> ms = {1, 2, 2, 2, 3};
    auto range = ms.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(*it, 2);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(MultisetTest, ConstEqualRange) {
    const multiset<int> ms = {1, 2, 2, 3};
    auto range = ms.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(MultisetTest, EqualRangeNonExistent) {
    multiset<int> ms = {1, 3, 5};
    auto range = ms.equal_range(2);
    EXPECT_EQ(range.first, range.second);
}

TEST_F(MultisetTest, Swap) {
    multiset<int> ms1 = {1, 2, 2};
    multiset<int> ms2 = {3, 4, 5, 6};
    ms1.swap(ms2);
    EXPECT_EQ(ms1.size(), 4);
    EXPECT_EQ(ms2.size(), 3);
}

TEST_F(MultisetTest, EqualTo) {
    multiset<int> ms1 = {1, 2, 2, 3};
    multiset<int> ms2 = {1, 2, 2, 3};
    multiset<int> ms3 = {1, 2, 3};
    EXPECT_TRUE(ms1.equal_to(ms2));
    EXPECT_FALSE(ms1.equal_to(ms3));
}

TEST_F(MultisetTest, LessThan) {
    multiset<int> ms1 = {1, 2, 3};
    multiset<int> ms2 = {1, 2, 4};
    EXPECT_TRUE(ms1.less_than(ms2));
}

TEST_F(MultisetTest, EqualityOperator) {
    multiset<int> ms1 = {1, 2, 2};
    multiset<int> ms2 = {1, 2, 2};
    multiset<int> ms3 = {1, 2};
    EXPECT_TRUE(ms1 == ms2);
    EXPECT_FALSE(ms1 == ms3);
}

TEST_F(MultisetTest, InequalityOperator) {
    multiset<int> ms1 = {1, 2};
    multiset<int> ms2 = {3, 4};
    EXPECT_TRUE(ms1 != ms2);
}

TEST_F(MultisetTest, LessThanOperator) {
    multiset<int> ms1 = {1, 2};
    multiset<int> ms2 = {1, 3};
    EXPECT_TRUE(ms1 < ms2);
}

TEST_F(MultisetTest, GreaterThanOperator) {
    multiset<int> ms1 = {1, 3};
    multiset<int> ms2 = {1, 2};
    EXPECT_TRUE(ms1 > ms2);
}

TEST_F(MultisetTest, StringKey) {
    multiset<string> ms;
    ms.insert("banana");
    ms.insert("apple");
    ms.insert("banana");
    ms.insert("cherry");
    EXPECT_EQ(ms.size(), 4);
    EXPECT_EQ(ms.count("banana"), 2);
    auto it = ms.begin();
    EXPECT_EQ(*it, "apple");
}

TEST_F(MultisetTest, LargeInsertWithDuplicates) {
    multiset<int> ms;
    const int count = 500;
    for (int i = 0; i < count; ++i) {
        ms.insert(i % 10);
    }
    EXPECT_EQ(ms.size(), count);
    EXPECT_EQ(ms.count(0), 50);
    EXPECT_EQ(ms.count(5), 50);
}

TEST_F(MultisetTest, DuplicateOrderPreservation) {
    multiset<int> ms;
    ms.insert(2);
    ms.insert(1);
    ms.insert(2);
    ms.insert(1);
    ms.insert(2);
    auto range = ms.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(*it, 2);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(MultisetTest, RangeBasedForLoop) {
    multiset<int> ms = {1, 2, 2, 3, 3, 3};
    int sum = 0;
    for (auto val: ms) {
        sum += val;
    }
    EXPECT_EQ(sum, 14);
}

class UnorderedMapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UnorderedMapTest, DefaultConstructor) {
    unordered_map<int, string> um;
    EXPECT_TRUE(um.empty());
    EXPECT_EQ(um.size(), 0);
    EXPECT_GT(um.buckets_size(), 0);
}

TEST_F(UnorderedMapTest, ConstructorWithBucketCount) {
    unordered_map<int, string> um(50);
    EXPECT_GE(um.buckets_size(), 50);
}

TEST_F(UnorderedMapTest, ConstructorWithBucketCountAndHash) {
    hash<int> hf;
    unordered_map<int, string> um(30, hf);
    EXPECT_GE(um.buckets_size(), 30);
}

TEST_F(UnorderedMapTest, ConstructorWithBucketCountHashAndEqual) {
    hash<int> hf;
    equal_to<int> eql;
    unordered_map<int, string> um(20, hf, eql);
    EXPECT_GE(um.buckets_size(), 20);
}

TEST_F(UnorderedMapTest, InitializerListConstructor) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(um.size(), 3);
    EXPECT_EQ(um[1], "one");
    EXPECT_EQ(um[2], "two");
    EXPECT_EQ(um[3], "three");
}

TEST_F(UnorderedMapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    unordered_map<int, string> um(vec.begin(), vec.end());
    EXPECT_EQ(um.size(), 3);
}

TEST_F(UnorderedMapTest, RangeConstructorWithBucketCount) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um(vec.begin(), vec.end(), 50);
    EXPECT_EQ(um.size(), 2);
}

TEST_F(UnorderedMapTest, RangeConstructorWithBucketCountAndHash) {
    vector<pair<int, string>> vec = {{1, "one"}};
    hash<int> hf;
    unordered_map<int, string> um(vec.begin(), vec.end(), 30, hf);
    EXPECT_EQ(um.size(), 1);
}

TEST_F(UnorderedMapTest, CopyConstructor) {
    unordered_map<int, string> um1 = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um2(um1);
    EXPECT_EQ(um2.size(), 2);
    EXPECT_EQ(um2[1], "one");
}

TEST_F(UnorderedMapTest, CopyAssignment) {
    unordered_map<int, string> um1 = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um2;
    um2 = um1;
    EXPECT_EQ(um2.size(), 2);
}

TEST_F(UnorderedMapTest, MoveConstructor) {
    unordered_map<int, string> um1 = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um2(move(um1));
    EXPECT_EQ(um2.size(), 2);
}

TEST_F(UnorderedMapTest, MoveAssignment) {
    unordered_map<int, string> um1 = {{1, "one"}};
    unordered_map<int, string> um2;
    um2 = move(um1);
    EXPECT_EQ(um2.size(), 1);
}

TEST_F(UnorderedMapTest, BeginEnd) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}, {3, "three"}};
    int count = 0;
    for (auto it = um.begin(); it != um.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedMapTest, ConstBeginEnd) {
    const unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    int count = 0;
    for (auto it = um.begin(); it != um.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedMapTest, CbeginCend) {
    unordered_map<int, string> um = {{1, "one"}};
    auto it = um.cbegin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(UnorderedMapTest, Size) {
    unordered_map<int, int> um;
    EXPECT_EQ(um.size(), 0);
    um.insert({1, 10});
    EXPECT_EQ(um.size(), 1);
}

TEST_F(UnorderedMapTest, MaxSize) {
    unordered_map<int, int> um;
    EXPECT_GT(um.max_size(), 0);
}

TEST_F(UnorderedMapTest, Empty) {
    unordered_map<int, int> um;
    EXPECT_TRUE(um.empty());
    um.insert({1, 10});
    EXPECT_FALSE(um.empty());
}

TEST_F(UnorderedMapTest, Count) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(um.count(1), 1);
    EXPECT_EQ(um.count(2), 1);
    EXPECT_EQ(um.count(99), 0);
}

TEST_F(UnorderedMapTest, BucketsSize) {
    unordered_map<int, int> um(100);
    EXPECT_GE(um.buckets_size(), 100);
}

TEST_F(UnorderedMapTest, BucketsMaxCount) {
    unordered_map<int, int> um;
    EXPECT_GT(um.buckets_max_count(), 0);
}

TEST_F(UnorderedMapTest, BucketSize) {
    unordered_map<int, int> um;
    um.insert({1, 10});
    um.insert({2, 20});
    EXPECT_GE(um.bucket_size(um.buckets_size() - 1), 0);
}

TEST_F(UnorderedMapTest, HashFunc) {
    unordered_map<int, string> um;
    auto hf = um.hash_func();
    EXPECT_EQ(hf(42), hash<int>()(42));
}

TEST_F(UnorderedMapTest, KeyEql) {
    unordered_map<int, string> um;
    auto eql = um.key_eql();
    EXPECT_TRUE(eql(1, 1));
    EXPECT_FALSE(eql(1, 2));
}

TEST_F(UnorderedMapTest, LoadFactor) {
    unordered_map<int, int> um;
    EXPECT_GE(um.load_factor(), 0.0f);
}

TEST_F(UnorderedMapTest, MaxLoadFactor) {
    unordered_map<int, int> um;
    float mlf = um.max_load_factor();
    EXPECT_GT(mlf, 0.0f);
    um.max_load_factor(2.0f);
    EXPECT_FLOAT_EQ(um.max_load_factor(), 2.0f);
}

TEST_F(UnorderedMapTest, Rehash) {
    unordered_map<int, int> um(10);
    size_t old_buckets = um.buckets_size();
    um.rehash(200);
    EXPECT_GE(um.buckets_size(), 200);
}

TEST_F(UnorderedMapTest, Reserve) {
    unordered_map<int, int> um;
    um.reserve(100);
    EXPECT_GE(um.buckets_size(), 100);
}

TEST_F(UnorderedMapTest, InsertValue) {
    unordered_map<int, string> um;
    auto result = um.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->second, "one");
}

TEST_F(UnorderedMapTest, InsertDuplicate) {
    unordered_map<int, string> um;
    um.insert({1, "one"});
    auto result = um.insert({1, "uno"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(um[1], "one");
}

TEST_F(UnorderedMapTest, InsertRvalue) {
    unordered_map<int, string> um;
    pair<int, string> p(1, "one");
    um.insert(move(p));
    EXPECT_EQ(um[1], "one");
}

TEST_F(UnorderedMapTest, InsertRange) {
    unordered_map<int, string> um;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    um.insert(vec.begin(), vec.end());
    EXPECT_EQ(um.size(), 3);
}

TEST_F(UnorderedMapTest, Emplace) {
    unordered_map<int, string> um;
    auto result = um.emplace(1, "one");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->second, "one");
}

TEST_F(UnorderedMapTest, EmplaceDuplicate) {
    unordered_map<int, string> um;
    um.emplace(1, "one");
    auto result = um.emplace(1, "uno");
    EXPECT_FALSE(result.second);
}

TEST_F(UnorderedMapTest, EraseByKey) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    size_t count = um.erase(1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(um.size(), 1);
}

TEST_F(UnorderedMapTest, EraseByIterator) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    auto it = um.find(1);
    auto next = um.erase(it);
    EXPECT_EQ(um.size(), 1);
}

TEST_F(UnorderedMapTest, EraseRange) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}, {3, "three"}};
    um.erase(um.find(1));
    um.erase(um.find(2));
    EXPECT_EQ(um.size(), 1);
    EXPECT_EQ(um.find(1), um.end());
    EXPECT_EQ(um.find(2), um.end());
}

TEST_F(UnorderedMapTest, EraseByConstIterator) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    const auto cit = um.cbegin();
    um.erase(cit);
    EXPECT_EQ(um.size(), 1);
}

TEST_F(UnorderedMapTest, Clear) {
    unordered_map<int, int> um = {{1, 10}, {2, 20}, {3, 30}};
    um.clear();
    EXPECT_TRUE(um.empty());
}

TEST_F(UnorderedMapTest, Find) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    auto it = um.find(2);
    EXPECT_NE(it, um.end());
    EXPECT_EQ(it->second, "two");
}

TEST_F(UnorderedMapTest, FindNonExistent) {
    unordered_map<int, string> um = {{1, "one"}};
    auto it = um.find(99);
    EXPECT_EQ(it, um.end());
}

TEST_F(UnorderedMapTest, ConstFind) {
    const unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    auto it = um.find(1);
    EXPECT_NE(it, um.end());
}

TEST_F(UnorderedMapTest, EqualRange) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    auto range = um.equal_range(1);
    EXPECT_NE(range.first, um.end());
}

TEST_F(UnorderedMapTest, ConstEqualRange) {
    const unordered_map<int, string> um = {{1, "one"}};
    auto range = um.equal_range(1);
    EXPECT_NE(range.first, um.end());
}

TEST_F(UnorderedMapTest, SubscriptOperator) {
    unordered_map<int, string> um;
    um[1] = "one";
    EXPECT_EQ(um[1], "one");
    um[1] = "uno";
    EXPECT_EQ(um[1], "uno");
}

TEST_F(UnorderedMapTest, SubscriptOperatorInsertDefault) {
    unordered_map<int, int> um;
    EXPECT_EQ(um[1], 0);
    um[1] = 42;
    EXPECT_EQ(um[1], 42);
}

TEST_F(UnorderedMapTest, At) {
    unordered_map<int, string> um = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(um.at(1), "one");
    um.at(1) = "uno";
    EXPECT_EQ(um.at(1), "uno");
}

TEST_F(UnorderedMapTest, AtNonExistent) {
    unordered_map<int, string> um;
    EXPECT_THROW(ignore = um.at(1), iterator_exception);
}

TEST_F(UnorderedMapTest, ConstAt) {
    const unordered_map<int, string> um = {{1, "one"}};
    EXPECT_EQ(um.at(1), "one");
}

TEST_F(UnorderedMapTest, Swap) {
    unordered_map<int, string> um1 = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um2 = {{3, "three"}};
    um1.swap(um2);
    EXPECT_EQ(um1.size(), 1);
    EXPECT_EQ(um2.size(), 2);
}

TEST_F(UnorderedMapTest, EqualTo) {
    unordered_map<int, string> um1 = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um2 = {{2, "two"}, {1, "one"}};
    EXPECT_TRUE(um1.equal_to(um2));
}

TEST_F(UnorderedMapTest, LessThan) {
    unordered_map<int, string> um1 = {{1, "one"}};
    unordered_map<int, string> um2 = {{1, "one"}, {2, "two"}};
    EXPECT_TRUE(um1.less_than(um2));
}

TEST_F(UnorderedMapTest, EqualityOperator) {
    unordered_map<int, string> um1 = {{1, "one"}, {2, "two"}};
    unordered_map<int, string> um2 = {{2, "two"}, {1, "one"}};
    EXPECT_TRUE(um1 == um2);
}

TEST_F(UnorderedMapTest, StringKey) {
    unordered_map<string, int> um;
    um["hello"] = 1;
    um["world"] = 2;
    EXPECT_EQ(um["hello"], 1);
    EXPECT_EQ(um["world"], 2);
}

TEST_F(UnorderedMapTest, LargeInsert) {
    unordered_map<int, int> um;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        um.insert({i, i * 10});
    }
    EXPECT_EQ(um.size(), count);
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(um[i], i * 10);
    }
}

class UnorderedSetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UnorderedSetTest, DefaultConstructor) {
    unordered_set<int> us;
    EXPECT_TRUE(us.empty());
    EXPECT_EQ(us.size(), 0);
    EXPECT_GT(us.buckets_size(), 0);
}

TEST_F(UnorderedSetTest, ConstructorWithBucketCount) {
    unordered_set<int> us(50);
    EXPECT_GE(us.buckets_size(), 50);
}

TEST_F(UnorderedSetTest, ConstructorWithBucketCountAndHash) {
    hash<int> hf;
    unordered_set<int> us(30, hf);
    EXPECT_GE(us.buckets_size(), 30);
}

TEST_F(UnorderedSetTest, ConstructorWithBucketCountHashAndEqual) {
    hash<int> hf;
    equal_to<int> eql;
    unordered_set<int> us(20, hf, eql);
    EXPECT_GE(us.buckets_size(), 20);
}

TEST_F(UnorderedSetTest, InitializerListConstructor) {
    unordered_set<int> us = {1, 2, 3, 4, 5};
    EXPECT_EQ(us.size(), 5);
    EXPECT_EQ(us.count(1), 1);
    EXPECT_EQ(us.count(3), 1);
    EXPECT_EQ(us.count(5), 1);
}

TEST_F(UnorderedSetTest, RangeConstructor) {
    vector<int> vec = {10, 20, 30, 40, 50};
    unordered_set<int> us(vec.begin(), vec.end());
    EXPECT_EQ(us.size(), 5);
}

TEST_F(UnorderedSetTest, RangeConstructorWithBucketCount) {
    vector<int> vec = {1, 2, 3};
    unordered_set<int> us(vec.begin(), vec.end(), 50);
    EXPECT_EQ(us.size(), 3);
}

TEST_F(UnorderedSetTest, RangeConstructorWithBucketCountAndHash) {
    vector<int> vec = {1, 2};
    hash<int> hf;
    unordered_set<int> us(vec.begin(), vec.end(), 30, hf);
    EXPECT_EQ(us.size(), 2);
}

TEST_F(UnorderedSetTest, CopyConstructor) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2(us1);
    EXPECT_EQ(us2.size(), 3);
    EXPECT_EQ(us2.count(1), 1);
}

TEST_F(UnorderedSetTest, CopyAssignment) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2;
    us2 = us1;
    EXPECT_EQ(us2.size(), 3);
}

TEST_F(UnorderedSetTest, MoveConstructor) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2(move(us1));
    EXPECT_EQ(us2.size(), 3);
}

TEST_F(UnorderedSetTest, MoveAssignment) {
    unordered_set<int> us1 = {1, 2};
    unordered_set<int> us2;
    us2 = move(us1);
    EXPECT_EQ(us2.size(), 2);
}

TEST_F(UnorderedSetTest, BeginEnd) {
    unordered_set<int> us = {1, 2, 3};
    int count = 0;
    for (auto it = us.begin(); it != us.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedSetTest, ConstBeginEnd) {
    const unordered_set<int> us = {1, 2};
    int count = 0;
    for (auto it = us.begin(); it != us.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedSetTest, CbeginCend) {
    unordered_set<int> us = {1, 2, 3};
    int count = 0;
    for (auto it = us.cbegin(); it != us.cend(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedSetTest, Size) {
    unordered_set<int> us;
    EXPECT_EQ(us.size(), 0);
    us.insert(1);
    EXPECT_EQ(us.size(), 1);
    us.insert(2);
    EXPECT_EQ(us.size(), 2);
}

TEST_F(UnorderedSetTest, MaxSize) {
    unordered_set<int> us;
    EXPECT_GT(us.max_size(), 0);
}

TEST_F(UnorderedSetTest, Empty) {
    unordered_set<int> us;
    EXPECT_TRUE(us.empty());
    us.insert(1);
    EXPECT_FALSE(us.empty());
    us.erase(1);
    EXPECT_TRUE(us.empty());
}

TEST_F(UnorderedSetTest, Count) {
    unordered_set<int> us = {1, 2, 3};
    EXPECT_EQ(us.count(1), 1);
    EXPECT_EQ(us.count(2), 1);
    EXPECT_EQ(us.count(99), 0);
}

TEST_F(UnorderedSetTest, BucketsSize) {
    unordered_set<int> us(100);
    EXPECT_GE(us.buckets_size(), 100);
}

TEST_F(UnorderedSetTest, MaxBucketCount) {
    unordered_set<int> us;
    EXPECT_GT(us.max_bucket_count(), 0);
}

TEST_F(UnorderedSetTest, BucketSize) {
    unordered_set<int> us;
    us.insert(1);
    us.insert(2);
    size_t bucket = us.buckets_size() - 1;
    EXPECT_GE(us.bucket_size(bucket), 0);
}

TEST_F(UnorderedSetTest, HashFunct) {
    unordered_set<int> us;
    auto hf = us.hash_funct();
    EXPECT_EQ(hf(42), hash<int>()(42));
}

TEST_F(UnorderedSetTest, KeyEq) {
    unordered_set<int> us;
    auto eql = us.key_eq();
    EXPECT_TRUE(eql(1, 1));
    EXPECT_FALSE(eql(1, 2));
}

TEST_F(UnorderedSetTest, LoadFactor) {
    unordered_set<int> us;
    EXPECT_GE(us.load_factor(), 0.0f);
}

TEST_F(UnorderedSetTest, MaxLoadFactor) {
    unordered_set<int> us;
    float mlf = us.max_load_factor();
    EXPECT_GT(mlf, 0.0f);
    us.max_load_factor(2.0f);
    EXPECT_FLOAT_EQ(us.max_load_factor(), 2.0f);
}

TEST_F(UnorderedSetTest, Rehash) {
    unordered_set<int> us(10);
    us.rehash(200);
    EXPECT_GE(us.buckets_size(), 200);
}

TEST_F(UnorderedSetTest, Reserve) {
    unordered_set<int> us;
    us.reserve(100);
    EXPECT_GE(us.buckets_size(), 100);
}

TEST_F(UnorderedSetTest, InsertValue) {
    unordered_set<int> us;
    auto result = us.insert(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(UnorderedSetTest, InsertDuplicate) {
    unordered_set<int> us;
    us.insert(42);
    auto result = us.insert(42);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(us.size(), 1);
}

TEST_F(UnorderedSetTest, InsertRvalue) {
    unordered_set<string> us;
    string s = "hello";
    us.insert(move(s));
    EXPECT_EQ(us.size(), 1);
}

TEST_F(UnorderedSetTest, InsertRange) {
    unordered_set<int> us;
    vector<int> vec = {1, 2, 3, 4, 5};
    us.insert(vec.begin(), vec.end());
    EXPECT_EQ(us.size(), 5);
}

TEST_F(UnorderedSetTest, Emplace) {
    unordered_set<int> us;
    auto result = us.emplace(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(UnorderedSetTest, EmplaceDuplicate) {
    unordered_set<int> us;
    us.emplace(42);
    auto result = us.emplace(42);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(us.size(), 1);
}

TEST_F(UnorderedSetTest, EraseByKey) {
    unordered_set<int> us = {1, 2, 3};
    size_t count = us.erase(2);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(us.size(), 2);
    EXPECT_EQ(us.count(2), 0);
}

TEST_F(UnorderedSetTest, EraseByKeyNonExistent) {
    unordered_set<int> us = {1, 2};
    size_t count = us.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(UnorderedSetTest, EraseByIterator) {
    unordered_set<int> us = {1, 2, 3};
    auto it = us.find(1);
    us.erase(it);
    EXPECT_EQ(us.size(), 2);
}

TEST_F(UnorderedSetTest, EraseRange) {
    unordered_set<int> us = {1, 2, 3, 4, 5};
    auto first = us.find(2);
    auto last = us.find(4);
    us.erase(first, last);
    EXPECT_LE(us.size(), 4);
}

TEST_F(UnorderedSetTest, Clear) {
    unordered_set<int> us = {1, 2, 3};
    us.clear();
    EXPECT_TRUE(us.empty());
    EXPECT_EQ(us.size(), 0);
}

TEST_F(UnorderedSetTest, Find) {
    unordered_set<int> us = {1, 2, 3};
    auto it = us.find(2);
    EXPECT_NE(it, us.end());
    EXPECT_EQ(*it, 2);
}

TEST_F(UnorderedSetTest, FindNonExistent) {
    unordered_set<int> us = {1, 2};
    auto it = us.find(99);
    EXPECT_EQ(it, us.end());
}

TEST_F(UnorderedSetTest, ConstFind) {
    const unordered_set<int> us = {1, 2, 3};
    auto it = us.find(1);
    EXPECT_NE(it, us.end());
}

TEST_F(UnorderedSetTest, EqualRange) {
    unordered_set<int> us = {1, 2, 3};
    auto range = us.equal_range(2);
    EXPECT_NE(range.first, us.end());
    EXPECT_EQ(*range.first, 2);
}

TEST_F(UnorderedSetTest, ConstEqualRange) {
    const unordered_set<int> us = {1, 2};
    auto range = us.equal_range(1);
    EXPECT_NE(range.first, us.end());
}

TEST_F(UnorderedSetTest, Swap) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2 = {4, 5};
    us1.swap(us2);
    EXPECT_EQ(us1.size(), 2);
    EXPECT_EQ(us2.size(), 3);
}

TEST_F(UnorderedSetTest, EqualTo) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2 = {3, 2, 1};
    EXPECT_TRUE(us1.equal_to(us2));
}

TEST_F(UnorderedSetTest, NotEqualTo) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2 = {1, 2, 4};
    EXPECT_FALSE(us1.equal_to(us2));
}

TEST_F(UnorderedSetTest, LessThan) {
    unordered_set<int> us1 = {1, 2};
    unordered_set<int> us2 = {1, 2, 3};
    EXPECT_TRUE(us1.less_than(us2));
}

TEST_F(UnorderedSetTest, EqualityOperator) {
    unordered_set<int> us1 = {1, 2, 3};
    unordered_set<int> us2 = {2, 3, 1};
    EXPECT_TRUE(us1 == us2);
}

TEST_F(UnorderedSetTest, InequalityOperator) {
    unordered_set<int> us1 = {1, 2};
    unordered_set<int> us2 = {3, 4};
    EXPECT_TRUE(us1 != us2);
}

TEST_F(UnorderedSetTest, StringKey) {
    unordered_set<string> us;
    us.insert("hello");
    us.insert("world");
    us.insert("foo");
    EXPECT_EQ(us.size(), 3);
    EXPECT_EQ(us.count("hello"), 1);
    EXPECT_EQ(us.count("world"), 1);
    EXPECT_EQ(us.count("foo"), 1);
    EXPECT_EQ(us.count("bar"), 0);
}

TEST_F(UnorderedSetTest, LargeInsert) {
    unordered_set<int> us;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        us.insert(i);
    }
    EXPECT_EQ(us.size(), count);
    for (int i = 0; i < count; ++i) {
        EXPECT_NE(us.find(i), us.end());
    }
}

TEST_F(UnorderedSetTest, DuplicateValuesIgnored) {
    unordered_set<int> us;
    us.insert(1);
    us.insert(1);
    us.insert(1);
    EXPECT_EQ(us.size(), 1);
}

TEST_F(UnorderedSetTest, IteratorReadOnly) {
    unordered_set<int> us = {10, 20, 30};
    auto it = us.begin();
    const int& val = *it;
    EXPECT_TRUE(val == 10 || val == 20 || val == 30);
}

class UnorderedMultimapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UnorderedMultimapTest, DefaultConstructor) {
    unordered_multimap<int, string> umm;
    EXPECT_TRUE(umm.empty());
    EXPECT_EQ(umm.size(), 0);
}

TEST_F(UnorderedMultimapTest, ConstructorWithBucketCount) {
    unordered_multimap<int, string> umm(50);
    EXPECT_GE(umm.buckets_size(), 50);
}

TEST_F(UnorderedMultimapTest, ConstructorWithBucketCountAndHash) {
    hash<int> hf;
    unordered_multimap<int, string> umm(30, hf);
    EXPECT_GE(umm.buckets_size(), 30);
}

TEST_F(UnorderedMultimapTest, ConstructorWithBucketCountHashAndEqual) {
    hash<int> hf;
    equal_to<int> eql;
    unordered_multimap<int, string> umm(20, hf, eql);
    EXPECT_GE(umm.buckets_size(), 20);
}

TEST_F(UnorderedMultimapTest, InitializerListConstructor) {
    unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}, {2, "deux"}};
    EXPECT_EQ(umm.size(), 3);
}

TEST_F(UnorderedMultimapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    unordered_multimap<int, string> umm(vec.begin(), vec.end());
    EXPECT_EQ(umm.size(), 3);
}

TEST_F(UnorderedMultimapTest, RangeConstructorWithBucketCount) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    unordered_multimap<int, string> umm(vec.begin(), vec.end(), 50);
    EXPECT_EQ(umm.size(), 2);
}

TEST_F(UnorderedMultimapTest, RangeConstructorWithBucketCountAndHash) {
    vector<pair<int, string>> vec = {{1, "one"}};
    hash<int> hf;
    unordered_multimap<int, string> umm(vec.begin(), vec.end(), 30, hf);
    EXPECT_EQ(umm.size(), 1);
}

TEST_F(UnorderedMultimapTest, CopyConstructor) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    unordered_multimap<int, string> umm2(umm1);
    EXPECT_EQ(umm2.size(), 3);
}

TEST_F(UnorderedMultimapTest, CopyAssignment) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}};
    unordered_multimap<int, string> umm2;
    umm2 = umm1;
    EXPECT_EQ(umm2.size(), 2);
}

TEST_F(UnorderedMultimapTest, MoveConstructor) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}};
    unordered_multimap<int, string> umm2(move(umm1));
    EXPECT_EQ(umm2.size(), 2);
}

TEST_F(UnorderedMultimapTest, MoveAssignment) {
    unordered_multimap<int, string> umm1 = {{1, "one"}};
    unordered_multimap<int, string> umm2;
    umm2 = move(umm1);
    EXPECT_EQ(umm2.size(), 1);
}

TEST_F(UnorderedMultimapTest, BeginEnd) {
    unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}, {3, "three"}};
    int count = 0;
    for (auto it = umm.begin(); it != umm.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedMultimapTest, ConstBeginEnd) {
    const unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}};
    int count = 0;
    for (auto it = umm.begin(); it != umm.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedMultimapTest, CbeginCend) {
    unordered_multimap<int, string> umm = {{1, "one"}};
    auto it = umm.cbegin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(UnorderedMultimapTest, Size) {
    unordered_multimap<int, int> umm;
    EXPECT_EQ(umm.size(), 0);
    umm.insert({1, 10});
    EXPECT_EQ(umm.size(), 1);
}

TEST_F(UnorderedMultimapTest, MaxSize) {
    unordered_multimap<int, int> umm;
    EXPECT_GT(umm.max_size(), 0);
}

TEST_F(UnorderedMultimapTest, Empty) {
    unordered_multimap<int, int> umm;
    EXPECT_TRUE(umm.empty());
    umm.insert({1, 10});
    EXPECT_FALSE(umm.empty());
}

TEST_F(UnorderedMultimapTest, Count) {
    unordered_multimap<int, string> umm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    EXPECT_EQ(umm.count(1), 2);
    EXPECT_EQ(umm.count(2), 1);
    EXPECT_EQ(umm.count(99), 0);
}

TEST_F(UnorderedMultimapTest, BucketsSize) {
    unordered_multimap<int, int> umm(100);
    EXPECT_GE(umm.buckets_size(), 100);
}

TEST_F(UnorderedMultimapTest, BucketsMaxCount) {
    unordered_multimap<int, int> umm;
    EXPECT_GT(umm.buckets_max_size(), 0);
}

TEST_F(UnorderedMultimapTest, BucketSize) {
    unordered_multimap<int, int> umm;
    umm.insert({1, 10});
    umm.insert({2, 20});
    EXPECT_GE(umm.bucket_size(umm.buckets_size() - 1), 0);
}

TEST_F(UnorderedMultimapTest, HashFunc) {
    unordered_multimap<int, string> umm;
    auto hf = umm.hash_func();
    EXPECT_EQ(hf(42), hash<int>()(42));
}

TEST_F(UnorderedMultimapTest, KeyEql) {
    unordered_multimap<int, string> umm;
    auto eql = umm.key_eql();
    EXPECT_TRUE(eql(1, 1));
    EXPECT_FALSE(eql(1, 2));
}

TEST_F(UnorderedMultimapTest, LoadFactor) {
    unordered_multimap<int, int> umm;
    EXPECT_GE(umm.load_factor(), 0.0f);
}

TEST_F(UnorderedMultimapTest, MaxLoadFactor) {
    unordered_multimap<int, int> umm;
    float mlf = umm.max_load_factor();
    EXPECT_GT(mlf, 0.0f);
    umm.max_load_factor(2.0f);
    EXPECT_FLOAT_EQ(umm.max_load_factor(), 2.0f);
}

TEST_F(UnorderedMultimapTest, Rehash) {
    unordered_multimap<int, int> umm(10);
    umm.rehash(200);
    EXPECT_GE(umm.buckets_size(), 200);
}

TEST_F(UnorderedMultimapTest, Reserve) {
    unordered_multimap<int, int> umm;
    umm.reserve(100);
    EXPECT_GE(umm.buckets_size(), 100);
}

TEST_F(UnorderedMultimapTest, InsertValue) {
    unordered_multimap<int, string> umm;
    auto it = umm.insert({1, "one"});
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(UnorderedMultimapTest, InsertDuplicateKeys) {
    unordered_multimap<int, string> umm;
    umm.insert({1, "one"});
    umm.insert({1, "uno"});
    umm.insert({1, "eins"});
    EXPECT_EQ(umm.size(), 3);
    EXPECT_EQ(umm.count(1), 3);
}

TEST_F(UnorderedMultimapTest, InsertRvalue) {
    unordered_multimap<int, string> umm;
    pair<int, string> p(1, "one");
    umm.insert(move(p));
    EXPECT_EQ(umm.size(), 1);
}

TEST_F(UnorderedMultimapTest, InsertRange) {
    unordered_multimap<int, string> umm;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    umm.insert(vec.begin(), vec.end());
    EXPECT_EQ(umm.size(), 3);
    EXPECT_EQ(umm.count(2), 2);
}

TEST_F(UnorderedMultimapTest, Emplace) {
    unordered_multimap<int, string> umm;
    auto it = umm.emplace(1, "one");
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(UnorderedMultimapTest, EmplaceDuplicate) {
    unordered_multimap<int, string> umm;
    umm.emplace(1, "one");
    auto it = umm.emplace(1, "uno");
    EXPECT_EQ(umm.size(), 2);
    EXPECT_EQ(it->second, "uno");
}

TEST_F(UnorderedMultimapTest, EraseByKey) {
    unordered_multimap<int, string> umm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    size_t count = umm.erase(1);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(umm.size(), 1);
    EXPECT_EQ(umm.count(1), 0);
}

TEST_F(UnorderedMultimapTest, EraseByIterator) {
    unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}};
    auto it = umm.find(1);
    auto next = umm.erase(it);
    EXPECT_EQ(umm.size(), 1);
}

TEST_F(UnorderedMultimapTest, EraseRange) {
    unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto first = umm.find(1);
    auto last = umm.find(3);
    umm.erase(first, last);
    EXPECT_LE(umm.size(), 2);
}

TEST_F(UnorderedMultimapTest, EraseByConstIterator) {
    unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}};
    const auto cit = umm.cbegin();
    umm.erase(cit);
    EXPECT_EQ(umm.size(), 1);
}

TEST_F(UnorderedMultimapTest, Clear) {
    unordered_multimap<int, int> umm = {{1, 10}, {2, 20}, {2, 30}};
    umm.clear();
    EXPECT_TRUE(umm.empty());
}

TEST_F(UnorderedMultimapTest, Find) {
    unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}, {2, "deux"}};
    auto it = umm.find(2);
    EXPECT_NE(it, umm.end());
    EXPECT_EQ(it->first, 2);
}

TEST_F(UnorderedMultimapTest, FindNonExistent) {
    unordered_multimap<int, string> umm = {{1, "one"}};
    auto it = umm.find(99);
    EXPECT_EQ(it, umm.end());
}

TEST_F(UnorderedMultimapTest, ConstFind) {
    const unordered_multimap<int, string> umm = {{1, "one"}, {2, "two"}};
    auto it = umm.find(1);
    EXPECT_NE(it, umm.end());
}

TEST_F(UnorderedMultimapTest, EqualRange) {
    unordered_multimap<int, string> umm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    auto range = umm.equal_range(1);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(it->first, 1);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedMultimapTest, ConstEqualRange) {
    const unordered_multimap<int, string> umm = {{1, "one"}, {1, "uno"}};
    auto range = umm.equal_range(1);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedMultimapTest, Swap) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}};
    unordered_multimap<int, string> umm2 = {{3, "three"}};
    umm1.swap(umm2);
    EXPECT_EQ(umm1.size(), 1);
    EXPECT_EQ(umm2.size(), 2);
}

TEST_F(UnorderedMultimapTest, EqualTo) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    unordered_multimap<int, string> umm2 = {{2, "two"}, {2, "deux"}, {1, "one"}};
    EXPECT_TRUE(umm1.equal_to(umm2));
}

TEST_F(UnorderedMultimapTest, NotEqualTo) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}};
    unordered_multimap<int, string> umm2 = {{1, "one"}};
    EXPECT_FALSE(umm1.equal_to(umm2));
}

TEST_F(UnorderedMultimapTest, LessThan) {
    unordered_multimap<int, string> umm1 = {{1, "one"}};
    unordered_multimap<int, string> umm2 = {{1, "one"}, {2, "two"}};
    EXPECT_TRUE(umm1.less_than(umm2));
}

TEST_F(UnorderedMultimapTest, EqualityOperator) {
    unordered_multimap<int, string> umm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    unordered_multimap<int, string> umm2 = {{2, "deux"}, {2, "two"}, {1, "one"}};
    EXPECT_TRUE(umm1 == umm2);
}

TEST_F(UnorderedMultimapTest, StringKey) {
    unordered_multimap<string, int> umm;
    umm.insert({"apple", 1});
    umm.insert({"apple", 2});
    umm.insert({"banana", 3});
    EXPECT_EQ(umm.size(), 3);
    EXPECT_EQ(umm.count("apple"), 2);
}

TEST_F(UnorderedMultimapTest, LargeInsertWithDuplicates) {
    unordered_multimap<int, int> umm;
    const int count = 500;
    for (int i = 0; i < count; ++i) {
        umm.insert({i % 10, i});
    }
    EXPECT_EQ(umm.size(), count);
    EXPECT_EQ(umm.count(0), 50);
    EXPECT_EQ(umm.count(5), 50);
}

class UnorderedMultisetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UnorderedMultisetTest, DefaultConstructor) {
    unordered_multiset<int> ums;
    EXPECT_TRUE(ums.empty());
    EXPECT_EQ(ums.size(), 0);
    EXPECT_GT(ums.buckets_size(), 0);
}

TEST_F(UnorderedMultisetTest, ConstructorWithBucketCount) {
    unordered_multiset<int> ums(50);
    EXPECT_GE(ums.buckets_size(), 50);
}

TEST_F(UnorderedMultisetTest, ConstructorWithBucketCountAndHash) {
    hash<int> hf;
    unordered_multiset<int> ums(30, hf);
    EXPECT_GE(ums.buckets_size(), 30);
}

TEST_F(UnorderedMultisetTest, ConstructorWithBucketCountHashAndEqual) {
    hash<int> hf;
    equal_to<int> eql;
    unordered_multiset<int> ums(20, hf, eql);
    EXPECT_GE(ums.buckets_size(), 20);
}

TEST_F(UnorderedMultisetTest, InitializerListConstructor) {
    unordered_multiset<int> ums = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(ums.size(), 6);
    EXPECT_EQ(ums.count(1), 1);
    EXPECT_EQ(ums.count(2), 2);
    EXPECT_EQ(ums.count(3), 3);
}

TEST_F(UnorderedMultisetTest, RangeConstructor) {
    vector<int> vec = {5, 2, 8, 2, 1, 9};
    unordered_multiset<int> ums(vec.begin(), vec.end());
    EXPECT_EQ(ums.size(), 6);
    EXPECT_EQ(ums.count(2), 2);
}

TEST_F(UnorderedMultisetTest, RangeConstructorWithBucketCount) {
    vector<int> vec = {1, 2, 2};
    unordered_multiset<int> ums(vec.begin(), vec.end(), 50);
    EXPECT_EQ(ums.size(), 3);
}

TEST_F(UnorderedMultisetTest, RangeConstructorWithBucketCountAndHash) {
    vector<int> vec = {1, 2};
    hash<int> hf;
    unordered_multiset<int> ums(vec.begin(), vec.end(), 30, hf);
    EXPECT_EQ(ums.size(), 2);
}

TEST_F(UnorderedMultisetTest, CopyConstructor) {
    unordered_multiset<int> ums1 = {1, 2, 2, 3};
    unordered_multiset<int> ums2(ums1);
    EXPECT_EQ(ums2.size(), 4);
    EXPECT_EQ(ums2.count(2), 2);
}

TEST_F(UnorderedMultisetTest, CopyAssignment) {
    unordered_multiset<int> ums1 = {1, 2, 2};
    unordered_multiset<int> ums2;
    ums2 = ums1;
    EXPECT_EQ(ums2.size(), 3);
}

TEST_F(UnorderedMultisetTest, MoveConstructor) {
    unordered_multiset<int> ums1 = {1, 2, 2, 3};
    unordered_multiset<int> ums2(move(ums1));
    EXPECT_EQ(ums2.size(), 4);
}

TEST_F(UnorderedMultisetTest, MoveAssignment) {
    unordered_multiset<int> ums1 = {1, 2};
    unordered_multiset<int> ums2;
    ums2 = move(ums1);
    EXPECT_EQ(ums2.size(), 2);
}

TEST_F(UnorderedMultisetTest, BeginEnd) {
    unordered_multiset<int> ums = {1, 2, 3};
    int count = 0;
    for (auto it = ums.begin(); it != ums.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedMultisetTest, ConstBeginEnd) {
    const unordered_multiset<int> ums = {1, 2};
    int count = 0;
    for (auto it = ums.begin(); it != ums.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedMultisetTest, CbeginCend) {
    unordered_multiset<int> ums = {1, 2, 3};
    int count = 0;
    for (auto it = ums.cbegin(); it != ums.cend(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedMultisetTest, Size) {
    unordered_multiset<int> ums;
    EXPECT_EQ(ums.size(), 0);
    ums.insert(1);
    EXPECT_EQ(ums.size(), 1);
    ums.insert(1);
    EXPECT_EQ(ums.size(), 2);
}

TEST_F(UnorderedMultisetTest, MaxSize) {
    unordered_multiset<int> ums;
    EXPECT_GT(ums.max_size(), 0);
}

TEST_F(UnorderedMultisetTest, Empty) {
    unordered_multiset<int> ums;
    EXPECT_TRUE(ums.empty());
    ums.insert(1);
    EXPECT_FALSE(ums.empty());
    ums.erase(1);
    EXPECT_TRUE(ums.empty());
}

TEST_F(UnorderedMultisetTest, Count) {
    unordered_multiset<int> ums = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(ums.count(1), 1);
    EXPECT_EQ(ums.count(2), 2);
    EXPECT_EQ(ums.count(3), 3);
    EXPECT_EQ(ums.count(99), 0);
}

TEST_F(UnorderedMultisetTest, BucketsSize) {
    unordered_multiset<int> ums(100);
    EXPECT_GE(ums.buckets_size(), 100);
}

TEST_F(UnorderedMultisetTest, BucketsMaxCount) {
    unordered_multiset<int> ums;
    EXPECT_GT(ums.buckets_max_size(), 0);
}

TEST_F(UnorderedMultisetTest, BucketSize) {
    unordered_multiset<int> ums;
    ums.insert(1);
    ums.insert(2);
    EXPECT_GE(ums.bucket_size(ums.buckets_size() - 1), 0);
}

TEST_F(UnorderedMultisetTest, HashFunc) {
    unordered_multiset<int> ums;
    auto hf = ums.hash_func();
    EXPECT_EQ(hf(42), hash<int>()(42));
}

TEST_F(UnorderedMultisetTest, KeyEql) {
    unordered_multiset<int> ums;
    auto eql = ums.key_eql();
    EXPECT_TRUE(eql(1, 1));
    EXPECT_FALSE(eql(1, 2));
}

TEST_F(UnorderedMultisetTest, LoadFactor) {
    unordered_multiset<int> ums;
    EXPECT_GE(ums.load_factor(), 0.0f);
}

TEST_F(UnorderedMultisetTest, MaxLoadFactor) {
    unordered_multiset<int> ums;
    float mlf = ums.max_load_factor();
    EXPECT_GT(mlf, 0.0f);
    ums.max_load_factor(2.0f);
    EXPECT_FLOAT_EQ(ums.max_load_factor(), 2.0f);
}

TEST_F(UnorderedMultisetTest, Rehash) {
    unordered_multiset<int> ums(10);
    ums.rehash(200);
    EXPECT_GE(ums.buckets_size(), 200);
}

TEST_F(UnorderedMultisetTest, Reserve) {
    unordered_multiset<int> ums;
    ums.reserve(100);
    EXPECT_GE(ums.buckets_size(), 100);
}

TEST_F(UnorderedMultisetTest, InsertValue) {
    unordered_multiset<int> ums;
    auto it = ums.insert(42);
    EXPECT_EQ(*it, 42);
}

TEST_F(UnorderedMultisetTest, InsertDuplicate) {
    unordered_multiset<int> ums;
    ums.insert(42);
    ums.insert(42);
    ums.insert(42);
    EXPECT_EQ(ums.size(), 3);
    EXPECT_EQ(ums.count(42), 3);
}

TEST_F(UnorderedMultisetTest, InsertRvalue) {
    unordered_multiset<string> ums;
    string s = "hello";
    ums.insert(move(s));
    EXPECT_EQ(ums.size(), 1);
}

TEST_F(UnorderedMultisetTest, InsertRange) {
    unordered_multiset<int> ums;
    vector<int> vec = {1, 2, 2, 3};
    ums.insert(vec.begin(), vec.end());
    EXPECT_EQ(ums.size(), 4);
    EXPECT_EQ(ums.count(2), 2);
}

TEST_F(UnorderedMultisetTest, Emplace) {
    unordered_multiset<int> ums;
    auto it = ums.emplace(42);
    EXPECT_EQ(*it, 42);
}

TEST_F(UnorderedMultisetTest, EmplaceDuplicate) {
    unordered_multiset<int> ums;
    ums.emplace(42);
    ums.emplace(42);
    EXPECT_EQ(ums.size(), 2);
}

TEST_F(UnorderedMultisetTest, EraseByKey) {
    unordered_multiset<int> ums = {1, 2, 2, 3};
    size_t count = ums.erase(2);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(ums.size(), 2);
    EXPECT_EQ(ums.count(2), 0);
}

TEST_F(UnorderedMultisetTest, EraseByKeyNonExistent) {
    unordered_multiset<int> ums = {1, 2};
    size_t count = ums.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(UnorderedMultisetTest, EraseByIterator) {
    unordered_multiset<int> ums = {1, 2, 2, 3};
    auto it = ums.find(2);
    ums.erase(it);
    EXPECT_EQ(ums.size(), 3);
    EXPECT_EQ(ums.count(2), 1);
}

TEST_F(UnorderedMultisetTest, EraseRange) {
    unordered_multiset<int> ums = {1, 2, 2, 2, 3};
    auto range = ums.equal_range(2);
    ums.erase(range.first, range.second);
    EXPECT_EQ(ums.size(), 2);
    EXPECT_EQ(ums.count(2), 0);
}

TEST_F(UnorderedMultisetTest, Clear) {
    unordered_multiset<int> ums = {1, 2, 2, 3};
    ums.clear();
    EXPECT_TRUE(ums.empty());
}

TEST_F(UnorderedMultisetTest, Swap) {
    unordered_multiset<int> ums1 = {1, 2, 2};
    unordered_multiset<int> ums2 = {3, 4, 5, 6};
    ums1.swap(ums2);
    EXPECT_EQ(ums1.size(), 4);
    EXPECT_EQ(ums2.size(), 3);
}

TEST_F(UnorderedMultisetTest, Find) {
    unordered_multiset<int> ums = {1, 2, 2, 3};
    auto it = ums.find(2);
    EXPECT_NE(it, ums.end());
    EXPECT_EQ(*it, 2);
}

TEST_F(UnorderedMultisetTest, FindNonExistent) {
    unordered_multiset<int> ums = {1, 2};
    auto it = ums.find(99);
    EXPECT_EQ(it, ums.end());
}

TEST_F(UnorderedMultisetTest, ConstFind) {
    const unordered_multiset<int> ums = {1, 2, 2};
    auto it = ums.find(2);
    EXPECT_NE(it, ums.end());
}

TEST_F(UnorderedMultisetTest, EqualRange) {
    unordered_multiset<int> ums = {1, 2, 2, 2, 3};
    auto range = ums.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(*it, 2);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(UnorderedMultisetTest, ConstEqualRange) {
    const unordered_multiset<int> ums = {1, 2, 2};
    auto range = ums.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(UnorderedMultisetTest, EqualTo) {
    unordered_multiset<int> ums1 = {1, 2, 2, 3};
    unordered_multiset<int> ums2 = {2, 2, 3, 1};
    EXPECT_TRUE(ums1.equal_to(ums2));
}

TEST_F(UnorderedMultisetTest, NotEqualTo) {
    unordered_multiset<int> ums1 = {1, 2, 2};
    unordered_multiset<int> ums2 = {1, 2};
    EXPECT_FALSE(ums1.equal_to(ums2));
}

TEST_F(UnorderedMultisetTest, LessThan) {
    unordered_multiset<int> ums1 = {1, 2};
    unordered_multiset<int> ums2 = {1, 2, 3};
    EXPECT_TRUE(ums1.less_than(ums2));
}

TEST_F(UnorderedMultisetTest, EqualityOperator) {
    unordered_multiset<int> ums1 = {1, 2, 2, 3};
    unordered_multiset<int> ums2 = {3, 2, 2, 1};
    EXPECT_TRUE(ums1 == ums2);
}

TEST_F(UnorderedMultisetTest, InequalityOperator) {
    unordered_multiset<int> ums1 = {1, 2};
    unordered_multiset<int> ums2 = {3, 4};
    EXPECT_TRUE(ums1 != ums2);
}

TEST_F(UnorderedMultisetTest, StringKey) {
    unordered_multiset<string> ums;
    ums.insert("apple");
    ums.insert("banana");
    ums.insert("apple");
    ums.insert("cherry");
    EXPECT_EQ(ums.size(), 4);
    EXPECT_EQ(ums.count("apple"), 2);
    EXPECT_EQ(ums.count("banana"), 1);
}

TEST_F(UnorderedMultisetTest, LargeInsertWithDuplicates) {
    unordered_multiset<int> ums;
    const int count = 500;
    for (int i = 0; i < count; ++i) {
        ums.insert(i % 10);
    }
    EXPECT_EQ(ums.size(), count);
    EXPECT_EQ(ums.count(0), 50);
    EXPECT_EQ(ums.count(5), 50);
}
