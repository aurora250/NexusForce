#include <NeForce/core/algorithm/numeric.hpp>
#include <NeForce/core/container/map.hpp>
#include <NeForce/core/container/multimap.hpp>
#include <NeForce/core/container/multiset.hpp>
#include <NeForce/core/container/set.hpp>
#include <NeForce/core/container/unordered_map.hpp>
#include <NeForce/core/container/unordered_multimap.hpp>
#include <NeForce/core/container/unordered_multiset.hpp>
#include <NeForce/core/container/unordered_set.hpp>
#include <NeForce/core/container/sparse_map.hpp>
#include <NeForce/core/container/sparse_multimap.hpp>
#include <NeForce/core/container/sparse_multiset.hpp>
#include <NeForce/core/container/sparse_set.hpp>
#include <NeForce/core/container/ttl_cache.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/container/flat_unordered_set.hpp>
#include <NeForce/core/container/flat_unordered_multimap.hpp>
#include <NeForce/core/container/flat_unordered_multiset.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
using namespace neforce;

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
    auto hf = um.hash_function();
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
    auto hf = us.hash_function();
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
    auto hf = umm.hash_function();
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
    auto hf = ums.hash_function();
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

class SparseMapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SparseMapTest, DefaultConstructor) {
    sparse_map<int, string> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(SparseMapTest, ConstructorWithCompare) {
    sparse_map<int, string, greater<int>> m;
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

TEST_F(SparseMapTest, InitializerListConstructor) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
    EXPECT_EQ(m[2], "two");
    EXPECT_EQ(m[3], "three");
}

TEST_F(SparseMapTest, InitializerListConstructorWithCompare) {
    sparse_map<int, string, greater<int>> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.size(), 2);
}

TEST_F(SparseMapTest, InitializerListAssignment) {
    sparse_map<int, string> m;
    m = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    sparse_map<int, string> m(vec.begin(), vec.end());
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, RangeConstructorWithCompare) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    sparse_map<int, string, greater<int>> m(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(m.size(), 2);
}

TEST_F(SparseMapTest, CopyConstructor) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}, {3, "three"}};
    sparse_map<int, string> m2(m1);
    EXPECT_EQ(m2.size(), 3);
    EXPECT_EQ(m2[1], "one");
    EXPECT_EQ(m2[2], "two");
    EXPECT_EQ(m2[3], "three");
}

TEST_F(SparseMapTest, CopyAssignment) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m2 = {{3, "three"}, {4, "four"}, {5, "five"}};
    m2 = m1;
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(SparseMapTest, CopyAssignmentSelf) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    m = m;
    EXPECT_EQ(m.size(), 2);
}

TEST_F(SparseMapTest, MoveConstructor) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}, {3, "three"}};
    sparse_map<int, string> m2(move(m1));
    EXPECT_EQ(m2.size(), 3);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(SparseMapTest, MoveAssignment) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m2 = {{3, "three"}};
    m2 = move(m1);
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(SparseMapTest, MoveAssignmentSelf) {
    sparse_map<int, string> m = {{1, "one"}};
    m = move(m);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(SparseMapTest, BeginEnd) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
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

TEST_F(SparseMapTest, ConstBeginEnd) {
    const sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.begin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(SparseMapTest, CbeginCend) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.cbegin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(SparseMapTest, ReverseBeginEnd) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto rit = m.rbegin();
    EXPECT_EQ(rit->first, 3);
    ++rit;
    EXPECT_EQ(rit->first, 2);
    ++rit;
    EXPECT_EQ(rit->first, 1);
    ++rit;
    EXPECT_EQ(rit, m.rend());
}

TEST_F(SparseMapTest, ConstReverseBeginEnd) {
    const sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto rit = m.rbegin();
    EXPECT_EQ(rit->first, 2);
}

TEST_F(SparseMapTest, CrbeginCrend) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto rit = m.crbegin();
    EXPECT_EQ(rit->first, 2);
}

TEST_F(SparseMapTest, Size) {
    sparse_map<int, int> m;
    EXPECT_EQ(m.size(), 0);
    m.insert({1, 10});
    EXPECT_EQ(m.size(), 1);
    m.insert({2, 20});
    EXPECT_EQ(m.size(), 2);
}

TEST_F(SparseMapTest, MaxSize) {
    sparse_map<int, int> m;
    EXPECT_GT(m.max_size(), 0);
}

TEST_F(SparseMapTest, Empty) {
    sparse_map<int, int> m;
    EXPECT_TRUE(m.empty());
    m.insert({1, 10});
    EXPECT_FALSE(m.empty());
    m.erase(1);
    EXPECT_TRUE(m.empty());
}

TEST_F(SparseMapTest, KeyComp) {
    sparse_map<int, string> m;
    auto comp = m.key_comp();
    EXPECT_TRUE(comp(1, 2));
    EXPECT_FALSE(comp(2, 1));
    EXPECT_FALSE(comp(1, 1));
}

TEST_F(SparseMapTest, ValueComp) {
    sparse_map<int, string> m;
    auto comp = m.value_comp();
    pair<const int, string> p1(1, "a");
    pair<const int, string> p2(2, "b");
    EXPECT_TRUE(comp(p1, p2));
}

TEST_F(SparseMapTest, InsertValue) {
    sparse_map<int, string> m;
    auto result = m.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
}

TEST_F(SparseMapTest, InsertDuplicateKey) {
    sparse_map<int, string> m;
    m.insert({1, "one"});
    auto result = m.insert({1, "uno"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, InsertRvalue) {
    sparse_map<int, string> m;
    pair<int, string> p(1, "one");
    m.insert(move(p));
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, InsertWithHint) {
    sparse_map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.insert(m.begin(), {2, "two"});
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(m.size(), 3);
}

TEST_F(SparseMapTest, InsertWithHintRvalue) {
    sparse_map<int, string> m = {{1, "one"}, {3, "three"}};
    pair<int, string> p(2, "two");
    auto it = m.insert(m.begin(), move(p));
    EXPECT_EQ(it->first, 2);
}

TEST_F(SparseMapTest, InsertRange) {
    sparse_map<int, string> m;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    m.insert(vec.begin(), vec.end());
    EXPECT_EQ(m.size(), 3);
}

TEST_F(SparseMapTest, Emplace) {
    sparse_map<int, string> m;
    auto result = m.emplace(1, "one");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, EmplaceDuplicate) {
    sparse_map<int, string> m;
    m.emplace(1, "one");
    auto result = m.emplace(1, "uno");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, EmplaceHint) {
    sparse_map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.emplace_hint(m.begin(), 2, "two");
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(m.size(), 3);
}

TEST_F(SparseMapTest, EraseByIterator) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = m.find(2);
    m.erase(it);
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m.find(2), m.end());
}

TEST_F(SparseMapTest, EraseByKey) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    size_t count = m.erase(1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(SparseMapTest, EraseByNonExistentKey) {
    sparse_map<int, string> m = {{1, "one"}};
    size_t count = m.erase(99);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(SparseMapTest, EraseRange) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};
    auto first = m.find(2);
    auto last = m.find(4);
    m.erase(first, last);
    EXPECT_EQ(m.size(), 2);
    EXPECT_NE(m.find(1), m.end());
    EXPECT_NE(m.find(4), m.end());
}

TEST_F(SparseMapTest, Clear) {
    sparse_map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    m.clear();
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(SparseMapTest, Find) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = m.find(2);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, "two");
}

TEST_F(SparseMapTest, FindNonExistent) {
    sparse_map<int, string> m = {{1, "one"}};
    auto it = m.find(99);
    EXPECT_EQ(it, m.end());
}

TEST_F(SparseMapTest, ConstFind) {
    const sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    EXPECT_NE(it, m.end());
}

TEST_F(SparseMapTest, Count) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.count(1), 1);
    EXPECT_EQ(m.count(99), 0);
}

TEST_F(SparseMapTest, LowerBound) {
    sparse_map<int, string> m = {{1, "one"}, {3, "three"}, {5, "five"}};
    auto it = m.lower_bound(3);
    EXPECT_EQ(it->first, 3);
    it = m.lower_bound(4);
    EXPECT_EQ(it->first, 5);
    it = m.lower_bound(6);
    EXPECT_EQ(it, m.end());
}

TEST_F(SparseMapTest, ConstLowerBound) {
    const sparse_map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.lower_bound(2);
    EXPECT_EQ(it->first, 3);
}

TEST_F(SparseMapTest, UpperBound) {
    sparse_map<int, string> m = {{1, "one"}, {3, "three"}, {5, "five"}};
    auto it = m.upper_bound(3);
    EXPECT_EQ(it->first, 5);
    it = m.upper_bound(5);
    EXPECT_EQ(it, m.end());
}

TEST_F(SparseMapTest, ConstUpperBound) {
    const sparse_map<int, string> m = {{1, "one"}, {3, "three"}};
    auto it = m.upper_bound(1);
    EXPECT_EQ(it->first, 3);
}

TEST_F(SparseMapTest, EqualRange) {
    sparse_map<int, string> m = {{1, "one"}, {3, "three"}, {5, "five"}};
    auto range = m.equal_range(3);
    EXPECT_EQ(range.first->first, 3);
    EXPECT_EQ(range.second->first, 5);
}

