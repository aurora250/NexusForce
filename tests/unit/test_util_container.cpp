#include <NeForce/core/container/bitmap.hpp>
#include <NeForce/core/container/bitset.hpp>
#include <NeForce/core/container/bloom_filter.hpp>
#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
using namespace neforce;

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
