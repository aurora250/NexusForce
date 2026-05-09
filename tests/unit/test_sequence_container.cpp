#include <NeForce/core/algorithm/numeric.hpp>
#include <NeForce/core/algorithm/sort.hpp>
#include <NeForce/core/container/deque.hpp>
#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/priority_queue.hpp>
#include <NeForce/core/container/queue.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/container/stack.hpp>
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