TEST_F(SparseMapTest, ConstEqualRange) {
    const sparse_map<int, string> m = {{1, "one"}, {3, "three"}};
    auto range = m.equal_range(1);
    EXPECT_EQ(range.first->first, 1);
}

TEST_F(SparseMapTest, SubscriptOperator) {
    sparse_map<int, string> m;
    m[1] = "one";
    EXPECT_EQ(m[1], "one");
    m[1] = "uno";
    EXPECT_EQ(m[1], "uno");
}

TEST_F(SparseMapTest, SubscriptOperatorInsertDefault) {
    sparse_map<int, int> m;
    EXPECT_EQ(m[1], 0);
    m[1] = 42;
    EXPECT_EQ(m[1], 42);
}

TEST_F(SparseMapTest, SubscriptOperatorRvalueKey) {
    sparse_map<int, string> m;
    m[1] = "one";
    EXPECT_EQ(m[1], "one");
}

TEST_F(SparseMapTest, At) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.at(1), "one");
    m.at(1) = "uno";
    EXPECT_EQ(m.at(1), "uno");
}

TEST_F(SparseMapTest, ConstAt) {
    const sparse_map<int, string> m = {{1, "one"}};
    EXPECT_EQ(m.at(1), "one");
}

TEST_F(SparseMapTest, AtNonExistent) {
    sparse_map<int, string> m;
    EXPECT_THROW(ignore = m.at(1), value_exception);
}

TEST_F(SparseMapTest, Swap) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m2 = {{3, "three"}, {4, "four"}, {5, "five"}};
    m1.swap(m2);
    EXPECT_EQ(m1.size(), 3);
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(SparseMapTest, EqualTo) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m2 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m3 = {{1, "one"}, {2, "deux"}};
    EXPECT_TRUE(m1.equal_to(m2));
    EXPECT_FALSE(m1.equal_to(m3));
}

TEST_F(SparseMapTest, LessThan) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m2 = {{1, "one"}, {3, "three"}};
    EXPECT_TRUE(m1.less_than(m2));
    EXPECT_FALSE(m2.less_than(m1));
}

TEST_F(SparseMapTest, EqualityOperator) {
    sparse_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m2 = {{1, "one"}, {2, "two"}};
    sparse_map<int, string> m3 = {{1, "one"}};
    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 == m3);
}

TEST_F(SparseMapTest, InequalityOperator) {
    sparse_map<int, string> m1 = {{1, "one"}};
    sparse_map<int, string> m2 = {{2, "two"}};
    EXPECT_TRUE(m1 != m2);
}

TEST_F(SparseMapTest, LessThanOperator) {
    sparse_map<int, string> m1 = {{1, "one"}};
    sparse_map<int, string> m2 = {{2, "two"}};
    EXPECT_TRUE(m1 < m2);
}

TEST_F(SparseMapTest, GreaterThanOperator) {
    sparse_map<int, string> m1 = {{2, "two"}};
    sparse_map<int, string> m2 = {{1, "one"}};
    EXPECT_TRUE(m1 > m2);
}

TEST_F(SparseMapTest, StringKey) {
    sparse_map<string, int> m;
    m["hello"] = 1;
    m["world"] = 2;
    EXPECT_EQ(m["hello"], 1);
    EXPECT_EQ(m["world"], 2);
    EXPECT_EQ(m.size(), 2);
}

TEST_F(SparseMapTest, LargeInsert) {
    sparse_map<int, int> m;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        m.insert({i, i * 10});
    }
    EXPECT_EQ(m.size(), count);
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(m[i], i * 10);
    }
}

TEST_F(SparseMapTest, OrderPreservation) {
    sparse_map<int, string> m;
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

TEST_F(SparseMapTest, RangeBasedForLoop) {
    sparse_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    int sum = 0;
    for (const auto& p: m) {
        sum += p.first;
    }
    EXPECT_EQ(sum, 6);
}

TEST_F(SparseMapTest, Reserve) {
    sparse_map<int, string> m;
    m.reserve(100);
    EXPECT_GE(m.capacity(), 100);
}

TEST_F(SparseMapTest, ShrinkToFit) {
    sparse_map<int, string> m;
    m.reserve(100);
    m.insert({1, "one"});
    m.insert({2, "two"});
    m.shrink_to_fit();
    EXPECT_LE(m.capacity(), 100);
}

class SparseSetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SparseSetTest, DefaultConstructor) {
    sparse_set<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(SparseSetTest, ConstructorWithCompare) {
    sparse_set<int, greater<int>> s;
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

TEST_F(SparseSetTest, InitializerListConstructor) {
    sparse_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(s.size(), 5);
    EXPECT_NE(s.find(1), s.end());
    EXPECT_NE(s.find(5), s.end());
}

TEST_F(SparseSetTest, InitializerListConstructorWithCompare) {
    sparse_set<int, greater<int>> s = {1, 2, 3};
    EXPECT_EQ(s.size(), 3);
    auto it = s.begin();
    EXPECT_EQ(*it, 3);
}

TEST_F(SparseSetTest, InitializerListAssignment) {
    sparse_set<int> s;
    s = {10, 20, 30};
    EXPECT_EQ(s.size(), 3);
    EXPECT_NE(s.find(10), s.end());
}

TEST_F(SparseSetTest, RangeConstructor) {
    vector<int> vec = {5, 2, 8, 1, 9};
    sparse_set<int> s(vec.begin(), vec.end());
    EXPECT_EQ(s.size(), 5);
    vector<int> expected = {1, 2, 5, 8, 9};
    auto it = expected.begin();
    for (auto val: s) {
        EXPECT_EQ(val, *it);
        ++it;
    }
}

TEST_F(SparseSetTest, RangeConstructorWithCompare) {
    vector<int> vec = {1, 2, 3};
    sparse_set<int, greater<int>> s(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(s.size(), 3);
}

TEST_F(SparseSetTest, CopyConstructor) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2(s1);
    EXPECT_EQ(s2.size(), 3);
    EXPECT_NE(s2.find(1), s2.end());
}

TEST_F(SparseSetTest, CopyAssignment) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2 = {4, 5};
    s2 = s1;
    EXPECT_EQ(s2.size(), 3);
    EXPECT_NE(s2.find(1), s2.end());
}

TEST_F(SparseSetTest, MoveConstructor) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2(move(s1));
    EXPECT_EQ(s2.size(), 3);
    EXPECT_NE(s2.find(2), s2.end());
}

TEST_F(SparseSetTest, MoveAssignment) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2 = {4, 5};
    s2 = move(s1);
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(SparseSetTest, BeginEnd) {
    sparse_set<int> s = {3, 1, 2};
    auto it = s.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, s.end());
}

TEST_F(SparseSetTest, CbeginCend) {
    sparse_set<int> s = {3, 1, 2};
    auto it = s.cbegin();
    EXPECT_EQ(*it, 1);
}

TEST_F(SparseSetTest, ReverseBeginEnd) {
    sparse_set<int> s = {1, 2, 3};
    auto rit = s.rbegin();
    EXPECT_EQ(*rit, 3);
    ++rit;
    EXPECT_EQ(*rit, 2);
    ++rit;
    EXPECT_EQ(*rit, 1);
    ++rit;
    EXPECT_EQ(rit, s.rend());
}

TEST_F(SparseSetTest, CrbeginCrend) {
    sparse_set<int> s = {1, 2, 3};
    auto rit = s.crbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(SparseSetTest, Size) {
    sparse_set<int> s;
    EXPECT_EQ(s.size(), 0);
    s.insert(1);
    EXPECT_EQ(s.size(), 1);
    s.insert(2);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(SparseSetTest, MaxSize) {
    sparse_set<int> s;
    EXPECT_GT(s.max_size(), 0);
}

TEST_F(SparseSetTest, Empty) {
    sparse_set<int> s;
    EXPECT_TRUE(s.empty());
    s.insert(1);
    EXPECT_FALSE(s.empty());
    s.erase(1);
    EXPECT_TRUE(s.empty());
}

TEST_F(SparseSetTest, KeyComp) {
    sparse_set<int> s;
    auto comp = s.key_comp();
    EXPECT_TRUE(comp(1, 2));
    EXPECT_FALSE(comp(2, 1));
}

TEST_F(SparseSetTest, ValueComp) {
    sparse_set<int> s;
    auto comp = s.value_comp();
    EXPECT_TRUE(comp(1, 2));
}

TEST_F(SparseSetTest, InsertValue) {
    sparse_set<int> s;
    auto result = s.insert(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(SparseSetTest, InsertDuplicate) {
    sparse_set<int> s;
    s.insert(42);
    auto result = s.insert(42);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(s.size(), 1);
}

TEST_F(SparseSetTest, InsertRvalue) {
    sparse_set<string> s;
    string str = "hello";
    s.insert(move(str));
    EXPECT_EQ(s.size(), 1);
}

TEST_F(SparseSetTest, InsertWithHint) {
    sparse_set<int> s = {1, 3, 5};
    auto it = s.insert(s.begin(), 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(s.size(), 4);
}

TEST_F(SparseSetTest, InsertWithHintRvalue) {
    sparse_set<int> s = {1, 3};
    auto it = s.insert(s.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseSetTest, InsertRange) {
    sparse_set<int> s;
    vector<int> vec = {5, 2, 8, 1, 9};
    s.insert(vec.begin(), vec.end());
    EXPECT_EQ(s.size(), 5);
}

TEST_F(SparseSetTest, Emplace) {
    sparse_set<int> s;
    auto result = s.emplace(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(SparseSetTest, EmplaceDuplicate) {
    sparse_set<int> s;
    s.emplace(42);
    auto result = s.emplace(42);
    EXPECT_FALSE(result.second);
}

TEST_F(SparseSetTest, EmplaceHint) {
    sparse_set<int> s = {1, 3, 5};
    auto it = s.emplace_hint(s.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseSetTest, EraseByIterator) {
    sparse_set<int> s = {1, 2, 3};
    auto it = s.find(2);
    s.erase(it);
    EXPECT_EQ(s.size(), 2);
    EXPECT_EQ(s.find(2), s.end());
}

TEST_F(SparseSetTest, EraseByKey) {
    sparse_set<int> s = {1, 2, 3};
    size_t count = s.erase(2);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(SparseSetTest, EraseByNonExistentKey) {
    sparse_set<int> s = {1, 2};
    size_t count = s.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(SparseSetTest, EraseRange) {
    sparse_set<int> s = {1, 2, 3, 4, 5};
    auto first = s.find(2);
    auto last = s.find(4);
    s.erase(first, last);
    EXPECT_EQ(s.size(), 3);
    EXPECT_NE(s.find(1), s.end());
    EXPECT_NE(s.find(4), s.end());
    EXPECT_NE(s.find(5), s.end());
}

TEST_F(SparseSetTest, Clear) {
    sparse_set<int> s = {1, 2, 3};
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(SparseSetTest, Find) {
    sparse_set<int> s = {1, 2, 3};
    auto it = s.find(2);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseSetTest, FindNonExistent) {
    sparse_set<int> s = {1, 2};
    auto it = s.find(99);
    EXPECT_EQ(it, s.end());
}

TEST_F(SparseSetTest, ConstFind) {
    const sparse_set<int> s = {1, 2, 3};
    auto it = s.find(1);
    EXPECT_NE(it, s.end());
}

TEST_F(SparseSetTest, Count) {
    sparse_set<int> s = {1, 2, 3};
    EXPECT_EQ(s.count(2), 1);
    EXPECT_EQ(s.count(99), 0);
}

TEST_F(SparseSetTest, LowerBound) {
    sparse_set<int> s = {1, 3, 5, 7};
    auto it = s.lower_bound(3);
    EXPECT_EQ(*it, 3);
    it = s.lower_bound(4);
    EXPECT_EQ(*it, 5);
    it = s.lower_bound(8);
    EXPECT_EQ(it, s.end());
}

TEST_F(SparseSetTest, ConstLowerBound) {
    const sparse_set<int> s = {1, 3, 5};
    auto it = s.lower_bound(2);
    EXPECT_EQ(*it, 3);
}

TEST_F(SparseSetTest, UpperBound) {
    sparse_set<int> s = {1, 3, 5, 7};
    auto it = s.upper_bound(3);
    EXPECT_EQ(*it, 5);
    it = s.upper_bound(7);
    EXPECT_EQ(it, s.end());
}

TEST_F(SparseSetTest, ConstUpperBound) {
    const sparse_set<int> s = {1, 3, 5};
    auto it = s.upper_bound(1);
    EXPECT_EQ(*it, 3);
}

TEST_F(SparseSetTest, EqualRange) {
    sparse_set<int> s = {1, 3, 5, 7};
    auto range = s.equal_range(3);
    EXPECT_EQ(*range.first, 3);
    EXPECT_EQ(*range.second, 5);
}

TEST_F(SparseSetTest, ConstEqualRange) {
    const sparse_set<int> s = {1, 3, 5};
    auto range = s.equal_range(1);
    EXPECT_EQ(*range.first, 1);
}

TEST_F(SparseSetTest, Swap) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2 = {4, 5, 6, 7};
    s1.swap(s2);
    EXPECT_EQ(s1.size(), 4);
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(SparseSetTest, EqualTo) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2 = {1, 2, 3};
    sparse_set<int> s3 = {1, 2, 4};
    EXPECT_TRUE(s1.equal_to(s2));
    EXPECT_FALSE(s1.equal_to(s3));
}

TEST_F(SparseSetTest, LessThan) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2 = {1, 2, 4};
    EXPECT_TRUE(s1.less_than(s2));
}

TEST_F(SparseSetTest, EqualityOperator) {
    sparse_set<int> s1 = {1, 2, 3};
    sparse_set<int> s2 = {1, 2, 3};
    sparse_set<int> s3 = {1, 2};
    EXPECT_TRUE(s1 == s2);
    EXPECT_FALSE(s1 == s3);
}

TEST_F(SparseSetTest, InequalityOperator) {
    sparse_set<int> s1 = {1, 2};
    sparse_set<int> s2 = {3, 4};
    EXPECT_TRUE(s1 != s2);
}

TEST_F(SparseSetTest, LessThanOperator) {
    sparse_set<int> s1 = {1, 2};
    sparse_set<int> s2 = {1, 3};
    EXPECT_TRUE(s1 < s2);
}

TEST_F(SparseSetTest, GreaterThanOperator) {
    sparse_set<int> s1 = {1, 3};
    sparse_set<int> s2 = {1, 2};
    EXPECT_TRUE(s1 > s2);
}

TEST_F(SparseSetTest, StringKey) {
    sparse_set<string> s;
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

TEST_F(SparseSetTest, LargeInsert) {
    sparse_set<int> s;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        s.insert(i * 2);
    }
    EXPECT_EQ(s.size(), count);
    EXPECT_EQ(*s.begin(), 0);
    EXPECT_EQ(*s.rbegin(), (count - 1) * 2);
}

TEST_F(SparseSetTest, DuplicateValuesIgnored) {
    sparse_set<int> s;
    s.insert(1);
    s.insert(1);
    s.insert(1);
    EXPECT_EQ(s.size(), 1);
}

TEST_F(SparseSetTest, RangeBasedForLoop) {
    sparse_set<int> s = {1, 2, 3, 4, 5};
    int sum = 0;
    for (auto val: s) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST_F(SparseSetTest, Reserve) {
    sparse_set<int> s;
    s.reserve(100);
    EXPECT_GE(s.capacity(), 100);
}

class SparseMultimapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SparseMultimapTest, DefaultConstructor) {
    sparse_multimap<int, string> mm;
    EXPECT_TRUE(mm.empty());
    EXPECT_EQ(mm.size(), 0);
}

TEST_F(SparseMultimapTest, ConstructorWithCompare) {
    sparse_multimap<int, string, greater<int>> mm;
    EXPECT_TRUE(mm.empty());
    mm.insert({3, "three"});
    mm.insert({1, "one"});
    mm.insert({2, "two"});
    auto it = mm.begin();
    EXPECT_EQ(it->first, 3);
}

TEST_F(SparseMultimapTest, InitializerListConstructor) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    EXPECT_EQ(mm.size(), 4);
}

TEST_F(SparseMultimapTest, InitializerListAssignment) {
    sparse_multimap<int, string> mm;
    mm = {{1, "one"}, {2, "two"}, {2, "second"}};
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(SparseMultimapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    sparse_multimap<int, string> mm(vec.begin(), vec.end());
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(SparseMultimapTest, RangeConstructorWithCompare) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    sparse_multimap<int, string, greater<int>> mm(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(mm.size(), 2);
}

TEST_F(SparseMultimapTest, CopyConstructor) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    sparse_multimap<int, string> mm2(mm1);
    EXPECT_EQ(mm2.size(), 3);
}

TEST_F(SparseMultimapTest, CopyAssignment) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    sparse_multimap<int, string> mm2 = {{3, "three"}};
    mm2 = mm1;
    EXPECT_EQ(mm2.size(), 2);
}

TEST_F(SparseMultimapTest, MoveConstructor) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    sparse_multimap<int, string> mm2(move(mm1));
    EXPECT_EQ(mm2.size(), 2);
}

TEST_F(SparseMultimapTest, MoveAssignment) {
    sparse_multimap<int, string> mm1 = {{1, "one"}};
    sparse_multimap<int, string> mm2 = {{2, "two"}};
    mm2 = move(mm1);
    EXPECT_EQ(mm2.size(), 1);
}

TEST_F(SparseMultimapTest, BeginEnd) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = mm.begin();
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 3);
    ++it;
    EXPECT_EQ(it, mm.end());
}

TEST_F(SparseMultimapTest, CbeginCend) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}};
    auto it = mm.cbegin();
    EXPECT_EQ(it->first, 1);
}

TEST_F(SparseMultimapTest, ReverseBeginEnd) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto rit = mm.rbegin();
    EXPECT_EQ(rit->first, 3);
    ++rit;
    EXPECT_EQ(rit->first, 2);
}

TEST_F(SparseMultimapTest, Size) {
    sparse_multimap<int, string> mm;
    EXPECT_EQ(mm.size(), 0);
    mm.insert({1, "one"});
    EXPECT_EQ(mm.size(), 1);
}

TEST_F(SparseMultimapTest, MaxSize) {
    sparse_multimap<int, int> mm;
    EXPECT_GT(mm.max_size(), 0);
}

TEST_F(SparseMultimapTest, Empty) {
    sparse_multimap<int, string> mm;
    EXPECT_TRUE(mm.empty());
    mm.insert({1, "one"});
    EXPECT_FALSE(mm.empty());
}

TEST_F(SparseMultimapTest, KeyComp) {
    sparse_multimap<int, string> mm;
    auto comp = mm.key_comp();
    EXPECT_TRUE(comp(1, 2));
}

TEST_F(SparseMultimapTest, ValueComp) {
    sparse_multimap<int, string> mm;
    auto comp = mm.value_comp();
    pair<const int, string> p1(1, "a");
    pair<const int, string> p2(2, "b");
    EXPECT_TRUE(comp(p1, p2));
}

TEST_F(SparseMultimapTest, InsertValue) {
    sparse_multimap<int, string> mm;
    auto it = mm.insert({1, "one"});
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(SparseMultimapTest, InsertDuplicateKeys) {
    sparse_multimap<int, string> mm;
    mm.insert({1, "one"});
    mm.insert({1, "uno"});
    mm.insert({1, "eins"});
    EXPECT_EQ(mm.size(), 3);
    EXPECT_EQ(mm.count(1), 3);
}

TEST_F(SparseMultimapTest, InsertRvalue) {
    sparse_multimap<int, string> mm;
    pair<int, string> p(1, "one");
    mm.insert(move(p));
    EXPECT_EQ(mm.size(), 1);
}

TEST_F(SparseMultimapTest, InsertWithHint) {
    sparse_multimap<int, string> mm = {{1, "one"}, {3, "three"}};
    auto it = mm.insert(mm.begin(), {2, "two"});
    EXPECT_EQ(it->first, 2);
}

TEST_F(SparseMultimapTest, InsertRange) {
    sparse_multimap<int, string> mm;
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    mm.insert(vec.begin(), vec.end());
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(SparseMultimapTest, Emplace) {
    sparse_multimap<int, string> mm;
    auto it = mm.emplace(1, "one");
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(SparseMultimapTest, EmplaceDuplicate) {
    sparse_multimap<int, string> mm;
    mm.emplace(1, "one");
    auto it = mm.emplace(1, "uno");
    EXPECT_EQ(mm.size(), 2);
    EXPECT_EQ(it->second, "uno");
}

TEST_F(SparseMultimapTest, EmplaceHint) {
    sparse_multimap<int, string> mm = {{1, "one"}, {3, "three"}};
    auto it = mm.emplace_hint(mm.begin(), 2, "two");
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(mm.size(), 3);
}

TEST_F(SparseMultimapTest, EraseByIterator) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = mm.find(2);
    mm.erase(it);
    EXPECT_EQ(mm.size(), 2);
}

TEST_F(SparseMultimapTest, EraseByKey) {
    sparse_multimap<int, string> mm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    size_t count = mm.erase(1);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(mm.size(), 1);
}

TEST_F(SparseMultimapTest, EraseByNonExistentKey) {
    sparse_multimap<int, string> mm = {{1, "one"}};
    size_t count = mm.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(SparseMultimapTest, EraseRange) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto range = mm.equal_range(2);
    mm.erase(range.first, range.second);
    EXPECT_EQ(mm.size(), 2);
    EXPECT_EQ(mm.count(2), 0);
}

TEST_F(SparseMultimapTest, Clear) {
    sparse_multimap<int, int> mm = {{1, 10}, {2, 20}, {2, 30}};
    mm.clear();
    EXPECT_TRUE(mm.empty());
}

TEST_F(SparseMultimapTest, Find) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}};
    auto it = mm.find(2);
    EXPECT_NE(it, mm.end());
    EXPECT_EQ(it->first, 2);
}

TEST_F(SparseMultimapTest, FindNonExistent) {
    sparse_multimap<int, string> mm = {{1, "one"}};
    auto it = mm.find(99);
    EXPECT_EQ(it, mm.end());
}

TEST_F(SparseMultimapTest, ConstFind) {
    const sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}};
    auto it = mm.find(1);
    EXPECT_NE(it, mm.end());
}

TEST_F(SparseMultimapTest, Count) {
    sparse_multimap<int, string> mm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    EXPECT_EQ(mm.count(1), 2);
    EXPECT_EQ(mm.count(2), 1);
    EXPECT_EQ(mm.count(99), 0);
}

TEST_F(SparseMultimapTest, LowerBound) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto it = mm.lower_bound(2);
    EXPECT_EQ(it->first, 2);
    it = mm.lower_bound(0);
    EXPECT_EQ(it->first, 1);
}

TEST_F(SparseMultimapTest, UpperBound) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto it = mm.upper_bound(2);
    EXPECT_EQ(it->first, 3);
}

TEST_F(SparseMultimapTest, EqualRange) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}, {3, "three"}};
    auto range = mm.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(it->first, 2);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(SparseMultimapTest, ConstEqualRange) {
    const sparse_multimap<int, string> mm = {{1, "one"}, {1, "uno"}, {2, "two"}};
    auto range = mm.equal_range(1);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(SparseMultimapTest, Swap) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    sparse_multimap<int, string> mm2 = {{3, "three"}, {4, "four"}};
    mm1.swap(mm2);
    EXPECT_EQ(mm1.size(), 2);
    EXPECT_EQ(mm2.size(), 2);
}

TEST_F(SparseMultimapTest, EqualTo) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    sparse_multimap<int, string> mm2 = {{1, "one"}, {2, "two"}, {2, "deux"}};
    sparse_multimap<int, string> mm3 = {{1, "one"}, {2, "two"}};
    EXPECT_TRUE(mm1.equal_to(mm2));
    EXPECT_FALSE(mm1.equal_to(mm3));
}

TEST_F(SparseMultimapTest, LessThan) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    sparse_multimap<int, string> mm2 = {{1, "one"}, {3, "three"}};
    EXPECT_TRUE(mm1.less_than(mm2));
}

TEST_F(SparseMultimapTest, EqualityOperator) {
    sparse_multimap<int, string> mm1 = {{1, "one"}, {2, "two"}};
    sparse_multimap<int, string> mm2 = {{1, "one"}, {2, "two"}};
    EXPECT_TRUE(mm1 == mm2);
}

TEST_F(SparseMultimapTest, LessThanOperator) {
    sparse_multimap<int, string> mm1 = {{1, "one"}};
    sparse_multimap<int, string> mm2 = {{2, "two"}};
    EXPECT_TRUE(mm1 < mm2);
}

TEST_F(SparseMultimapTest, DuplicateKeyOrderPreserved) {
    sparse_multimap<int, string> mm;
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

TEST_F(SparseMultimapTest, StringKey) {
    sparse_multimap<string, int> mm;
    mm.insert({"apple", 1});
    mm.insert({"apple", 2});
    mm.insert({"banana", 3});
    EXPECT_EQ(mm.size(), 3);
    EXPECT_EQ(mm.count("apple"), 2);
    EXPECT_EQ(mm.count("banana"), 1);
}

TEST_F(SparseMultimapTest, LargeInsertWithDuplicates) {
    sparse_multimap<int, int> mm;
    const int count = 500;
    for (int i = 0; i < count; ++i) {
        mm.insert({i % 10, i});
    }
    EXPECT_EQ(mm.size(), count);
    EXPECT_EQ(mm.count(0), 50);
    EXPECT_EQ(mm.count(5), 50);
}

TEST_F(SparseMultimapTest, RangeBasedForLoop) {
    sparse_multimap<int, string> mm = {{1, "one"}, {2, "two"}, {2, "deux"}};
    int keySum = 0;
    for (const auto& p: mm) {
        keySum += p.first;
    }
    EXPECT_EQ(keySum, 5);
}

TEST_F(SparseMultimapTest, Reserve) {
    sparse_multimap<int, string> mm;
    mm.reserve(100);
    EXPECT_GE(mm.capacity(), 100);
}

class SparseMultisetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SparseMultisetTest, DefaultConstructor) {
    sparse_multiset<int> ms;
    EXPECT_TRUE(ms.empty());
    EXPECT_EQ(ms.size(), 0);
}

TEST_F(SparseMultisetTest, ConstructorWithCompare) {
    sparse_multiset<int, greater<int>> ms;
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

TEST_F(SparseMultisetTest, InitializerListConstructor) {
    sparse_multiset<int> ms = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(ms.size(), 6);
}

TEST_F(SparseMultisetTest, InitializerListConstructorWithCompare) {
    sparse_multiset<int, greater<int>> ms = {1, 2, 2, 3};
    EXPECT_EQ(ms.size(), 4);
    auto it = ms.begin();
    EXPECT_EQ(*it, 3);
}

TEST_F(SparseMultisetTest, InitializerListAssignment) {
    sparse_multiset<int> ms;
    ms = {10, 20, 20, 30};
    EXPECT_EQ(ms.size(), 4);
}

TEST_F(SparseMultisetTest, RangeConstructor) {
    vector<int> vec = {5, 2, 8, 2, 1, 9};
    sparse_multiset<int> ms(vec.begin(), vec.end());
    EXPECT_EQ(ms.size(), 6);
    auto it = ms.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseMultisetTest, RangeConstructorWithCompare) {
    vector<int> vec = {1, 2, 2, 3};
    sparse_multiset<int, greater<int>> ms(vec.begin(), vec.end(), greater<int>());
    EXPECT_EQ(ms.size(), 4);
}

TEST_F(SparseMultisetTest, CopyConstructor) {
    sparse_multiset<int> ms1 = {1, 2, 2, 3};
    sparse_multiset<int> ms2(ms1);
    EXPECT_EQ(ms2.size(), 4);
    EXPECT_EQ(ms2.count(2), 2);
}

TEST_F(SparseMultisetTest, CopyAssignment) {
    sparse_multiset<int> ms1 = {1, 2, 2};
    sparse_multiset<int> ms2 = {4, 5};
    ms2 = ms1;
    EXPECT_EQ(ms2.size(), 3);
    EXPECT_EQ(ms2.count(2), 2);
}

TEST_F(SparseMultisetTest, MoveConstructor) {
    sparse_multiset<int> ms1 = {1, 2, 2, 3};
    sparse_multiset<int> ms2(move(ms1));
    EXPECT_EQ(ms2.size(), 4);
}

TEST_F(SparseMultisetTest, MoveAssignment) {
    sparse_multiset<int> ms1 = {1, 2, 2};
    sparse_multiset<int> ms2 = {4, 5};
    ms2 = move(ms1);
    EXPECT_EQ(ms2.size(), 3);
}

TEST_F(SparseMultisetTest, BeginEnd) {
    sparse_multiset<int> ms = {3, 1, 2, 2};
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

TEST_F(SparseMultisetTest, CbeginCend) {
    sparse_multiset<int> ms = {3, 1, 2};
    auto it = ms.cbegin();
    EXPECT_EQ(*it, 1);
}

TEST_F(SparseMultisetTest, ReverseBeginEnd) {
    sparse_multiset<int> ms = {1, 2, 2, 3};
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

TEST_F(SparseMultisetTest, CrbeginCrend) {
    sparse_multiset<int> ms = {1, 2, 3};
    auto rit = ms.crbegin();
    EXPECT_EQ(*rit, 3);
}

TEST_F(SparseMultisetTest, Size) {
    sparse_multiset<int> ms;
    EXPECT_EQ(ms.size(), 0);
    ms.insert(1);
    EXPECT_EQ(ms.size(), 1);
    ms.insert(1);
    EXPECT_EQ(ms.size(), 2);
}

TEST_F(SparseMultisetTest, MaxSize) {
    sparse_multiset<int> ms;
    EXPECT_GT(ms.max_size(), 0);
}

TEST_F(SparseMultisetTest, Empty) {
    sparse_multiset<int> ms;
    EXPECT_TRUE(ms.empty());
    ms.insert(1);
    EXPECT_FALSE(ms.empty());
    ms.erase(1);
    EXPECT_TRUE(ms.empty());
}

TEST_F(SparseMultisetTest, KeyComp) {
    sparse_multiset<int> ms;
    auto comp = ms.key_comp();
    EXPECT_TRUE(comp(1, 2));
    EXPECT_FALSE(comp(2, 1));
}

TEST_F(SparseMultisetTest, ValueComp) {
    sparse_multiset<int> ms;
    auto comp = ms.value_comp();
    EXPECT_TRUE(comp(1, 2));
}

TEST_F(SparseMultisetTest, InsertValue) {
    sparse_multiset<int> ms;
    auto it = ms.insert(42);
    EXPECT_EQ(*it, 42);
    EXPECT_EQ(ms.size(), 1);
}

TEST_F(SparseMultisetTest, InsertDuplicate) {
    sparse_multiset<int> ms;
    ms.insert(42);
    ms.insert(42);
    ms.insert(42);
    EXPECT_EQ(ms.size(), 3);
    EXPECT_EQ(ms.count(42), 3);
}

TEST_F(SparseMultisetTest, InsertRvalue) {
    sparse_multiset<string> ms;
    string str = "hello";
    ms.insert(move(str));
    EXPECT_EQ(ms.size(), 1);
}

TEST_F(SparseMultisetTest, InsertWithHint) {
    sparse_multiset<int> ms = {1, 2, 3};
    auto it = ms.insert(ms.begin(), 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(ms.size(), 4);
}

TEST_F(SparseMultisetTest, InsertWithHintRvalue) {
    sparse_multiset<int> ms = {1, 3};
    auto it = ms.insert(ms.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseMultisetTest, InsertRange) {
    sparse_multiset<int> ms;
    vector<int> vec = {5, 2, 8, 2, 1, 9};
    ms.insert(vec.begin(), vec.end());
    EXPECT_EQ(ms.size(), 6);
    EXPECT_EQ(ms.count(2), 2);
}

TEST_F(SparseMultisetTest, Emplace) {
    sparse_multiset<int> ms;
    auto it = ms.emplace(42);
    EXPECT_EQ(*it, 42);
}

TEST_F(SparseMultisetTest, EmplaceDuplicate) {
    sparse_multiset<int> ms;
    ms.emplace(42);
    ms.emplace(42);
    EXPECT_EQ(ms.size(), 2);
}

TEST_F(SparseMultisetTest, EmplaceHint) {
    sparse_multiset<int> ms = {1, 3};
    auto it = ms.emplace_hint(ms.begin(), 2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseMultisetTest, EraseByIterator) {
    sparse_multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.find(2);
    ms.erase(it);
    EXPECT_EQ(ms.size(), 3);
    EXPECT_EQ(ms.count(2), 1);
}

TEST_F(SparseMultisetTest, EraseByKey) {
    sparse_multiset<int> ms = {1, 2, 2, 3, 3, 3};
    size_t count = ms.erase(2);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(ms.size(), 4);
    EXPECT_EQ(ms.count(2), 0);
}

TEST_F(SparseMultisetTest, EraseByNonExistentKey) {
    sparse_multiset<int> ms = {1, 2};
    size_t count = ms.erase(99);
    EXPECT_EQ(count, 0);
}

TEST_F(SparseMultisetTest, EraseRange) {
    sparse_multiset<int> ms = {1, 2, 2, 2, 3, 3, 4};
    auto range = ms.equal_range(2);
    ms.erase(range.first, range.second);
    EXPECT_EQ(ms.size(), 4);
    EXPECT_EQ(ms.count(2), 0);
}

TEST_F(SparseMultisetTest, Clear) {
    sparse_multiset<int> ms = {1, 2, 2, 3};
    ms.clear();
    EXPECT_TRUE(ms.empty());
    EXPECT_EQ(ms.size(), 0);
}

TEST_F(SparseMultisetTest, Find) {
    sparse_multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.find(2);
    EXPECT_NE(it, ms.end());
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseMultisetTest, FindNonExistent) {
    sparse_multiset<int> ms = {1, 2};
    auto it = ms.find(99);
    EXPECT_EQ(it, ms.end());
}

TEST_F(SparseMultisetTest, ConstFind) {
    const sparse_multiset<int> ms = {1, 2, 2};
    auto it = ms.find(2);
    EXPECT_NE(it, ms.end());
}

TEST_F(SparseMultisetTest, Count) {
    sparse_multiset<int> ms = {1, 2, 2, 3, 3, 3};
    EXPECT_EQ(ms.count(1), 1);
    EXPECT_EQ(ms.count(2), 2);
    EXPECT_EQ(ms.count(3), 3);
    EXPECT_EQ(ms.count(99), 0);
}

TEST_F(SparseMultisetTest, LowerBound) {
    sparse_multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.lower_bound(2);
    EXPECT_EQ(*it, 2);
    it = ms.lower_bound(0);
    EXPECT_EQ(*it, 1);
    it = ms.lower_bound(4);
    EXPECT_EQ(it, ms.end());
}

TEST_F(SparseMultisetTest, ConstLowerBound) {
    const sparse_multiset<int> ms = {1, 2, 3};
    auto it = ms.lower_bound(2);
    EXPECT_EQ(*it, 2);
}

TEST_F(SparseMultisetTest, UpperBound) {
    sparse_multiset<int> ms = {1, 2, 2, 3};
    auto it = ms.upper_bound(2);
    EXPECT_EQ(*it, 3);
    it = ms.upper_bound(3);
    EXPECT_EQ(it, ms.end());
}

TEST_F(SparseMultisetTest, ConstUpperBound) {
    const sparse_multiset<int> ms = {1, 2, 3};
    auto it = ms.upper_bound(2);
    EXPECT_EQ(*it, 3);
}

TEST_F(SparseMultisetTest, EqualRange) {
    sparse_multiset<int> ms = {1, 2, 2, 2, 3};
    auto range = ms.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(*it, 2);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(SparseMultisetTest, ConstEqualRange) {
    const sparse_multiset<int> ms = {1, 2, 2, 3};
    auto range = ms.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(SparseMultisetTest, EqualRangeNonExistent) {
    sparse_multiset<int> ms = {1, 3, 5};
    auto range = ms.equal_range(2);
    EXPECT_EQ(range.first, range.second);
}

TEST_F(SparseMultisetTest, Swap) {
    sparse_multiset<int> ms1 = {1, 2, 2};
    sparse_multiset<int> ms2 = {3, 4, 5, 6};
    ms1.swap(ms2);
    EXPECT_EQ(ms1.size(), 4);
    EXPECT_EQ(ms2.size(), 3);
}

TEST_F(SparseMultisetTest, EqualTo) {
    sparse_multiset<int> ms1 = {1, 2, 2, 3};
    sparse_multiset<int> ms2 = {1, 2, 2, 3};
    sparse_multiset<int> ms3 = {1, 2, 3};
    EXPECT_TRUE(ms1.equal_to(ms2));
    EXPECT_FALSE(ms1.equal_to(ms3));
}

TEST_F(SparseMultisetTest, LessThan) {
    sparse_multiset<int> ms1 = {1, 2, 3};
    sparse_multiset<int> ms2 = {1, 2, 4};
    EXPECT_TRUE(ms1.less_than(ms2));
}

TEST_F(SparseMultisetTest, EqualityOperator) {
    sparse_multiset<int> ms1 = {1, 2, 2};
    sparse_multiset<int> ms2 = {1, 2, 2};
    sparse_multiset<int> ms3 = {1, 2};
    EXPECT_TRUE(ms1 == ms2);
    EXPECT_FALSE(ms1 == ms3);
}

TEST_F(SparseMultisetTest, InequalityOperator) {
    sparse_multiset<int> ms1 = {1, 2};
    sparse_multiset<int> ms2 = {3, 4};
    EXPECT_TRUE(ms1 != ms2);
}

TEST_F(SparseMultisetTest, LessThanOperator) {
    sparse_multiset<int> ms1 = {1, 2};
    sparse_multiset<int> ms2 = {1, 3};
    EXPECT_TRUE(ms1 < ms2);
}

TEST_F(SparseMultisetTest, GreaterThanOperator) {
    sparse_multiset<int> ms1 = {1, 3};
    sparse_multiset<int> ms2 = {1, 2};
    EXPECT_TRUE(ms1 > ms2);
}

TEST_F(SparseMultisetTest, StringKey) {
    sparse_multiset<string> ms;
    ms.insert("banana");
    ms.insert("apple");
    ms.insert("banana");
    ms.insert("cherry");
    EXPECT_EQ(ms.size(), 4);
    EXPECT_EQ(ms.count("banana"), 2);
    auto it = ms.begin();
    EXPECT_EQ(*it, "apple");
}

TEST_F(SparseMultisetTest, LargeInsertWithDuplicates) {
    sparse_multiset<int> ms;
    const int count = 500;
    for (int i = 0; i < count; ++i) {
        ms.insert(i % 10);
    }
    EXPECT_EQ(ms.size(), count);
    EXPECT_EQ(ms.count(0), 50);
    EXPECT_EQ(ms.count(5), 50);
}

TEST_F(SparseMultisetTest, DuplicateOrderPreservation) {
    sparse_multiset<int> ms;
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

TEST_F(SparseMultisetTest, RangeBasedForLoop) {
    sparse_multiset<int> ms = {1, 2, 2, 3, 3, 3};
    int sum = 0;
    for (auto val: ms) {
        sum += val;
    }
    EXPECT_EQ(sum, 14);
}

TEST_F(SparseMultisetTest, Reserve) {
    sparse_multiset<int> ms;
    ms.reserve(100);
    EXPECT_GE(ms.capacity(), 100);
}

class FlatUnorderedMapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FlatUnorderedMapTest, DefaultConstructor) {
    flat_unordered_map<int, string> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
    EXPECT_EQ(m.capacity(), 0);
}

TEST_F(FlatUnorderedMapTest, ConstructorWithCapacity) {
    flat_unordered_map<int, string> m(50);
    EXPECT_GE(m.capacity(), 16);
}

TEST_F(FlatUnorderedMapTest, ConstructorWithCapacityAndHash) {
    hash<int> hf;
    flat_unordered_map<int, string> m(30, hf);
    EXPECT_GE(m.capacity(), 16);
}

TEST_F(FlatUnorderedMapTest, ConstructorWithCapacityHashAndEqual) {
    hash<int> hf;
    equal_to<int> eql;
    flat_unordered_map<int, string> m(20, hf, eql);
    EXPECT_GE(m.capacity(), 16);
}

TEST_F(FlatUnorderedMapTest, InitializerListConstructor) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], "one");
    EXPECT_EQ(m[2], "two");
    EXPECT_EQ(m[3], "three");
}

TEST_F(FlatUnorderedMapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {3, "three"}};
    flat_unordered_map<int, string> m(vec.begin(), vec.end());
    EXPECT_EQ(m.size(), 3);
}

TEST_F(FlatUnorderedMapTest, RangeConstructorWithCapacity) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}};
    flat_unordered_map<int, string> m(vec.begin(), vec.end(), 50);
    EXPECT_EQ(m.size(), 2);
}

TEST_F(FlatUnorderedMapTest, RangeConstructorWithCapacityAndHash) {
    vector<pair<int, string>> vec = {{1, "one"}};
    hash<int> hf;
    flat_unordered_map<int, string> m(vec.begin(), vec.end(), 30, hf);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(FlatUnorderedMapTest, CopyConstructor) {
    flat_unordered_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    flat_unordered_map<int, string> m2(m1);
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(FlatUnorderedMapTest, CopyAssignment) {
    flat_unordered_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    flat_unordered_map<int, string> m2;
    m2 = m1;
    EXPECT_EQ(m2.size(), 2);
}

TEST_F(FlatUnorderedMapTest, MoveConstructor) {
    flat_unordered_map<int, string> m1 = {{1, "one"}, {2, "two"}};
    flat_unordered_map<int, string> m2(move(m1));
    EXPECT_EQ(m2.size(), 2);
}

TEST_F(FlatUnorderedMapTest, MoveAssignment) {
    flat_unordered_map<int, string> m1 = {{1, "one"}};
    flat_unordered_map<int, string> m2;
    m2 = move(m1);
    EXPECT_EQ(m2.size(), 1);
}

TEST_F(FlatUnorderedMapTest, BeginEnd) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    int count = 0;
    for (auto it = m.begin(); it != m.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(FlatUnorderedMapTest, ConstBeginEnd) {
    const flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    int count = 0;
    for (auto it = m.begin(); it != m.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(FlatUnorderedMapTest, CbeginCend) {
    flat_unordered_map<int, string> m = {{1, "one"}};
    int count = 0;
    for (auto it = m.cbegin(); it != m.cend(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 1);
}

TEST_F(FlatUnorderedMapTest, Size) {
    flat_unordered_map<int, string> m;
    EXPECT_EQ(m.size(), 0);
    m.insert({1, "one"});
    EXPECT_EQ(m.size(), 1);
}

TEST_F(FlatUnorderedMapTest, Empty) {
    flat_unordered_map<int, string> m;
    EXPECT_TRUE(m.empty());
    m.insert({1, "one"});
    EXPECT_FALSE(m.empty());
}

TEST_F(FlatUnorderedMapTest, InsertCopy) {
    flat_unordered_map<int, string> m;
    pair<int, string> val = {1, "one"};
    auto result = m.insert(val);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
}

TEST_F(FlatUnorderedMapTest, InsertMove) {
    flat_unordered_map<int, string> m;
    auto result = m.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(FlatUnorderedMapTest, InsertDuplicateKey) {
    flat_unordered_map<int, string> m = {{1, "one"}};
    auto result = m.insert({1, "again"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m[1], "one");
}

TEST_F(FlatUnorderedMapTest, Emplace) {
    flat_unordered_map<int, string> m;
    auto result = m.emplace(1, "one");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
}

TEST_F(FlatUnorderedMapTest, EmplaceDuplicateKey) {
    flat_unordered_map<int, string> m = {{1, "one"}};
    auto result = m.emplace(1, "again");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(FlatUnorderedMapTest, EraseByKey) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto erased = m.erase(1);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m.find(1), m.end());
}

TEST_F(FlatUnorderedMapTest, EraseByIterator) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    auto next = m.erase(it);
    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m.find(1), m.end());
}

TEST_F(FlatUnorderedMapTest, EraseRange) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    auto it = m.erase(m.begin(), m.end());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(FlatUnorderedMapTest, Clear) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    m.clear();
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(FlatUnorderedMapTest, Find) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, "one");
    auto it2 = m.find(99);
    EXPECT_EQ(it2, m.end());
}

TEST_F(FlatUnorderedMapTest, ConstFind) {
    const flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    EXPECT_NE(it, m.end());
}

TEST_F(FlatUnorderedMapTest, Count) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    EXPECT_EQ(m.count(1), 1);
    EXPECT_EQ(m.count(99), 0);
}

TEST_F(FlatUnorderedMapTest, Contains) {
    flat_unordered_map<int, string> m = {{1, "one"}};
    EXPECT_TRUE(m.contains(1));
    EXPECT_FALSE(m.contains(99));
}

TEST_F(FlatUnorderedMapTest, EqualRange) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    auto range = m.equal_range(1);
    EXPECT_NE(range.first, m.end());
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        ++count;
    }
    EXPECT_EQ(count, 1);
}

TEST_F(FlatUnorderedMapTest, SubscriptOperator) {
    flat_unordered_map<int, string> m;
    m[1] = "one";
    EXPECT_EQ(m[1], "one");
    m[1] = "updated";
    EXPECT_EQ(m[1], "updated");
}

TEST_F(FlatUnorderedMapTest, SubscriptOperatorDefaultInsert) {
    flat_unordered_map<int, string> m;
    auto& val = m[42];
    EXPECT_EQ(val, "");
    EXPECT_EQ(m.size(), 1);
}

TEST_F(FlatUnorderedMapTest, At) {
    flat_unordered_map<int, string> m = {{1, "one"}};
    EXPECT_EQ(m.at(1), "one");
    m.at(1) = "updated";
    EXPECT_EQ(m.at(1), "updated");
}

TEST_F(FlatUnorderedMapTest, AtConst) {
    const flat_unordered_map<int, string> m = {{1, "one"}};
    EXPECT_EQ(m.at(1), "one");
}

TEST_F(FlatUnorderedMapTest, AtThrowsOnMissingKey) {
    flat_unordered_map<int, string> m;
    EXPECT_THROW(ignore = m.at(99), iterator_exception);
}

TEST_F(FlatUnorderedMapTest, AtConstThrowsOnMissingKey) {
    const flat_unordered_map<int, string> m;
    EXPECT_THROW(ignore = m.at(99), iterator_exception);
}

TEST_F(FlatUnorderedMapTest, Swap) {
    flat_unordered_map<int, string> m1 = {{1, "one"}};
    flat_unordered_map<int, string> m2 = {{2, "two"}, {3, "three"}};
    m1.swap(m2);
    EXPECT_EQ(m1.size(), 2);
    EXPECT_EQ(m2.size(), 1);
    EXPECT_EQ(m2[1], "one");
}

TEST_F(FlatUnorderedMapTest, LoadFactor) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}};
    EXPECT_GE(m.max_load_factor(), 0.0F);
    EXPECT_GE(m.load_factor(), 0.0F);
}

TEST_F(FlatUnorderedMapTest, SetMaxLoadFactor) {
    flat_unordered_map<int, string> m;
    m.max_load_factor(0.5F);
    EXPECT_FLOAT_EQ(m.max_load_factor(), 0.5F);
}

TEST_F(FlatUnorderedMapTest, Reserve) {
    flat_unordered_map<int, string> m;
    m.reserve(100);
    EXPECT_GE(m.capacity(), 100);
}

TEST_F(FlatUnorderedMapTest, Rehash) {
    flat_unordered_map<int, string> m = {{1, "one"}};
    size_t old_cap = m.capacity();
    m.rehash(500);
    EXPECT_GE(m.capacity(), old_cap);
}

TEST_F(FlatUnorderedMapTest, LargeInsert) {
    flat_unordered_map<int, int> m;
    for (int i = 0; i < 1000; ++i) {
        m.insert({i, i * 10});
    }
    EXPECT_EQ(m.size(), 1000);
    EXPECT_EQ(m[500], 5000);
}

TEST_F(FlatUnorderedMapTest, InsertAfterErase) {
    flat_unordered_map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};
    m.erase(1);
    m.erase(2);
    auto result = m.insert({4, "four"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(m.size(), 2);
}

TEST_F(FlatUnorderedMapTest, RangeBasedForLoop) {
    flat_unordered_map<int, int> m = {{1, 10}, {2, 20}, {3, 30}};
    int sum = 0;
    for (const auto& kv: m) {
        sum += kv.second;
    }
    EXPECT_EQ(sum, 60);
}

TEST_F(FlatUnorderedMapTest, StringKey) {
    flat_unordered_map<string, int> m;
    m.insert({"hello", 1});
    m.insert({"world", 2});
    EXPECT_EQ(m["hello"], 1);
    EXPECT_EQ(m["world"], 2);
}

class FlatUnorderedSetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FlatUnorderedSetTest, DefaultConstructor) {
    flat_unordered_set<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(FlatUnorderedSetTest, ConstructorWithCapacity) {
    flat_unordered_set<int> s(50);
    EXPECT_GE(s.capacity(), 16);
}

TEST_F(FlatUnorderedSetTest, ConstructorWithCapacityAndHash) {
    hash<int> hf;
    flat_unordered_set<int> s(30, hf);
    EXPECT_GE(s.capacity(), 16);
}

TEST_F(FlatUnorderedSetTest, ConstructorWithCapacityHashAndEqual) {
    hash<int> hf;
    equal_to<int> eql;
    flat_unordered_set<int> s(20, hf, eql);
    EXPECT_GE(s.capacity(), 16);
}

TEST_F(FlatUnorderedSetTest, InitializerListConstructor) {
    flat_unordered_set<int> s = {1, 2, 3};
    EXPECT_EQ(s.size(), 3);
    EXPECT_NE(s.find(1), s.end());
    EXPECT_NE(s.find(2), s.end());
    EXPECT_NE(s.find(3), s.end());
}

TEST_F(FlatUnorderedSetTest, RangeConstructor) {
    vector<int> vec = {1, 2, 3, 4, 5};
    flat_unordered_set<int> s(vec.begin(), vec.end());
    EXPECT_EQ(s.size(), 5);
}

TEST_F(FlatUnorderedSetTest, CopyConstructor) {
    flat_unordered_set<int> s1 = {1, 2, 3};
    flat_unordered_set<int> s2(s1);
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(FlatUnorderedSetTest, CopyAssignment) {
    flat_unordered_set<int> s1 = {1, 2};
    flat_unordered_set<int> s2;
    s2 = s1;
    EXPECT_EQ(s2.size(), 2);
}

TEST_F(FlatUnorderedSetTest, MoveConstructor) {
    flat_unordered_set<int> s1 = {1, 2, 3};
    flat_unordered_set<int> s2(move(s1));
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(FlatUnorderedSetTest, MoveAssignment) {
    flat_unordered_set<int> s1 = {1, 2};
    flat_unordered_set<int> s2;
    s2 = move(s1);
    EXPECT_EQ(s2.size(), 2);
}

TEST_F(FlatUnorderedSetTest, BeginEnd) {
    flat_unordered_set<int> s = {1, 2, 3};
    int count = 0;
    for (auto it = s.begin(); it != s.end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(FlatUnorderedSetTest, Empty) {
    flat_unordered_set<int> s;
    EXPECT_TRUE(s.empty());
    s.insert(1);
    EXPECT_FALSE(s.empty());
}

TEST_F(FlatUnorderedSetTest, InsertCopy) {
    flat_unordered_set<int> s;
    const int val = 42;
    auto result = s.insert(val);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(s.size(), 1);
}

TEST_F(FlatUnorderedSetTest, InsertMove) {
    flat_unordered_set<int> s;
    auto result = s.insert(42);
    EXPECT_TRUE(result.second);
}

TEST_F(FlatUnorderedSetTest, InsertDuplicate) {
    flat_unordered_set<int> s = {1, 2, 3};
    auto result = s.insert(1);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(s.size(), 3);
}

TEST_F(FlatUnorderedSetTest, Emplace) {
    flat_unordered_set<int> s;
    auto result = s.emplace(42);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 42);
}

TEST_F(FlatUnorderedSetTest, EraseByKey) {
    flat_unordered_set<int> s = {1, 2, 3};
    auto erased = s.erase(2);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(s.find(2), s.end());
}

TEST_F(FlatUnorderedSetTest, EraseByIterator) {
    flat_unordered_set<int> s = {1, 2, 3};
    auto it = s.find(2);
    s.erase(it);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(FlatUnorderedSetTest, Clear) {
    flat_unordered_set<int> s = {1, 2, 3};
    s.clear();
    EXPECT_TRUE(s.empty());
}

TEST_F(FlatUnorderedSetTest, Find) {
    flat_unordered_set<int> s = {10, 20, 30};
    EXPECT_NE(s.find(20), s.end());
    EXPECT_EQ(s.find(99), s.end());
}

TEST_F(FlatUnorderedSetTest, Count) {
    flat_unordered_set<int> s = {1, 2, 3};
    EXPECT_EQ(s.count(2), 1);
    EXPECT_EQ(s.count(99), 0);
}

TEST_F(FlatUnorderedSetTest, Contains) {
    flat_unordered_set<int> s = {1, 2, 3};
    EXPECT_TRUE(s.contains(2));
    EXPECT_FALSE(s.contains(99));
}

TEST_F(FlatUnorderedSetTest, EqualRange) {
    flat_unordered_set<int> s = {1, 2, 3};
    auto range = s.equal_range(2);
    EXPECT_NE(range.first, s.end());
}

TEST_F(FlatUnorderedSetTest, Swap) {
    flat_unordered_set<int> s1 = {1, 2};
    flat_unordered_set<int> s2 = {3, 4, 5};
    s1.swap(s2);
    EXPECT_EQ(s1.size(), 3);
    EXPECT_EQ(s2.size(), 2);
}

TEST_F(FlatUnorderedSetTest, LargeInsert) {
    flat_unordered_set<int> s;
    for (int i = 0; i < 1000; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(s.size(), 1000);
    EXPECT_NE(s.find(500), s.end());
}

TEST_F(FlatUnorderedSetTest, InsertAfterErase) {
    flat_unordered_set<int> s = {1, 2, 3, 4, 5};
    s.erase(1);
    s.erase(2);
    s.erase(3);
    auto result = s.insert(10);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(s.size(), 3);
}

TEST_F(FlatUnorderedSetTest, RangeBasedForLoop) {
    flat_unordered_set<int> s = {10, 20, 30};
    int sum = 0;
    for (int val: s) {
        sum += val;
    }
    EXPECT_EQ(sum, 60);
}

TEST_F(FlatUnorderedSetTest, StringKey) {
    flat_unordered_set<string> s;
    s.insert("hello");
    s.insert("world");
    EXPECT_EQ(s.size(), 2);
    EXPECT_NE(s.find("hello"), s.end());
}

class FlatUnorderedMultimapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FlatUnorderedMultimapTest, DefaultConstructor) {
    flat_unordered_multimap<int, string> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST_F(FlatUnorderedMultimapTest, InitializerListConstructor) {
    flat_unordered_multimap<int, string> m = {{1, "one"}, {1, "uno"}, {2, "two"}};
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m.count(1), 2);
}

TEST_F(FlatUnorderedMultimapTest, RangeConstructor) {
    vector<pair<int, string>> vec = {{1, "one"}, {2, "two"}, {2, "deux"}};
    flat_unordered_multimap<int, string> m(vec.begin(), vec.end());
    EXPECT_EQ(m.size(), 3);
}

TEST_F(FlatUnorderedMultimapTest, CopyConstructor) {
    flat_unordered_multimap<int, string> m1 = {{1, "a"}, {1, "b"}};
    flat_unordered_multimap<int, string> m2(m1);
    EXPECT_EQ(m2.size(), 2);
    EXPECT_EQ(m2.count(1), 2);
}

TEST_F(FlatUnorderedMultimapTest, MoveConstructor) {
    flat_unordered_multimap<int, string> m1 = {{1, "a"}, {2, "b"}};
    flat_unordered_multimap<int, string> m2(move(m1));
    EXPECT_EQ(m2.size(), 2);
}

TEST_F(FlatUnorderedMultimapTest, MoveAssignment) {
    flat_unordered_multimap<int, string> m1 = {{1, "a"}};
    flat_unordered_multimap<int, string> m2;
    m2 = move(m1);
    EXPECT_EQ(m2.size(), 1);
}

TEST_F(FlatUnorderedMultimapTest, InsertDuplicateKeys) {
    flat_unordered_multimap<int, string> m;
    auto it1 = m.insert({1, "one"});
    auto it2 = m.insert({1, "uno"});
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m.count(1), 2);
}

TEST_F(FlatUnorderedMultimapTest, Emplace) {
    flat_unordered_multimap<int, string> m;
    auto it = m.emplace(1, "one");
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
}

TEST_F(FlatUnorderedMultimapTest, EraseByKey) {
    flat_unordered_multimap<int, string> m = {{1, "a"}, {1, "b"}, {2, "c"}};
    auto erased = m.erase(1);
    EXPECT_EQ(erased, 2);
    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m.count(1), 0);
}

TEST_F(FlatUnorderedMultimapTest, EraseByIterator) {
    flat_unordered_multimap<int, string> m = {{1, "a"}, {1, "b"}};
    auto it = m.find(1);
    m.erase(it);
    EXPECT_EQ(m.size(), 1);
}

TEST_F(FlatUnorderedMultimapTest, Clear) {
    flat_unordered_multimap<int, string> m = {{1, "a"}, {1, "b"}, {2, "c"}};
    m.clear();
    EXPECT_TRUE(m.empty());
}

TEST_F(FlatUnorderedMultimapTest, Find) {
    flat_unordered_multimap<int, string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    EXPECT_NE(it, m.end());
    EXPECT_EQ(it->second, "one");
}

TEST_F(FlatUnorderedMultimapTest, Count) {
    flat_unordered_multimap<int, string> m = {{1, "a"}, {1, "b"}, {1, "c"}, {2, "d"}};
    EXPECT_EQ(m.count(1), 3);
    EXPECT_EQ(m.count(2), 1);
    EXPECT_EQ(m.count(99), 0);
}

TEST_F(FlatUnorderedMultimapTest, Contains) {
    flat_unordered_multimap<int, string> m = {{1, "a"}, {2, "b"}};
    EXPECT_TRUE(m.contains(1));
    EXPECT_FALSE(m.contains(99));
}

TEST_F(FlatUnorderedMultimapTest, EqualRange) {
    flat_unordered_multimap<int, string> m = {{1, "a"}, {1, "b"}, {2, "c"}};
    auto range = m.equal_range(1);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(it->first, 1);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(FlatUnorderedMultimapTest, Swap) {
    flat_unordered_multimap<int, string> m1 = {{1, "a"}};
    flat_unordered_multimap<int, string> m2 = {{2, "b"}, {3, "c"}};
    m1.swap(m2);
    EXPECT_EQ(m1.size(), 2);
    EXPECT_EQ(m2.size(), 1);
}

TEST_F(FlatUnorderedMultimapTest, LargeInsert) {
    flat_unordered_multimap<int, int> m;
    for (int i = 0; i < 500; ++i) {
        m.insert({i, i * 10});
        m.insert({i, i * 10 + 1});
    }
    EXPECT_EQ(m.size(), 1000);
    EXPECT_EQ(m.count(100), 2);
}

TEST_F(FlatUnorderedMultimapTest, RangeBasedForLoop) {
    flat_unordered_multimap<int, int> m = {{1, 10}, {1, 100}, {2, 20}};
    int sum = 0;
    for (const auto& kv: m) {
        sum += kv.second;
    }
    EXPECT_EQ(sum, 130);
}

class FlatUnorderedMultisetTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FlatUnorderedMultisetTest, DefaultConstructor) {
    flat_unordered_multiset<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST_F(FlatUnorderedMultisetTest, InitializerListConstructor) {
    flat_unordered_multiset<int> s = {1, 1, 2, 2, 3};
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s.count(1), 2);
    EXPECT_EQ(s.count(2), 2);
    EXPECT_EQ(s.count(3), 1);
}

TEST_F(FlatUnorderedMultisetTest, RangeConstructor) {
    vector<int> vec = {1, 1, 2, 3, 3, 3};
    flat_unordered_multiset<int> s(vec.begin(), vec.end());
    EXPECT_EQ(s.size(), 6);
}

TEST_F(FlatUnorderedMultisetTest, CopyConstructor) {
    flat_unordered_multiset<int> s1 = {1, 1, 2};
    flat_unordered_multiset<int> s2(s1);
    EXPECT_EQ(s2.size(), 3);
    EXPECT_EQ(s2.count(1), 2);
}

TEST_F(FlatUnorderedMultisetTest, MoveConstructor) {
    flat_unordered_multiset<int> s1 = {1, 2, 2};
    flat_unordered_multiset<int> s2(move(s1));
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(FlatUnorderedMultisetTest, MoveAssignment) {
    flat_unordered_multiset<int> s1 = {1, 1, 2};
    flat_unordered_multiset<int> s2;
    s2 = move(s1);
    EXPECT_EQ(s2.size(), 3);
}

TEST_F(FlatUnorderedMultisetTest, InsertDuplicate) {
    flat_unordered_multiset<int> s;
    s.insert(1);
    s.insert(1);
    s.insert(1);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.count(1), 3);
}

TEST_F(FlatUnorderedMultisetTest, Emplace) {
    flat_unordered_multiset<int> s;
    auto it = s.emplace(42);
    EXPECT_EQ(*it, 42);
    EXPECT_EQ(s.size(), 1);
}

TEST_F(FlatUnorderedMultisetTest, EraseByKey) {
    flat_unordered_multiset<int> s = {1, 1, 2, 2, 2};
    auto erased = s.erase(2);
    EXPECT_EQ(erased, 3);
    EXPECT_EQ(s.count(2), 0);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(FlatUnorderedMultisetTest, EraseByIterator) {
    flat_unordered_multiset<int> s = {1, 1, 2};
    auto it = s.find(1);
    s.erase(it);
    EXPECT_EQ(s.size(), 2);
}

TEST_F(FlatUnorderedMultisetTest, Clear) {
    flat_unordered_multiset<int> s = {1, 1, 2, 3};
    s.clear();
    EXPECT_TRUE(s.empty());
}

TEST_F(FlatUnorderedMultisetTest, Find) {
    flat_unordered_multiset<int> s = {10, 10, 20};
    auto it = s.find(10);
    EXPECT_NE(it, s.end());
    EXPECT_EQ(*it, 10);
}

TEST_F(FlatUnorderedMultisetTest, Count) {
    flat_unordered_multiset<int> s = {1, 1, 1, 2, 2, 3};
    EXPECT_EQ(s.count(1), 3);
    EXPECT_EQ(s.count(2), 2);
    EXPECT_EQ(s.count(3), 1);
    EXPECT_EQ(s.count(99), 0);
}

TEST_F(FlatUnorderedMultisetTest, Contains) {
    flat_unordered_multiset<int> s = {1, 1, 2};
    EXPECT_TRUE(s.contains(1));
    EXPECT_FALSE(s.contains(99));
}

TEST_F(FlatUnorderedMultisetTest, EqualRange) {
    flat_unordered_multiset<int> s = {1, 1, 2, 2, 3};
    auto range = s.equal_range(2);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) {
        EXPECT_EQ(*it, 2);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(FlatUnorderedMultisetTest, Swap) {
    flat_unordered_multiset<int> s1 = {1, 1};
    flat_unordered_multiset<int> s2 = {2, 3, 4};
    s1.swap(s2);
    EXPECT_EQ(s1.size(), 3);
    EXPECT_EQ(s2.size(), 2);
}

TEST_F(FlatUnorderedMultisetTest, LargeInsert) {
    flat_unordered_multiset<int> s;
    for (int i = 0; i < 500; ++i) {
        s.insert(i);
        s.insert(i);
    }
    EXPECT_EQ(s.size(), 1000);
    EXPECT_EQ(s.count(100), 2);
}

TEST_F(FlatUnorderedMultisetTest, RangeBasedForLoop) {
    flat_unordered_multiset<int> s = {1, 1, 2, 2, 2};
    int sum = 0;
    for (int val: s) {
        sum += val;
    }
    EXPECT_EQ(sum, 8);
}
