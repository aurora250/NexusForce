#include <NeForce/core/algorithm/type_erase.hpp>
#include <NeForce/core/container/list.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/memory/allocated_ptr.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/memory/memory.hpp>
#include <NeForce/core/memory/memory_view.hpp>
#include <NeForce/core/memory/weak_ptr.hpp>
#include <NeForce/core/string/string.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    int delete_counter = 0;
    struct delete_counter_id {
        int id;
        delete_counter_id(int i = 0) :
        id(i) {}
    };
    void delete_delete_counter(delete_counter_id* p) {
        delete_counter++;
        delete p;
    }

    struct complex_type {
        int x;
        double y;
        complex_type(int a, double b) :
        x(a),
        y(b) {}
    };

    struct enable_test : enable_shared_from_this<enable_test> {
        int value;
        enable_test(int v = 0) :
        value(v) {}
    };

    struct weak_test_base {
        int value;
        weak_test_base(int v = 0) :
        value(v) {}
        virtual ~weak_test_base() = default;
    };

    struct weak_test_derived : weak_test_base {
        int extra;
        weak_test_derived(int v = 0, int e = 0) :
        weak_test_base(v),
        extra(e) {}
    };

    struct owner_less_test {
        shared_ptr<int> sp1 = make_shared<int>(1);
        shared_ptr<int> sp2 = make_shared<int>(2);
        shared_ptr<int> sp3 = sp1;
        weak_ptr<int> wp1{sp1};
        weak_ptr<int> wp2{sp2};
        weak_ptr<int> wp3{sp3};
    };

    struct construct_counter {
        static size_t constructions;
        static size_t destructions;
        int value;

        construct_counter(int v = 0) :
        value(v) {
            ++constructions;
        }
        construct_counter(const construct_counter& other) :
        value(other.value) {
            ++constructions;
        }
        construct_counter(construct_counter&& other) noexcept :
        value(other.value) {
            other.value = -1;
            ++constructions;
        }
        ~construct_counter() { ++destructions; }

        static void reset() { constructions = destructions = 0; }
    };
    size_t construct_counter::constructions = 0;
    size_t construct_counter::destructions = 0;

    struct throw_on_copy {
        static size_t copy_count;
        static size_t throw_at;
        static size_t destructions;
        int value;

        throw_on_copy(int v = 0) :
        value(v) {}
        throw_on_copy(const throw_on_copy& other) :
        value(other.value) {
            if (++copy_count == throw_at) {
                throw exception("forced copy exception");
            }
        }
        throw_on_copy(throw_on_copy&& other) noexcept :
        value(other.value) {
            other.value = -1;
        }
        ~throw_on_copy() { ++destructions; }

        static void reset(size_t throw_at_val) {
            copy_count = 0;
            throw_at = throw_at_val;
            destructions = 0;
        }
    };
    size_t throw_on_copy::copy_count = 0;
    size_t throw_on_copy::throw_at = 0;
    size_t throw_on_copy::destructions = 0;

    class UninitializedMemoryTest : public ::testing::Test {
    protected:
        template <typename T>
        T* allocate(size_t n) {
            return static_cast<T*>(::malloc(n * sizeof(T)));
        }

        template <typename T>
        void deallocate(T* p) {
            ::free(p);
        }
    };

    template <typename T>
    void test_host_network_round_trip(T val) {
        T net = endian::host_to_network(val);
        T host = endian::network_to_host(net);
        EXPECT_EQ(host, val);
    }

    template <typename T>
    struct counting_deleter {
        T* count;
        explicit counting_deleter(T* c) :
        count(c) {}
        counting_deleter(const counting_deleter&) = default;
        counting_deleter(counting_deleter&&) noexcept = default;
        counting_deleter& operator=(const counting_deleter&) = default;
        counting_deleter& operator=(counting_deleter&&) noexcept = default;

        void operator()(T* p) const {
            ++(*count);
            delete p;
        }

        template <typename U>
        counting_deleter<U> rebind() && noexcept {
            return counting_deleter<U>(dynamic_cast<U*>(count));
        }
    };

    template <typename T>
    struct move_only_deleter {
        T* count;
        explicit move_only_deleter(T* c) :
        count(c) {}
        move_only_deleter(const move_only_deleter&) = delete;
        move_only_deleter(move_only_deleter&&) noexcept = default;
        move_only_deleter& operator=(const move_only_deleter&) = delete;
        move_only_deleter& operator=(move_only_deleter&&) noexcept = default;

        void operator()(T* p) const {
            ++(*count);
            delete p;
        }

        template <typename U>
        move_only_deleter<U> rebind() && noexcept {
            return move_only_deleter<U>(dynamic_cast<U*>(count));
        }
    };

    template <typename T>
    struct non_assignable_deleter {
        T* count;
        explicit non_assignable_deleter(T* c) :
        count(c) {}
        non_assignable_deleter(const non_assignable_deleter&) = default;
        non_assignable_deleter(non_assignable_deleter&&) noexcept = default;
        void operator=(const non_assignable_deleter&) = delete;
        void operator=(non_assignable_deleter&&) = delete;

        void operator()(T* p) const {
            ++(*count);
            delete p;
        }

        template <typename U>
        non_assignable_deleter<U> rebind() && noexcept {
            return non_assignable_deleter<U>(dynamic_cast<U*>(count));
        }
    };

    struct base {
        int value;
        base(int v) :
        value(v) {}
        virtual ~base() = default;

        base& operator++() {
            ++value;
            return *this;
        }
        base operator++(int) {
            auto tmp = *this;
            ++value;
            return tmp;
        }
    };
    struct derived : base {
        int derived_val;
        derived(int b = 0, int d = 0) :
        base(b),
        derived_val(d) {}
    };

    struct other {};

    template <typename T>
    struct tracking_allocator {
        using value_type = T;
        T* last_allocated = nullptr;
        T* last_deallocated = nullptr;
        size_t allocate_count = 0;
        size_t deallocate_count = 0;

        tracking_allocator() = default;

        T* allocate(size_t n) {
            ++allocate_count;
            last_allocated = static_cast<T*>(::operator new(n * sizeof(T)));
            return last_allocated;
        }

        void deallocate(T* p, size_t n) {
            ++deallocate_count;
            last_deallocated = p;
            ::operator delete(p);
        }
    };
} // namespace

TEST(MemoryCopy, Basic) {
    char src[] = "hello";
    char dest[10] = {};
    EXPECT_EQ(_NEFORCE memory_copy(dest, src, 5), dest);
    EXPECT_EQ(memcmp(dest, src, 5), 0);
}

TEST(MemoryCopy, Nullptr) {
    char buf[10];
    EXPECT_EQ(_NEFORCE memory_copy(nullptr, buf, 10), nullptr);
    EXPECT_EQ(_NEFORCE memory_copy(buf, nullptr, 10), nullptr);
}

TEST(MemoryCopy, ZeroCount) {
    char dest = 'A', src = 'B';
    EXPECT_EQ(_NEFORCE memory_copy(&dest, &src, 0), &dest);
    EXPECT_EQ(dest, 'A');
}

TEST(MemoryCopy, Template) {
    int src = 42, dest = 0;
    EXPECT_EQ(_NEFORCE memory_copy(&dest, &src), &dest);
    EXPECT_EQ(dest, 42);
}

TEST(MemoryCopyOffset, Basic) {
    char src[] = "world", dest[10];
    void* end = _NEFORCE memory_copy_offset(dest, src, 5);
    EXPECT_EQ(end, dest + 5);
    EXPECT_EQ(memcmp(dest, src, 5), 0);
}

TEST(MemoryCopyOffset, Nullptr) {
    char buf[10];
    EXPECT_EQ(_NEFORCE memory_copy_offset(nullptr, buf, 5), nullptr);
    EXPECT_EQ(_NEFORCE memory_copy_offset(buf, nullptr, 5), nullptr);
}

TEST(MemoryCopyUntil, Found) {
    const char src[] = "abcXdef";
    char dest[10];
    void* pos = _NEFORCE memory_copy_until(dest, src, 'X', 7);
    EXPECT_EQ(pos, dest + 4);
    EXPECT_EQ(dest[3], 'X');
}

TEST(MemoryCopyUntil, NotFound) {
    const char src[] = "abcdef";
    char dest[10];
    void* pos = _NEFORCE memory_copy_until(dest, src, 'z', 6);
    EXPECT_EQ(pos, nullptr);
}

TEST(MemoryCopyUntil, Nullptr) {
    char buf[10];
    EXPECT_EQ(_NEFORCE memory_copy_until(nullptr, buf, 'a', 1), nullptr);
    EXPECT_EQ(_NEFORCE memory_copy_until(buf, nullptr, 'a', 1), nullptr);
}

TEST(MemoryCompare, Equal) {
    char a[] = "test", b[] = "test";
    EXPECT_EQ(_NEFORCE memory_compare(a, b, 4), 0);
}

TEST(MemoryCompare, Less) {
    char a = 0, b = 1;
    EXPECT_LT(_NEFORCE memory_compare(&a, &b, 1), 0);
}

TEST(MemoryCompare, Greater) {
    char a = 2, b = 1;
    EXPECT_GT(_NEFORCE memory_compare(&a, &b, 1), 0);
}

TEST(MemoryCompare, BothNull) { EXPECT_EQ(_NEFORCE memory_compare(nullptr, nullptr, 1), 0); }

TEST(MemoryCompare, LeftNull) {
    char x = 0;
    EXPECT_LT(_NEFORCE memory_compare(nullptr, &x, 1), 0);
}

TEST(MemoryCompare, RightNull) {
    char x = 0;
    EXPECT_GT(_NEFORCE memory_compare(&x, nullptr, 1), 0);
}

TEST(MemoryCompare, Template) {
    int a = 100, b = 200;
    EXPECT_LT(_NEFORCE memory_compare(a, b), 0);
}

TEST(MemoryMove, NoOverlap) {
    char src[] = "move", dest[10] = {};
    EXPECT_EQ(_NEFORCE memory_move(dest, src, 4), dest);
    EXPECT_STREQ(dest, "move");
}

TEST(MemoryMove, OverlapDestBeforeSrc) {
    char buf[] = "abcdef";
    _NEFORCE memory_move(buf, buf + 2, 4);
    EXPECT_EQ(memcmp(buf, "cdefef", 6), 0);
}

TEST(MemoryMove, OverlapDestAfterSrc) {
    char buf[] = "abcdef";
    _NEFORCE memory_move(buf + 2, buf, 4);
    EXPECT_EQ(memcmp(buf, "ababcd", 6), 0);
}

TEST(MemoryMove, Nullptr) {
    char buf[10];
    EXPECT_EQ(_NEFORCE memory_move(nullptr, buf, 4), nullptr);
    EXPECT_EQ(_NEFORCE memory_move(buf, nullptr, 4), nullptr);
}

TEST(MemorySet, Basic) {
    char buf[5];
    EXPECT_EQ(_NEFORCE memory_set(buf, 'X', 5), buf);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(buf[i], 'X');
    }
}

TEST(MemorySet, Nullptr) { EXPECT_EQ(_NEFORCE memory_set(nullptr, 'X', 5), nullptr); }

TEST(MemoryZero, VoidSize) {
    int x = 0x12345678;
    _NEFORCE memory_zero(&x, sizeof(x));
    EXPECT_EQ(x, 0);
}

TEST(MemoryZero, Nullptr) { _NEFORCE memory_zero(nullptr, 10); }

TEST(MemoryZero, Template) {
    double d = 3.14;
    _NEFORCE memory_zero(&d);
    EXPECT_EQ(d, 0.0);
}

TEST(MemoryFind, Found) {
    char buf[] = "find_me";
    const void* p = _NEFORCE memory_find(buf, '_', 7);
    EXPECT_EQ(static_cast<const char*>(p), buf + 4);
}

TEST(MemoryFind, NotFound) {
    char buf[] = "hello";
    EXPECT_EQ(_NEFORCE memory_find(buf, 'z', 5), nullptr);
}

TEST(MemoryFind, Nullptr) { EXPECT_EQ(_NEFORCE memory_find(nullptr, 'a', 1), nullptr); }

TEST(MemoryFindPattern, Found) {
    const char data[] = "abracadabra";
    const char pat[] = "cad";
    const void* r = _NEFORCE memory_find_pattern(data, 11, pat, 3);
    EXPECT_EQ(static_cast<const char*>(r), data + 4);
}

TEST(MemoryFindPattern, NotFound) {
    const char data[] = "hello";
    EXPECT_EQ(_NEFORCE memory_find_pattern(data, 5, "z", 1), nullptr);
}

TEST(MemoryFindPattern, PatternTooLong) {
    const char data[] = "abc";
    EXPECT_EQ(_NEFORCE memory_find_pattern(data, 3, "abcd", 4), nullptr);
}

TEST(MemoryFindPattern, Nullptr) {
    EXPECT_EQ(_NEFORCE memory_find_pattern(nullptr, 1, "a", 1), nullptr);
    EXPECT_EQ(_NEFORCE memory_find_pattern("a", 1, nullptr, 1), nullptr);
}

TEST(MemoryCast, IntToFloat) {
    int x = 0x3f800000;
    float f = _NEFORCE memory_cast<float>(x);
    EXPECT_FLOAT_EQ(f, 1.0f);
}

TEST(MemoryCast, FloatToInt) {
    float f = 1.0f;
    int x = _NEFORCE memory_cast<int>(f);
    EXPECT_EQ(x, 0x3f800000);
}

TEST(ToLowercase, Uppercase) {
    EXPECT_EQ(_NEFORCE to_lowercase('A'), 'a');
    EXPECT_EQ(_NEFORCE to_lowercase('Z'), 'z');
}

TEST(ToLowercase, AlreadyLower) {
    EXPECT_EQ(_NEFORCE to_lowercase('a'), 'a');
    EXPECT_EQ(_NEFORCE to_lowercase('z'), 'z');
}

TEST(ToLowercase, NonAlpha) {
    EXPECT_EQ(_NEFORCE to_lowercase('1'), '1');
    EXPECT_EQ(_NEFORCE to_lowercase('@'), '@');
}

TEST(ToUppercase, Lowercase) {
    EXPECT_EQ(_NEFORCE to_uppercase('a'), 'A');
    EXPECT_EQ(_NEFORCE to_uppercase('z'), 'Z');
}

TEST(ToUppercase, AlreadyUpper) {
    EXPECT_EQ(_NEFORCE to_uppercase('A'), 'A');
    EXPECT_EQ(_NEFORCE to_uppercase('Z'), 'Z');
}

TEST(ToUppercase, NonAlpha) {
    EXPECT_EQ(_NEFORCE to_uppercase('3'), '3');
    EXPECT_EQ(_NEFORCE to_uppercase('!'), '!');
}

TEST(StringCopy, Basic) {
    const char src[] = "hello";
    char dest[10] = {};
    EXPECT_EQ(_NEFORCE string_copy(dest, src), dest);
    EXPECT_STREQ(dest, src);
}

TEST(StringCopy, Nullptr) {
    char buf[10];
    EXPECT_EQ(_NEFORCE string_copy<char>(nullptr, buf), nullptr);
    EXPECT_EQ(_NEFORCE string_copy<char>(buf, nullptr), nullptr);
}

TEST(StringCopy, WithCount) {
    const char src[] = "hello";
    char dest[10] = {};
    _NEFORCE string_copy(dest, src, 3);
    EXPECT_EQ(memcmp(dest, "hel", 3), 0);
    for (int i = 3; i < 10; ++i) {
        EXPECT_EQ(dest[i], '\0');
    }
}

TEST(StringCopy, CountGreaterThanSrc) {
    const char src[] = "hi";
    char dest[5] = {};
    _NEFORCE string_copy(dest, src, 4);
    EXPECT_EQ(dest[0], 'h');
    EXPECT_EQ(dest[1], 'i');
    EXPECT_EQ(dest[2], '\0');
    EXPECT_EQ(dest[3], '\0');
}

TEST(StringCopyOffset, Basic) {
    const char src[] = "world";
    char dest[10];
    char* end = _NEFORCE string_copy_offset(dest, src);
    EXPECT_EQ(end, dest + 4);
    EXPECT_STREQ(dest, "world");
}

TEST(StringCopyOffset, WithCount) {
    const char src[] = "abcdef";
    char dest[10];
    char* end = _NEFORCE string_copy_offset(dest, src, 4);
    EXPECT_EQ(end, dest + 4);
}

TEST(StringCompare, Equal) { EXPECT_EQ(_NEFORCE string_compare("abc", "abc"), 0); }

TEST(StringCompare, Greater) { EXPECT_GT(_NEFORCE string_compare("b", "a"), 0); }

TEST(StringCompare, Less) { EXPECT_LT(_NEFORCE string_compare("a", "b"), 0); }

TEST(StringCompare, Nullptrs) {
    EXPECT_EQ(_NEFORCE string_compare<char>(nullptr, nullptr), 0);
    EXPECT_LT(_NEFORCE string_compare<char>(nullptr, "a"), 0);
    EXPECT_GT(_NEFORCE string_compare<char>("a", nullptr), 0);
}

TEST(StringCompare, WithCount) {
    EXPECT_EQ(_NEFORCE string_compare("abc", "abd", 2), 0);
    EXPECT_LT(_NEFORCE string_compare("abc", "abd", 3), 0);
}

TEST(StringCompareIgnoreCase, Equal) { EXPECT_EQ(_NEFORCE string_compare_ignore_case("AbC", "aBc"), 0); }

TEST(StringCompareIgnoreCase, Greater) { EXPECT_GT(_NEFORCE string_compare_ignore_case("b", "A"), 0); }

TEST(StringCompareIgnoreCase, WithCount) {
    EXPECT_EQ(_NEFORCE string_compare_ignore_case("AbCd", "abce", 3), 0);
    EXPECT_EQ(_NEFORCE string_compare_ignore_case("AbCd", "abce", 4), 0);
}

TEST(StringLength, Basic) { EXPECT_EQ(_NEFORCE string_length("hello"), 5u); }

TEST(StringLength, Empty) { EXPECT_EQ(_NEFORCE string_length(""), 0u); }

TEST(StringLength, Nullptr) { EXPECT_EQ(_NEFORCE string_length<char>(nullptr), 0u); }

TEST(StringLength, WithMaxLen) {
    EXPECT_EQ(_NEFORCE string_length("hello", 3), 3u);
    EXPECT_EQ(_NEFORCE string_length("hello", 10), 5u);
}

TEST(StringFind, CharFound) {
    const char* p = _NEFORCE string_find("hello", 'l');
    EXPECT_STREQ(p, "llo");
}

TEST(StringFind, CharNotFound) { EXPECT_EQ(_NEFORCE string_find("hello", 'z'), nullptr); }

TEST(StringFind, NullTerminator) {
    const char* p = _NEFORCE string_find("", 'a');
    EXPECT_EQ(p, nullptr);
}

TEST(StringFind, WithCount) {
    const char s[] = "hello";
    const char* p = _NEFORCE string_find(s, 'l', 3);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p, s + 2);
}

TEST(StringFindLast, Found) {
    char s[] = "abca";
    const char* p = _NEFORCE string_find_last(s, 'a');
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p, s + 3);
}

TEST(StringFindLast, NotFound) { EXPECT_EQ(_NEFORCE string_find_last("abc", 'z'), nullptr); }

TEST(StringFindAny, Found) {
    char str[] = "abcdef";
    const char* accept = "xd";
    EXPECT_EQ(_NEFORCE string_find_any(str, accept), str + 3);
}

TEST(StringFindAny, NotFound) {
    char str[] = "abc";
    EXPECT_EQ(_NEFORCE string_find_any(str, "xyz"), nullptr);
}

TEST(StringFindAny, EmptyAccept) {
    char str[] = "abc";
    EXPECT_EQ(_NEFORCE string_find_any(str, ""), nullptr);
}

TEST(StringFindPattern, Found) {
    const char* p = _NEFORCE string_find_pattern("hello world", "wor");
    EXPECT_STREQ(p, "world");
}

TEST(StringFindPattern, NotFound) { EXPECT_EQ(_NEFORCE string_find_pattern("hello", "z"), nullptr); }

TEST(StringFindPatternIgnoredCase, Found) {
    const char* p = _NEFORCE string_find_pattern_ignored_case("Hello WORLD", "wor");
    EXPECT_STREQ(p, "WORLD");
}

TEST(StringFindPatternIgnoredCase, NotFound) {
    EXPECT_EQ(_NEFORCE string_find_pattern_ignored_case("Hello", "xyz"), nullptr);
}

TEST(StringSpanIn, Basic) {
    const char* str = "abc123";
    EXPECT_EQ(_NEFORCE string_span_in(str, "cba"), 3u);
}

TEST(StringSpanIn, None) { EXPECT_EQ(_NEFORCE string_span_in("123", "abc"), 0u); }

TEST(StringSpanNotIn, Basic) {
    const char* str = "123abc";
    EXPECT_EQ(_NEFORCE string_span_not_in(str, "abc"), 3u);
}

TEST(StringSpanNotIn, All) { EXPECT_EQ(_NEFORCE string_span_not_in("abc", "123"), 3u); }

TEST(StringSpanNotIn, EmptyReject) { EXPECT_EQ(_NEFORCE string_span_not_in("abc", ""), 3u); }

TEST(StringSet, SetAll) {
    char str[] = "hello";
    _NEFORCE string_set(str, 'X');
    EXPECT_STREQ(str, "XXXXX");
}

TEST(StringSet, SetCount) {
    char str[] = "hello";
    _NEFORCE string_set(str, 'Y', 3);
    EXPECT_STREQ(str, "YYYlo");
}

TEST(StringSet, Nullptr) { EXPECT_EQ(_NEFORCE string_set<char>(nullptr, 'X'), nullptr); }

TEST(StringReverse, Basic) {
    char str[] = "abcde";
    _NEFORCE string_reverse(str);
    EXPECT_STREQ(str, "edcba");
}

TEST(StringReverse, Empty) {
    char str[] = "";
    _NEFORCE string_reverse(str);
    EXPECT_STREQ(str, "");
}

TEST(StringConcatenate, Basic) {
    char dest[20] = "Hello ";
    _NEFORCE string_concatenate(dest, "World");
    EXPECT_STREQ(dest, "Hello World");
}

TEST(StringConcatenate, WithCount) {
    char dest[20] = "AB";
    _NEFORCE string_concatenate(dest, "CDEF", 2);
    EXPECT_STREQ(dest, "ABCD");
}

TEST(StringConcatenate, Nullptrs) {
    EXPECT_EQ(_NEFORCE string_concatenate<char>(nullptr, "src"), nullptr);
    string dst = "dst";
    EXPECT_EQ(_NEFORCE string_concatenate<char>(dst.data(), nullptr), nullptr);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyTrivial) {
    int src[] = {1, 2, 3, 4, 5};
    int* dest = allocate<int>(5);
    auto result = uninitialized_copy(begin(src), end(src), dest);
    EXPECT_EQ(result, dest + 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(dest[i], src[i]);
    }
    for (int i = 0; i < 5; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyNonTrivial) {
    construct_counter::reset();
    construct_counter src[] = {10, 20, 30};
    construct_counter* dest = allocate<construct_counter>(3);
    auto result = uninitialized_copy(begin(src), end(src), dest);
    EXPECT_EQ(result, dest + 3);
    EXPECT_EQ(construct_counter::constructions, 6u);
    EXPECT_EQ(construct_counter::destructions, 0u);
    EXPECT_EQ(dest[0].value, 10);
    EXPECT_EQ(dest[1].value, 20);
    EXPECT_EQ(dest[2].value, 30);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
    EXPECT_EQ(construct_counter::destructions, 3u);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyForwardIterator) {
    list<int> src = {4, 5, 6};
    int* dest = allocate<int>(3);
    auto result = uninitialized_copy(src.begin(), src.end(), dest);
    EXPECT_EQ(result, dest + 3);
    EXPECT_EQ(dest[0], 4);
    EXPECT_EQ(dest[1], 5);
    EXPECT_EQ(dest[2], 6);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyExceptionSafety) {
    throw_on_copy::reset(3);
    throw_on_copy src[] = {1, 2, 3, 4, 5};
    throw_on_copy* dest = allocate<throw_on_copy>(5);
    EXPECT_THROW(uninitialized_copy(begin(src), end(src), dest), exception);
    EXPECT_EQ(throw_on_copy::destructions, 2u);
    for (int i = 0; i < 5; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyNTrivial) {
    int src[] = {7, 8, 9, 10, 11};
    int* dest = allocate<int>(4);
    auto result = uninitialized_copy_n(begin(src), 4, dest);
    EXPECT_EQ(result, dest + 4);
    EXPECT_EQ(dest[0], 7);
    EXPECT_EQ(dest[1], 8);
    EXPECT_EQ(dest[2], 9);
    EXPECT_EQ(dest[3], 10);
    for (int i = 0; i < 4; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyNZeroCount) {
    int src[] = {1, 2};
    int* dest = allocate<int>(1);
    auto result = uninitialized_copy_n(begin(src), 0, dest);
    EXPECT_EQ(result, dest);
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyNNonTrivial) {
    construct_counter::reset();
    construct_counter src[] = {100, 200, 300};
    construct_counter* dest = allocate<construct_counter>(2);
    auto result = uninitialized_copy_n(begin(src), 2, dest);
    EXPECT_EQ(result, dest + 2);
    EXPECT_EQ(construct_counter::constructions, 5u);
    EXPECT_EQ(dest[0].value, 100);
    EXPECT_EQ(dest[1].value, 200);
    for (int i = 0; i < 2; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedCopyNExceptionSafety) {
    throw_on_copy::reset(2);
    throw_on_copy src[] = {1, 2, 3};
    throw_on_copy* dest = allocate<throw_on_copy>(3);
    EXPECT_THROW(uninitialized_copy_n(begin(src), 3, dest), exception);
    EXPECT_EQ(throw_on_copy::destructions, 1u);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillTrivial) {
    int* dest = allocate<int>(4);
    uninitialized_fill(dest, dest + 4, 42);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(dest[i], 42);
    }
    for (int i = 0; i < 4; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillEmptyRange) {
    int* dest = allocate<int>(1);
    uninitialized_fill(dest, dest, 99);
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillNonTrivial) {
    construct_counter::reset();
    construct_counter* dest = allocate<construct_counter>(3);
    construct_counter val(55);
    auto new_val_constructions = construct_counter::constructions;
    uninitialized_fill(dest, dest + 3, val);
    EXPECT_EQ(construct_counter::constructions, new_val_constructions + 3);
    EXPECT_EQ(construct_counter::destructions, 0u);
    EXPECT_EQ(dest[0].value, 55);
    EXPECT_EQ(dest[1].value, 55);
    EXPECT_EQ(dest[2].value, 55);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillExceptionSafety) {
    throw_on_copy::reset(2);
    throw_on_copy* dest = allocate<throw_on_copy>(3);
    throw_on_copy val(99);
    EXPECT_THROW(uninitialized_fill(dest, dest + 3, val), exception);
    EXPECT_EQ(throw_on_copy::destructions, 1u);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillNTrivial) {
    int* dest = allocate<int>(5);
    auto result = uninitialized_fill_n(dest, 5, 13);
    EXPECT_EQ(result, dest + 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(dest[i], 13);
    }
    for (int i = 0; i < 5; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillNZero) {
    int* dest = allocate<int>(1);
    auto result = uninitialized_fill_n(dest, 0, 7);
    EXPECT_EQ(result, dest);
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillNNonTrivial) {
    construct_counter::reset();
    construct_counter* dest = allocate<construct_counter>(2);
    construct_counter val(77);
    auto init_constructions = construct_counter::constructions;
    auto result = uninitialized_fill_n(dest, 2, val);
    EXPECT_EQ(result, dest + 2);
    EXPECT_EQ(construct_counter::constructions, init_constructions + 2);
    EXPECT_EQ(dest[0].value, 77);
    EXPECT_EQ(dest[1].value, 77);
    for (int i = 0; i < 2; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedFillNExceptionSafety) {
    throw_on_copy::reset(3);
    throw_on_copy* dest = allocate<throw_on_copy>(4);
    throw_on_copy val(44);
    EXPECT_THROW(uninitialized_fill_n(dest, 4, val), exception);
    EXPECT_EQ(throw_on_copy::destructions, 2u);
    for (int i = 0; i < 4; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedMoveTrivial) {
    int src[] = {1, 2, 3, 4};
    int* dest = allocate<int>(4);
    auto result = uninitialized_move(begin(src), end(src), dest);
    EXPECT_EQ(result, dest + 4);
    EXPECT_EQ(dest[0], 1);
    EXPECT_EQ(dest[1], 2);
    EXPECT_EQ(dest[2], 3);
    EXPECT_EQ(dest[3], 4);
    for (int i = 0; i < 4; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedMoveNonTrivial) {
    construct_counter::reset();
    construct_counter src[] = {10, 20, 30};
    construct_counter* dest = allocate<construct_counter>(3);
    auto result = uninitialized_move(begin(src), end(src), dest);
    EXPECT_EQ(result, dest + 3);
    EXPECT_EQ(construct_counter::constructions, 6u);
    EXPECT_EQ(dest[0].value, 10);
    EXPECT_EQ(dest[1].value, 20);
    EXPECT_EQ(dest[2].value, 30);
    EXPECT_EQ(src[0].value, -1);
    EXPECT_EQ(src[1].value, -1);
    EXPECT_EQ(src[2].value, -1);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedMoveEmptyRange) {
    int src[] = {1};
    int* dest = allocate<int>(1);
    auto result = uninitialized_move(src, src, dest);
    EXPECT_EQ(result, dest);
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedMoveNTrivial) {
    int src[] = {5, 6, 7, 8, 9};
    int* dest = allocate<int>(3);
    auto result = uninitialized_move_n(src, 3, dest);
    EXPECT_EQ(result, dest + 3);
    EXPECT_EQ(dest[0], 5);
    EXPECT_EQ(dest[1], 6);
    EXPECT_EQ(dest[2], 7);
    for (int i = 0; i < 3; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedMoveNNonTrivial) {
    construct_counter::reset();
    construct_counter src[] = {100, 200, 300};
    construct_counter* dest = allocate<construct_counter>(2);
    auto result = uninitialized_move_n(src, 2, dest);
    EXPECT_EQ(result, dest + 2);
    EXPECT_EQ(construct_counter::constructions, 5u);
    EXPECT_EQ(dest[0].value, 100);
    EXPECT_EQ(dest[1].value, 200);
    EXPECT_EQ(src[0].value, -1);
    EXPECT_EQ(src[1].value, -1);
    EXPECT_EQ(src[2].value, 300);
    for (int i = 0; i < 2; ++i) {
        destroy(dest + i);
    }
    deallocate(dest);
}

TEST_F(UninitializedMemoryTest, UninitializedMoveNZeroCount) {
    int src[] = {1};
    int* dest = allocate<int>(1);
    auto result = uninitialized_move_n(src, 0, dest);
    EXPECT_EQ(result, dest);
    deallocate(dest);
}

TEST(BitTest, popcount64_zero) { EXPECT_EQ(popcount64(0ULL), 0); }

TEST(BitTest, popcount64_ones) { EXPECT_EQ(popcount64(0xFFFFFFFFFFFFFFFFULL), 64); }

TEST(BitTest, popcount64_alternating) {
    EXPECT_EQ(popcount64(0x5555555555555555ULL), 32);
    EXPECT_EQ(popcount64(0xAAAAAAAAAAAAAAAAULL), 32);
}

TEST(BitTest, popcount64_single_bit) {
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(popcount64(1ULL << i), 1);
    }
}

TEST(BitTest, popcount64_few_bits) {
    EXPECT_EQ(popcount64(0x8000000000000001ULL), 2);
    EXPECT_EQ(popcount64(0x1111111111111111ULL), 16);
}

TEST(BitTest, clz64_zero) { EXPECT_EQ(clz64(0ULL), 64); }

TEST(BitTest, clz64_one) { EXPECT_EQ(clz64(1ULL), 63); }

TEST(BitTest, clz64_max) { EXPECT_EQ(clz64(0x8000000000000000ULL), 0); }

TEST(BitTest, clz64_pattern) {
    EXPECT_EQ(clz64(0x0000FFFFFFFFFFFFULL), 16);
    EXPECT_EQ(clz64(0x00FFFFFFFFFFFFFFULL), 8);
    EXPECT_EQ(clz64(0x0FFFFFFFFFFFFFFFULL), 4);
    EXPECT_EQ(clz64(0x3FFFFFFFFFFFFFFFULL), 2);
    EXPECT_EQ(clz64(0x7FFFFFFFFFFFFFFFULL), 1);
}

TEST(BitTest, popcount32_zero) { EXPECT_EQ(popcount32(0U), 0); }

TEST(BitTest, popcount32_ones) { EXPECT_EQ(popcount32(0xFFFFFFFFU), 32); }

TEST(BitTest, popcount32_alternating) {
    EXPECT_EQ(popcount32(0x55555555U), 16);
    EXPECT_EQ(popcount32(0xAAAAAAAAU), 16);
}

TEST(BitTest, popcount32_single_bit) {
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(popcount32(1U << i), 1);
    }
}

TEST(BitTest, clz32_zero) { EXPECT_EQ(clz32(0U), 32); }

TEST(BitTest, clz32_one) { EXPECT_EQ(clz32(1U), 31); }

TEST(BitTest, clz32_max) { EXPECT_EQ(clz32(0x80000000U), 0); }

TEST(BitTest, clz32_pattern) {
    EXPECT_EQ(clz32(0x0000FFFFU), 16);
    EXPECT_EQ(clz32(0x00FFFFFFU), 8);
    EXPECT_EQ(clz32(0x0FFFFFFFU), 4);
    EXPECT_EQ(clz32(0x3FFFFFFFU), 2);
    EXPECT_EQ(clz32(0x7FFFFFFFU), 1);
}

TEST(BitTest, popcount_ptr_zero) { EXPECT_EQ(popcount(uintptr_t(0)), 0); }

TEST(BitTest, popcount_ptr_ones) { EXPECT_EQ(popcount(~uintptr_t(0)), numeric_traits<uintptr_t>::digits); }

TEST(BitTest, popcount_ptr_single) {
    for (int i = 0; i < numeric_traits<uintptr_t>::digits; ++i) {
        EXPECT_EQ(popcount(uintptr_t(1) << i), 1);
    }
}

TEST(BitTest, countl_zero_ptr_zero) { EXPECT_EQ(countl_zero(uintptr_t(0)), numeric_traits<uintptr_t>::digits); }

TEST(BitTest, countl_zero_ptr_one) { EXPECT_EQ(countl_zero(uintptr_t(1)), numeric_traits<uintptr_t>::digits - 1); }

TEST(BitTest, countl_zero_ptr_max) {
    uintptr_t max_bit = uintptr_t(1) << (numeric_traits<uintptr_t>::digits - 1);
    EXPECT_EQ(countl_zero(max_bit), 0);
}

TEST(BitTest, countl_one_ptr_zero) { EXPECT_EQ(countl_one(uintptr_t(0)), 0); }

TEST(BitTest, countl_one_ptr_max) { EXPECT_EQ(countl_one(~uintptr_t(0)), numeric_traits<uintptr_t>::digits); }

TEST(BitTest, countl_one_ptr_pattern) {
    uintptr_t val = ~uintptr_t(0) << 4;
    EXPECT_EQ(countl_one(val), numeric_traits<uintptr_t>::digits - 4);
}

TEST(BitTest, countr_zero_ptr_zero) { EXPECT_EQ(countr_zero(uintptr_t(0)), numeric_traits<uintptr_t>::digits); }

TEST(BitTest, countr_zero_ptr_one) { EXPECT_EQ(countr_zero(uintptr_t(1)), 0); }

TEST(BitTest, countr_zero_ptr_low_bit) {
    EXPECT_EQ(countr_zero(uintptr_t(12)), 2);
    EXPECT_EQ(countr_zero(uintptr_t(8)), 3);
}

TEST(BitTest, countr_one_ptr_zero) { EXPECT_EQ(countr_one(uintptr_t(0)), 0); }

TEST(BitTest, countr_one_ptr_max) { EXPECT_EQ(countr_one(~uintptr_t(0)), numeric_traits<uintptr_t>::digits); }

TEST(BitTest, countr_one_ptr_pattern) {
    EXPECT_EQ(countr_one(uintptr_t(3)), 2);
    EXPECT_EQ(countr_one(uintptr_t(7)), 3);
    EXPECT_EQ(countr_one(uintptr_t(0x15)), 1);
}

TEST(BitTest, lowest_set_bit_pos_zero) { EXPECT_EQ(lowest_set_bit_pos(intptr_t(0)), -1); }

TEST(BitTest, lowest_set_bit_pos_values) {
    EXPECT_EQ(lowest_set_bit_pos(intptr_t(1)), 0);
    EXPECT_EQ(lowest_set_bit_pos(intptr_t(2)), 1);
    EXPECT_EQ(lowest_set_bit_pos(intptr_t(0x80)), 7);
    EXPECT_EQ(lowest_set_bit_pos(intptr_t(-1)), 0);
}

TEST(BitTest, highest_set_bit_pos_zero) { EXPECT_EQ(highest_set_bit_pos(intptr_t(0)), -1); }

TEST(BitTest, highest_set_bit_pos_values) {
    EXPECT_EQ(highest_set_bit_pos(intptr_t(1)), 0);
    EXPECT_EQ(highest_set_bit_pos(intptr_t(2)), 1);
    EXPECT_EQ(highest_set_bit_pos(intptr_t(0x80)), 7);
}

TEST(BitTest, highest_set_bit_pos_negative) {
    EXPECT_EQ(highest_set_bit_pos(intptr_t(-1)), numeric_traits<uintptr_t>::digits - 1);
}

TEST(BitTest, parity32_even) {
    EXPECT_FALSE(parity32(0U));
    EXPECT_FALSE(parity32(0xFFFFFFFFU));
    EXPECT_FALSE(parity32(0x55555555U));
}

TEST(BitTest, parity32_odd) {
    EXPECT_TRUE(parity32(1U));
    EXPECT_FALSE(parity32(0x80000001U));
    EXPECT_TRUE(parity32(0xAAAAAAABU));
}

TEST(BitTest, parity64_even) {
    EXPECT_FALSE(parity64(0ULL));
    EXPECT_FALSE(parity64(0xFFFFFFFFFFFFFFFFULL));
}

TEST(BitTest, parity64_odd) { EXPECT_TRUE(parity64(1ULL)); }

TEST(BitTest, parity_ptr) {
    EXPECT_FALSE(parity(uintptr_t(0)));
    EXPECT_FALSE(parity(~uintptr_t(0)));
    EXPECT_TRUE(parity(uintptr_t(1)));
}

TEST(BitTest, bit_width_zero) { EXPECT_EQ(bit_width(uintptr_t(0)), 0); }

TEST(BitTest, bit_width_values) {
    EXPECT_EQ(bit_width(uintptr_t(1)), 1);
    EXPECT_EQ(bit_width(uintptr_t(2)), 2);
    EXPECT_EQ(bit_width(uintptr_t(3)), 2);
    EXPECT_EQ(bit_width(uintptr_t(4)), 3);
    EXPECT_EQ(bit_width(~uintptr_t(0)), numeric_traits<uintptr_t>::digits);
    uintptr_t max = ~uintptr_t(0);
    EXPECT_EQ(bit_width(max), numeric_traits<uintptr_t>::digits);
}

TEST(BitTest, bit_floor_zero) { EXPECT_EQ(bit_floor(uintptr_t(0)), uintptr_t(0)); }

TEST(BitTest, bit_floor_pow2) {
    for (int i = 0; i < numeric_traits<uintptr_t>::digits; ++i) {
        uintptr_t v = uintptr_t(1) << i;
        EXPECT_EQ(bit_floor(v), v);
    }
}

TEST(BitTest, bit_floor_non_pow2) {
    EXPECT_EQ(bit_floor(uintptr_t(3)), uintptr_t(2));
    EXPECT_EQ(bit_floor(uintptr_t(5)), uintptr_t(4));
    EXPECT_EQ(bit_floor(uintptr_t(0x7FFF)), uintptr_t(0x4000));
}

TEST(BitTest, bit_ceil_zero_or_one) {
    EXPECT_EQ(bit_ceil(uintptr_t(0)), 1ULL);
    EXPECT_EQ(bit_ceil(uintptr_t(1)), 1ULL);
}

TEST(BitTest, bit_ceil_pow2) {
    for (int i = 1; i < numeric_traits<uintptr_t>::digits; ++i) {
        uintptr_t v = uintptr_t(1) << i;
        EXPECT_EQ(bit_ceil(v), uint64_t(v));
    }
}

TEST(BitTest, bit_ceil_non_pow2) {
    EXPECT_EQ(bit_ceil(uintptr_t(3)), 4ULL);
    EXPECT_EQ(bit_ceil(uintptr_t(5)), 8ULL);
    EXPECT_EQ(bit_ceil(uintptr_t(0x7FFF)), 0x8000ULL);
}

TEST(BitTest, has_single_bit_false) {
    EXPECT_FALSE(has_single_bit(uintptr_t(0)));
    EXPECT_FALSE(has_single_bit(uintptr_t(3)));
    EXPECT_FALSE(has_single_bit(uintptr_t(0x7FFFFFFF)));
}

TEST(BitTest, has_single_bit_true) {
    for (int i = 0; i < numeric_traits<uintptr_t>::digits; ++i) {
        EXPECT_TRUE(has_single_bit(uintptr_t(1) << i));
    }
}

TEST(BitTest, rotate_l32_identity) {
    EXPECT_EQ(rotate_l32(0x12345678U, 0), 0x12345678U);
    EXPECT_EQ(rotate_l32(0x12345678U, 32), 0x12345678U);
    EXPECT_EQ(rotate_l32(0x12345678U, -32), 0x12345678U);
}

TEST(BitTest, rotate_l32_by_one) {
    EXPECT_EQ(rotate_l32(0x80000000U, 1), 1U);
    EXPECT_EQ(rotate_l32(0x00000001U, 1), 2U);
}

TEST(BitTest, rotate_r32_equiv_left_neg) {
    EXPECT_EQ(rotate_r32(0x80000001U, 1), rotate_l32(0x80000001U, -1));
    EXPECT_EQ(rotate_r32(0x80000001U, 2), rotate_l32(0x80000001U, -2));
}

TEST(BitTest, rotate_r32_by_one) { EXPECT_EQ(rotate_r32(1U, 1), 0x80000000U); }

TEST(BitTest, rotate_l64_identity) {
    EXPECT_EQ(rotate_l64(0x123456789ABCDEF0ULL, 0), 0x123456789ABCDEF0ULL);
    EXPECT_EQ(rotate_l64(0x123456789ABCDEF0ULL, 64), 0x123456789ABCDEF0ULL);
}

TEST(BitTest, rotate_l64_by_one) {
    EXPECT_EQ(rotate_l64(0x8000000000000000ULL, 1), 1ULL);
    EXPECT_EQ(rotate_l64(1ULL, 1), 2ULL);
}

TEST(BitTest, rotate_r64_by_one) { EXPECT_EQ(rotate_r64(1ULL, 1), 0x8000000000000000ULL); }

TEST(BitTest, rotate_ptr_left) {
    uintptr_t one = 1;
    uintptr_t high_bit = one << (numeric_traits<uintptr_t>::digits - 1);
    EXPECT_EQ(rotate_l(high_bit, 1), uintptr_t(1));
    EXPECT_EQ(rotate_r(one, 1), high_bit);
}

TEST(BitTest, bit_extract_single_bit) {
    uintptr_t x = 0x1234;
    EXPECT_EQ(bit_extract(x, 0, 1), 0U);
    EXPECT_EQ(bit_extract(x, 2, 1), 1U);
}

TEST(BitTest, bit_extract_field) {
    uintptr_t x = 0xABCD;
    EXPECT_EQ(bit_extract(x, 4, 4), 0xCU);
}

TEST(BitTest, bit_extract_full_width) {
    uintptr_t x = ~uintptr_t(0);
    EXPECT_EQ(bit_extract(x, 0, numeric_traits<uintptr_t>::digits - 1),
              x & ((uintptr_t(1) << (numeric_traits<uintptr_t>::digits - 1)) - 1));
}

TEST(BitTest, bit_insert_clear_and_set) {
    uintptr_t x = 0xFFFF;
    uintptr_t result = bit_insert(x, 0xA, 4, 4);
    EXPECT_EQ(result, (0xFFFF & ~(0xF << 4)) | (0xA << 4));
}

TEST(BitTest, bit_insert_preserve_other_bits) {
    uintptr_t x = 0x12345678;
    uintptr_t bits = 0xE;
    int pos = 8, len = 4;
    uintptr_t res = bit_insert(x, bits, pos, len);
    EXPECT_EQ(res & (0xF << pos), bits << pos);
    EXPECT_EQ(res & ~(uintptr_t(0xF) << pos), x & ~(uintptr_t(0xF) << pos));
}

TEST(BitTest, reverse_bits32_zero) { EXPECT_EQ(reverse_bits32(0U), 0U); }

TEST(BitTest, reverse_bits32_all_ones) { EXPECT_EQ(reverse_bits32(0xFFFFFFFFU), 0xFFFFFFFFU); }

TEST(BitTest, reverse_bits32_pattern) {
    EXPECT_EQ(reverse_bits32(0x80000000U), 1U);
    EXPECT_EQ(reverse_bits32(0x00000001U), 0x80000000U);
    EXPECT_EQ(reverse_bits32(0x55555555U), 0xAAAAAAAAU);
}

TEST(BitTest, reverse_bits64_zero) { EXPECT_EQ(reverse_bits64(0ULL), 0ULL); }

TEST(BitTest, reverse_bits64_all_ones) { EXPECT_EQ(reverse_bits64(0xFFFFFFFFFFFFFFFFULL), 0xFFFFFFFFFFFFFFFFULL); }

TEST(BitTest, reverse_bits64_pattern) {
    EXPECT_EQ(reverse_bits64(0x8000000000000000ULL), 1ULL);
    EXPECT_EQ(reverse_bits64(1ULL), 0x8000000000000000ULL);
}

TEST(BitTest, reverse_bits_ptr) {
    uintptr_t one = 1;
    EXPECT_EQ(reverse_bits(one), one << (numeric_traits<uintptr_t>::digits - 1));
}

TEST(BitTest, mask_from_to_single_bit) {
    EXPECT_EQ(mask_from_to(0, 0), uintptr_t(1));
    EXPECT_EQ(mask_from_to(5, 5), uintptr_t(1) << 5);
}

TEST(BitTest, mask_from_to_range) {
    EXPECT_EQ(mask_from_to(0, 3), uintptr_t(0xF));
    EXPECT_EQ(mask_from_to(4, 7), uintptr_t(0xF0));
    EXPECT_EQ(mask_from_to(0, numeric_traits<uintptr_t>::digits - 2),
              (uintptr_t(1) << (numeric_traits<uintptr_t>::digits - 1)) - 1);
}

TEST(BitTest, mask_from_to_middle) { EXPECT_EQ(mask_from_to(8, 15), uintptr_t(0xFF00)); }

TEST(EndianConstantsTest, LittleAndBigAreOpposite) { EXPECT_EQ(endian::is_little_endian, !endian::is_big_endian); }

TEST(EndianConstantsTest, RuntimeMatchesCompileTime) {
    EXPECT_EQ(endian::is_little_endian_runtime(), endian::is_little_endian);
}

TEST(ByteswapTest, Uint16) {
    EXPECT_EQ(endian::byteswap16(0x1234), 0x3412);
    EXPECT_EQ(endian::byteswap16(0x0000), 0x0000);
    EXPECT_EQ(endian::byteswap16(0xFFFF), 0xFFFF);
}

TEST(ByteswapTest, Uint32) {
    EXPECT_EQ(endian::byteswap32(0x12345678), 0x78563412);
    EXPECT_EQ(endian::byteswap32(0x00000000), 0x00000000);
    EXPECT_EQ(endian::byteswap32(0xFFFFFFFF), 0xFFFFFFFF);
}

TEST(ByteswapTest, Uint64) {
    EXPECT_EQ(endian::byteswap64(0x0123456789ABCDEF), 0xEFCDAB8967452301);
    EXPECT_EQ(endian::byteswap64(0x0000000000000000), 0x0000000000000000);
    EXPECT_EQ(endian::byteswap64(0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF);
}

TEST(SwapEndianTest, Uint16) {
    uint16_t val = 0xABCD;
    EXPECT_EQ(endian::swap_endian(val), endian::byteswap16(val));
}

TEST(SwapEndianTest, Uint32) {
    uint32_t val = 0x12345678;
    EXPECT_EQ(endian::swap_endian(val), endian::byteswap32(val));
}

TEST(SwapEndianTest, Uint64) {
    uint64_t val = 0x0123456789ABCDEF;
    EXPECT_EQ(endian::swap_endian(val), endian::byteswap64(val));
}

TEST(HostNetworkTest, RoundTrip) {
    test_host_network_round_trip<uint16_t>(0x1234);
    test_host_network_round_trip<uint32_t>(0x12345678);
    test_host_network_round_trip<uint64_t>(0x0123456789ABCDEF);
}

TEST(HostNetworkTest, CorrectnessAgainstSwap) {
    if (endian::is_little_endian) {
        EXPECT_EQ(endian::host_to_network<uint16_t>(0x1122), endian::byteswap16(0x1122));
        EXPECT_EQ(endian::host_to_network<uint32_t>(0x11223344), endian::byteswap32(0x11223344));
        EXPECT_EQ(endian::host_to_network<uint64_t>(0x1122334455667788), endian::byteswap64(0x1122334455667788));
    } else {
        EXPECT_EQ(endian::host_to_network<uint16_t>(0x1122), 0x1122);
        EXPECT_EQ(endian::host_to_network<uint32_t>(0x11223344), 0x11223344);
        EXPECT_EQ(endian::host_to_network<uint64_t>(0x1122334455667788), 0x1122334455667788);
    }
}

TEST(HostLETest, RoundTrip) {
    auto val16 = uint16_t(0xAABB);
    auto val32 = uint32_t(0xAABBCCDD);
    auto val64 = uint64_t(0xAABBCCDDEEFF0011);
    EXPECT_EQ(endian::le_to_host(endian::host_to_le(val16)), val16);
    EXPECT_EQ(endian::le_to_host(endian::host_to_le(val32)), val32);
    EXPECT_EQ(endian::le_to_host(endian::host_to_le(val64)), val64);
}

TEST(HostLETest, CorrectnessAgainstSwap) {
    if (endian::is_little_endian) {
        EXPECT_EQ(endian::host_to_le<uint16_t>(0x1122), 0x1122);
        EXPECT_EQ(endian::host_to_le<uint32_t>(0x11223344), 0x11223344);
        EXPECT_EQ(endian::host_to_le<uint64_t>(0x1122334455667788), 0x1122334455667788);
    } else {
        EXPECT_EQ(endian::host_to_le<uint16_t>(0x1122), endian::byteswap16(0x1122));
        EXPECT_EQ(endian::host_to_le<uint32_t>(0x11223344), endian::byteswap32(0x11223344));
        EXPECT_EQ(endian::host_to_le<uint64_t>(0x1122334455667788), endian::byteswap64(0x1122334455667788));
    }
}

TEST(HostBETest, RoundTrip) {
    auto val16 = uint16_t(0x9988);
    auto val32 = uint32_t(0x99887766);
    auto val64 = uint64_t(0x9988776655443322);
    EXPECT_EQ(endian::be_to_host(endian::host_to_be(val16)), val16);
    EXPECT_EQ(endian::be_to_host(endian::host_to_be(val32)), val32);
    EXPECT_EQ(endian::be_to_host(endian::host_to_be(val64)), val64);
}

TEST(HostBETest, CorrectnessAgainstSwap) {
    if (endian::is_big_endian) {
        EXPECT_EQ(endian::host_to_be<uint16_t>(0x1122), 0x1122);
        EXPECT_EQ(endian::host_to_be<uint32_t>(0x11223344), 0x11223344);
        EXPECT_EQ(endian::host_to_be<uint64_t>(0x1122334455667788), 0x1122334455667788);
    } else {
        EXPECT_EQ(endian::host_to_be<uint16_t>(0x1122), endian::byteswap16(0x1122));
        EXPECT_EQ(endian::host_to_be<uint32_t>(0x11223344), endian::byteswap32(0x11223344));
        EXPECT_EQ(endian::host_to_be<uint64_t>(0x1122334455667788), endian::byteswap64(0x1122334455667788));
    }
}

TEST(ReadLETest, Uint16) {
    const byte_t data[] = {0x34, 0x12};
    EXPECT_EQ(endian::read_le16(data), 0x1234);
}

TEST(ReadLETest, Uint32) {
    const byte_t data[] = {0x78, 0x56, 0x34, 0x12};
    EXPECT_EQ(endian::read_le32(data), 0x12345678);
}

TEST(ReadLETest, Uint64) {
    const byte_t data[] = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    EXPECT_EQ(endian::read_le64(data), 0x0123456789ABCDEF);
}

TEST(ReadBETest, Uint16) {
    const byte_t data[] = {0x12, 0x34};
    EXPECT_EQ(endian::read_be16(data), 0x1234);
}

TEST(ReadBETest, Uint32) {
    const byte_t data[] = {0x12, 0x34, 0x56, 0x78};
    EXPECT_EQ(endian::read_be32(data), 0x12345678);
}

TEST(ReadBETest, Uint64) {
    const byte_t data[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(endian::read_be64(data), 0x0123456789ABCDEF);
}

TEST(WriteLETest, Uint16) {
    byte_t dest[2] = {};
    endian::write_le16(dest, 0x1234);
    EXPECT_EQ(dest[0], 0x34);
    EXPECT_EQ(dest[1], 0x12);
}

TEST(WriteLETest, Uint32) {
    byte_t dest[4] = {};
    endian::write_le32(dest, 0x12345678);
    EXPECT_EQ(dest[0], 0x78);
    EXPECT_EQ(dest[1], 0x56);
    EXPECT_EQ(dest[2], 0x34);
    EXPECT_EQ(dest[3], 0x12);
}

TEST(WriteLETest, Uint64) {
    byte_t dest[8] = {};
    endian::write_le64(dest, 0x0123456789ABCDEF);
    EXPECT_EQ(dest[0], 0xEF);
    EXPECT_EQ(dest[1], 0xCD);
    EXPECT_EQ(dest[2], 0xAB);
    EXPECT_EQ(dest[3], 0x89);
    EXPECT_EQ(dest[4], 0x67);
    EXPECT_EQ(dest[5], 0x45);
    EXPECT_EQ(dest[6], 0x23);
    EXPECT_EQ(dest[7], 0x01);
}

TEST(WriteBETest, Uint16) {
    byte_t dest[2] = {};
    endian::write_be16(dest, 0x1234);
    EXPECT_EQ(dest[0], 0x12);
    EXPECT_EQ(dest[1], 0x34);
}

TEST(WriteBETest, Uint32) {
    byte_t dest[4] = {};
    endian::write_be32(dest, 0x12345678);
    EXPECT_EQ(dest[0], 0x12);
    EXPECT_EQ(dest[1], 0x34);
    EXPECT_EQ(dest[2], 0x56);
    EXPECT_EQ(dest[3], 0x78);
}

TEST(WriteBETest, Uint64) {
    byte_t dest[8] = {};
    endian::write_be64(dest, 0x0123456789ABCDEF);
    EXPECT_EQ(dest[0], 0x01);
    EXPECT_EQ(dest[1], 0x23);
    EXPECT_EQ(dest[2], 0x45);
    EXPECT_EQ(dest[3], 0x67);
    EXPECT_EQ(dest[4], 0x89);
    EXPECT_EQ(dest[5], 0xAB);
    EXPECT_EQ(dest[6], 0xCD);
    EXPECT_EQ(dest[7], 0xEF);
}

TEST(ReadWriteRoundTrip, LE16) {
    byte_t buf[2];
    uint16_t original = 0xCAFE;
    endian::write_le16(buf, original);
    EXPECT_EQ(endian::read_le16(buf), original);
}

TEST(ReadWriteRoundTrip, BE16) {
    byte_t buf[2];
    uint16_t original = 0xBEEF;
    endian::write_be16(buf, original);
    EXPECT_EQ(endian::read_be16(buf), original);
}

TEST(ReadWriteRoundTrip, LE32) {
    byte_t buf[4];
    uint32_t original = 0xDEADBEEF;
    endian::write_le32(buf, original);
    EXPECT_EQ(endian::read_le32(buf), original);
}

TEST(ReadWriteRoundTrip, BE32) {
    byte_t buf[4];
    uint32_t original = 0xCAFEBABE;
    endian::write_be32(buf, original);
    EXPECT_EQ(endian::read_be32(buf), original);
}

TEST(ReadWriteRoundTrip, LE64) {
    byte_t buf[8];
    uint64_t original = 0x0123456789ABCDEF;
    endian::write_le64(buf, original);
    EXPECT_EQ(endian::read_le64(buf), original);
}

TEST(ReadWriteRoundTrip, BE64) {
    byte_t buf[8];
    uint64_t original = 0xFEDCBA9876543210;
    endian::write_be64(buf, original);
    EXPECT_EQ(endian::read_be64(buf), original);
}

TEST(UniquePtrTest, DefaultConstruction) {
    unique_ptr<int> p;
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(p);
}

TEST(UniquePtrTest, NullptrConstruction) {
    unique_ptr<int> p(nullptr);
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(p);
}

TEST(UniquePtrTest, PointerConstruction) {
    int* raw = new int(5);
    unique_ptr<int> p(raw);
    EXPECT_EQ(p.get(), raw);
    EXPECT_TRUE(p);
    EXPECT_EQ(*p, 5);
}

TEST(UniquePtrTest, PointerDeleterConstructionCopy) {
    int count = 0;
    int* raw = new int(10);
    {
        counting_deleter<int> d(&count);
        unique_ptr<int, counting_deleter<int>> p(raw, move(d));
        EXPECT_EQ(p.get(), raw);
        EXPECT_EQ(*p, 10);
    }
    EXPECT_EQ(count, 1u);
}

TEST(UniquePtrTest, PointerDeleterConstructionMove) {
    int count = 0;
    int* raw = new int(10);
    {
        unique_ptr<int, move_only_deleter<int>> p(raw, move_only_deleter<int>{&count});
        EXPECT_EQ(p.get(), raw);
    }
    EXPECT_EQ(count, 1u);
}

TEST(UniquePtrTest, MoveConstruction) {
    int* raw = new int(20);
    unique_ptr<int> src(raw);
    unique_ptr<int> dst(move(src));
    EXPECT_EQ(dst.get(), raw);
    EXPECT_EQ(src.get(), nullptr);
    EXPECT_FALSE(src);
    EXPECT_TRUE(dst);
}

TEST(UniquePtrTest, MoveAssignment) {
    int* raw1 = new int(20);
    int* raw2 = new int(30);
    unique_ptr<int> p1(raw1);
    unique_ptr<int> p2(raw2);
    p1 = move(p2);
    EXPECT_EQ(p1.get(), raw2);
    EXPECT_EQ(p2.get(), nullptr);
    EXPECT_EQ(*p1, 30);
}

TEST(UniquePtrTest, NullptrAssignment) {
    unique_ptr<int> p(new int(5));
    p = nullptr;
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(p);
}

TEST(UniquePtrTest, CopyConstructionIsDeleted) { EXPECT_FALSE(is_copy_constructible<unique_ptr<int>>::value); }

TEST(UniquePtrTest, CopyAssignmentIsDeleted) { EXPECT_FALSE(is_copy_assignable<unique_ptr<int>>::value); }

TEST(UniquePtrTest, OperatorBool) {
    unique_ptr<int> p;
    EXPECT_FALSE(p);
    p.reset(new int(0));
    EXPECT_TRUE(p);
    p.reset();
    EXPECT_FALSE(p);
}

TEST(UniquePtrTest, OperatorDereference) {
    unique_ptr<int> p(new int(42));
    EXPECT_EQ(*p, 42);
    *p = 100;
    EXPECT_EQ(*p, 100);
}

TEST(UniquePtrTest, OperatorArrow) {
    unique_ptr<string> p(new string("hello"));
    EXPECT_EQ(p->size(), 5u);
    EXPECT_EQ(*p, "hello");
}

TEST(UniquePtrTest, Get) {
    int* raw = new int(99);
    unique_ptr<int> p(raw);
    EXPECT_EQ(p.get(), raw);
}

TEST(UniquePtrTest, Release) {
    int* raw = new int(7);
    unique_ptr<int> p(raw);
    int* released = p.release();
    EXPECT_EQ(released, raw);
    EXPECT_EQ(p.get(), nullptr);
    delete released;
}

TEST(UniquePtrTest, ResetNull) {
    unique_ptr<int> p(new int(1));
    p.reset();
    EXPECT_EQ(p.get(), nullptr);
}

TEST(UniquePtrTest, ResetPointer) {
    int* raw = new int(2);
    unique_ptr<int> p(new int(3));
    p.reset(raw);
    EXPECT_EQ(p.get(), raw);
    EXPECT_EQ(*p, 2);
}

TEST(UniquePtrTest, ResetPointerWithDeleter) {
    int count = 0;
    int* first = new int(10);
    int* second = new int(20);
    {
        counting_deleter<int> d(&count);
        unique_ptr<int, counting_deleter<int>> p(first, d);
        p.reset(second);
        EXPECT_EQ(p.get(), second);
    }
    EXPECT_EQ(count, 2u);
}

TEST(UniquePtrTest, Swap) {
    int* r1 = new int(1);
    int* r2 = new int(2);
    unique_ptr<int> p1(r1);
    unique_ptr<int> p2(r2);
    p1.swap(p2);
    EXPECT_EQ(p1.get(), r2);
    EXPECT_EQ(p2.get(), r1);
    EXPECT_EQ(*p1, 2);
    EXPECT_EQ(*p2, 1);
}

TEST(UniquePtrTest, FreeSwap) {
    int* r1 = new int(1);
    int* r2 = new int(2);
    unique_ptr<int> p1(r1);
    unique_ptr<int> p2(r2);
    swap(p1, p2);
    EXPECT_EQ(p1.get(), r2);
    EXPECT_EQ(p2.get(), r1);
}

TEST(UniquePtrTest, DestructorCallsDeleter) {
    int count = 0;
    {
        unique_ptr<int, counting_deleter<int>> p(new int(0), counting_deleter<int>{&count});
    }
    EXPECT_EQ(count, 1u);
}

TEST(UniquePtrDeleterTest, CopyDeleter) {
    int count = 0;
    counting_deleter d(&count);
    {
        unique_ptr<int, counting_deleter<int>> p(new int(0), d);
        unique_ptr<int, counting_deleter<int>> p2(move(p));
    }
    EXPECT_EQ(count, 1u);
}

TEST(UniquePtrDeleterTest, MoveOnlyDeleterMoveAssignment) {
    int count = 0;
    {
        unique_ptr<int, move_only_deleter<int>> p(new int(0), move_only_deleter<int>{&count});
        p = unique_ptr<int, move_only_deleter<int>>(new int(1), move_only_deleter<int>{&count});
    }
    EXPECT_EQ(count, 2u);
}

TEST(UniquePtrDeleterTest, GetDeleterRef) {
    int count = 0;
    counting_deleter<int> d(&count);
    unique_ptr<int, counting_deleter<int>> p(new int(0), d);
    EXPECT_EQ(p.get_deleter().count, &count);
    const auto& cp = p;
    EXPECT_EQ(cp.get_deleter().count, &count);
}

TEST(UniquePtrConversionTest, DerivedToBaseConversion) {
    unique_ptr<derived> d(new derived(10));
    unique_ptr<base> b(move(d));
    EXPECT_EQ(b->value, 10);
    EXPECT_EQ(d.get(), nullptr);
}

TEST(UniquePtrConversionTest, ConversionWithDeleter) {
    derived count = 0;
    unique_ptr<derived, counting_deleter<derived>> d(new derived(10), counting_deleter<derived>{&count});
    unique_ptr<base, counting_deleter<base>> b = dynamic_pointer_cast<base, counting_deleter<base>>(move(d));
    EXPECT_EQ(b->value, 10);
    EXPECT_EQ(d.get(), nullptr);
    b.reset();
    EXPECT_EQ(count.value, 1u);
}

TEST(UniquePtrConversionTest, InvalidConversionDisabled) {
    static_assert(!is_constructible<unique_ptr<base>, unique_ptr<other>>::value, "cannot convert unrelated pointers");
    static_assert(!is_constructible<unique_ptr<int>, unique_ptr<derived>>::value, "cannot convert unrelated pointers");
}

TEST(UniquePtrConversionTest, MoveAssignmentConversion) {
    unique_ptr<derived> d(new derived(20));
    unique_ptr<base> b;
    b = move(d);
    EXPECT_EQ(b->value, 20);
    EXPECT_EQ(d.get(), nullptr);
}

TEST(UniquePtrArrayTest, DefaultConstruction) {
    unique_ptr<int[]> p;
    EXPECT_EQ(p.get(), nullptr);
}

TEST(UniquePtrArrayTest, ArrayPointerConstruction) {
    int* raw = new int[3];
    raw[0] = 1;
    raw[1] = 2;
    raw[2] = 3;
    unique_ptr<int[]> p(raw);
    EXPECT_EQ(p.get(), raw);
    EXPECT_TRUE(p);
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[1], 2);
    EXPECT_EQ(p[2], 3);
}

TEST(UniquePtrArrayTest, MoveConstruction) {
    int* raw = new int[2]{10, 20};
    unique_ptr<int[]> src(raw);
    unique_ptr<int[]> dst(move(src));
    EXPECT_EQ(dst.get(), raw);
    EXPECT_EQ(src.get(), nullptr);
    EXPECT_EQ(dst[0], 10);
    EXPECT_EQ(dst[1], 20);
}

TEST(UniquePtrArrayTest, MoveAssignment) {
    int* raw1 = new int[1]{5};
    int* raw2 = new int[1]{6};
    unique_ptr<int[]> p1(raw1);
    unique_ptr<int[]> p2(raw2);
    p1 = move(p2);
    EXPECT_EQ(p1.get(), raw2);
    EXPECT_EQ(p2.get(), nullptr);
}

TEST(UniquePtrArrayTest, NullptrAssignment) {
    unique_ptr<int[]> p(new int[1]{7});
    p = nullptr;
    EXPECT_EQ(p.get(), nullptr);
}

TEST(UniquePtrArrayTest, OperatorIndex) {
    unique_ptr<int[]> p(new int[2]{100, 200});
    EXPECT_EQ(p[0], 100);
    EXPECT_EQ(p[1], 200);
    p[0] = 111;
    EXPECT_EQ(p[0], 111);
}

TEST(UniquePtrArrayTest, Release) {
    int* raw = new int[2]{9, 10};
    unique_ptr<int[]> p(raw);
    int* released = p.release();
    EXPECT_EQ(released, raw);
    EXPECT_EQ(p.get(), nullptr);
    delete[] released;
}

TEST(UniquePtrArrayTest, Reset) {
    int* raw1 = new int[2]{1, 2};
    int* raw2 = new int[3]{3, 4, 5};
    unique_ptr<int[]> p(raw1);
    p.reset(raw2);
    EXPECT_EQ(p.get(), raw2);
    EXPECT_EQ(p[0], 3);
}

TEST(UniquePtrArrayTest, ResetNull) {
    unique_ptr<int[]> p(new int[1]{0});
    p.reset(nullptr);
    EXPECT_EQ(p.get(), nullptr);
}

TEST(UniquePtrArrayTest, Swap) {
    int* r1 = new int[1]{10};
    int* r2 = new int[1]{20};
    unique_ptr<int[]> p1(r1);
    unique_ptr<int[]> p2(r2);
    p1.swap(p2);
    EXPECT_EQ(p1.get(), r2);
    EXPECT_EQ(p2.get(), r1);
}

TEST(UniquePtrArrayTest, BoolConversion) {
    unique_ptr<int[]> p;
    EXPECT_FALSE(p);
    p.reset(new int[1]);
    EXPECT_TRUE(p);
}

TEST(UniquePtrArrayTest, DeleterInvocation) {
    int count = 0;
    {
        unique_ptr<int[], counting_deleter<int>> p(new int(1), counting_deleter<int>{&count});
    }
    EXPECT_EQ(count, 1u);
}

TEST(UniquePtrArrayTest, DerivedArrayDisabled) {
    static_assert(!is_constructible<unique_ptr<base[]>, unique_ptr<derived[]>>::value,
                  "array conversion is not allowed");
}

TEST(UniquePtrComparisonTest, EqualSamePointer) {
    int* raw = new int(0);
    unique_ptr<int> a(raw);
    unique_ptr<int> b;
    EXPECT_EQ(a, a);
    EXPECT_NE(a, b);
    EXPECT_EQ(b, nullptr);
    EXPECT_EQ(nullptr, b);
    EXPECT_NE(a, nullptr);
    EXPECT_NE(nullptr, a);
}

TEST(UniquePtrComparisonTest, NullptrComparisons) {
    unique_ptr<int> p;
    EXPECT_TRUE(p == nullptr);
    EXPECT_FALSE(p != nullptr);
    EXPECT_FALSE(p < nullptr);
    EXPECT_FALSE(p > nullptr);
    EXPECT_TRUE(p <= nullptr);
    EXPECT_TRUE(p >= nullptr);

    unique_ptr<int> q(new int(0));
    EXPECT_FALSE(q == nullptr);
    EXPECT_TRUE(q != nullptr);
    EXPECT_FALSE(q < nullptr);
    EXPECT_TRUE(nullptr < q);
    EXPECT_TRUE(q > nullptr);
    EXPECT_TRUE(q >= nullptr);
    EXPECT_FALSE(nullptr > q);
}

TEST(MakeUniqueTest, SingleObject) {
    auto p = make_unique<int>(42);
    EXPECT_EQ(*p, 42);
    EXPECT_TRUE(p);
}

TEST(MakeUniqueTest, MultipleArgs) {
    auto p = make_unique<pair<int, double>>(1, 2.5);
    EXPECT_EQ(p->first, 1);
    EXPECT_DOUBLE_EQ(p->second, 2.5);
}

TEST(MakeUniqueTest, UnboundedArray) {
    auto p = make_unique<int[]>(5);
    p[0] = 1;
    p[4] = 2;
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[4], 2);
}

TEST(PointerCastTest, StaticCast) {
    unique_ptr<derived> d(new derived(100));
    auto b = static_pointer_cast<base, default_deleter<base>>(move(d));
    EXPECT_EQ(b->value, 100);
    EXPECT_EQ(d.get(), nullptr);
}

TEST(PointerCastTest, ConstCast) {
    unique_ptr<const int> p(new int(5));
    auto q = const_pointer_cast<int, default_deleter<int>>(move(p));
    *q = 10;
    EXPECT_EQ(*q, 10);
}

TEST(PointerCastTest, ReinterpretCast) {
    long* raw = new long(0x12345678);
    unique_ptr<long> p(raw);
    auto q = reinterpret_pointer_cast<unsigned long, default_deleter<unsigned long>>(move(p));
    EXPECT_EQ(q.get(), reinterpret_cast<unsigned long*>(raw));
}

TEST(PointerCastTest, DynamicCastSuccess) {
    unique_ptr<base> b(new derived(44));
    auto d = dynamic_pointer_cast<derived, default_deleter<derived>>(move(b));
    EXPECT_NE(d.get(), nullptr);
    EXPECT_EQ(d->value, 44);
}

TEST(PointerCastTest, DynamicCastFail) {
    unique_ptr<base> b(new base(44));
    auto d = dynamic_pointer_cast<derived, default_deleter<derived>>(move(b));
    EXPECT_EQ(d.get(), nullptr);
}

TEST(UniquePtrHashTest, HashValue) {
    int* raw = new int(42);
    unique_ptr<int> p(raw);
    hash<unique_ptr<int>> hasher;
    EXPECT_EQ(hasher(p), hash<int*>()(raw));
}

TEST(UniquePtrHashTest, NullHash) {
    unique_ptr<int> p;
    hash<unique_ptr<int>> hasher;
    EXPECT_EQ(hasher(p), hash<int*>()(nullptr));
}

TEST(UniquePtrEdgeTest, SelfMoveAssignment) {
    int* raw = new int(5);
    unique_ptr<int> p(raw);
    p = move(p);
    EXPECT_EQ(p.get(), raw);
    EXPECT_EQ(*p, 5);
}

TEST(UniquePtrEdgeTest, ResetWithNullDeleter) {
    unique_ptr<int> p(new int(5));
    p.reset();
    EXPECT_EQ(p.get(), nullptr);
}

TEST(UniquePtrEdgeTest, MoveFromReleasedPointer) {
    unique_ptr<int> p(new int(7));
    int* raw = p.release();
    unique_ptr<int> q(raw);
    EXPECT_EQ(*q, 7);
    EXPECT_EQ(p.get(), nullptr);
}

TEST(UniquePtrEdgeTest, DeleterWithReference) {
    int count = 0;
    counting_deleter<int> d(&count);
    using ref_deleter = counting_deleter<int>&;
    {
        unique_ptr<int, ref_deleter> p(new int(0), d);
    }
    EXPECT_EQ(count, 1u);
}

TEST(UniquePtrEdgeTest, NonAssignableDeleterConstruction) {
    int count = 0;
    non_assignable_deleter<int> d(&count);
    unique_ptr<int, non_assignable_deleter<int>> p(new int(0), d);
    EXPECT_EQ(p.get_deleter().count, &count);
}

TEST(SharedPtrTest, DefaultConstruct) {
    shared_ptr<int> sp;
    EXPECT_FALSE(sp);
    EXPECT_EQ(sp.use_count(), 0);
    EXPECT_TRUE(sp.unique());
}

TEST(SharedPtrTest, NullptrConstruct) {
    shared_ptr<int> sp(nullptr);
    EXPECT_FALSE(sp);
    EXPECT_EQ(sp.use_count(), 0);
}

TEST(SharedPtrTest, RawPointerConstruct) {
    shared_ptr<int> sp(new int(42));
    EXPECT_TRUE(sp);
    EXPECT_EQ(*sp, 42);
    EXPECT_EQ(sp.use_count(), 1);
    EXPECT_TRUE(sp.unique());
}

TEST(SharedPtrTest, CustomDeleterConstruct) {
    delete_counter = 0;
    {
        shared_ptr<delete_counter_id> sp(new delete_counter_id(1), delete_delete_counter);
        EXPECT_EQ(sp->id, 1);
        EXPECT_EQ(delete_counter, 0);
    }
    EXPECT_EQ(delete_counter, 1);
}

TEST(SharedPtrTest, FromUniquePtr) {
    auto up = make_unique<int>(10);
    int* raw = up.get();
    shared_ptr<int> sp(move(up));
    EXPECT_FALSE(up);
    EXPECT_EQ(sp.get(), raw);
    EXPECT_EQ(*sp, 10);
}

TEST(SharedPtrTest, CopyConstruct) {
    shared_ptr<int> sp1(new int(5));
    shared_ptr<int> sp2(sp1);
    EXPECT_EQ(sp1.get(), sp2.get());
    EXPECT_EQ(sp1.use_count(), 2);
    EXPECT_EQ(sp2.use_count(), 2);
    EXPECT_FALSE(sp1.unique());
}

TEST(SharedPtrTest, CopyAssign) {
    shared_ptr<int> sp1(new int(5));
    shared_ptr<int> sp2;
    sp2 = sp1;
    EXPECT_EQ(sp1.get(), sp2.get());
    EXPECT_EQ(sp1.use_count(), 2);
    sp1 = sp1;
    EXPECT_EQ(sp1.use_count(), 2);
}

TEST(SharedPtrTest, MoveConstruct) {
    shared_ptr<int> sp1(new int(7));
    int* raw = sp1.get();
    shared_ptr<int> sp2(move(sp1));
    EXPECT_FALSE(sp1);
    EXPECT_EQ(sp2.get(), raw);
    EXPECT_EQ(sp2.use_count(), 1);
}

TEST(SharedPtrTest, MoveAssign) {
    shared_ptr<int> sp1(new int(7));
    shared_ptr<int> sp2(new int(8));
    int* raw = sp1.get();
    sp2 = move(sp1);
    EXPECT_FALSE(sp1);
    EXPECT_EQ(sp2.get(), raw);
    EXPECT_EQ(sp2.use_count(), 1);
}

TEST(SharedPtrTest, MoveSelfAssign) {
    shared_ptr<int> sp(new int(9));
    sp = move(sp);
    EXPECT_TRUE(sp);
    EXPECT_EQ(*sp, 9);
}

TEST(SharedPtrTest, ConvertingCopyConstruct) {
    shared_ptr<derived> d(new derived(1, 2));
    shared_ptr<base> b(d);
    EXPECT_EQ(d.get(), b.get());
    EXPECT_EQ(d.use_count(), 2);
}

TEST(SharedPtrTest, ConvertingMoveConstruct) {
    shared_ptr<derived> d(new derived(1, 2));
    derived* raw = d.get();
    shared_ptr<base> b(move(d));
    EXPECT_FALSE(d);
    EXPECT_EQ(b.get(), raw);
    EXPECT_EQ(b.use_count(), 1);
}

TEST(SharedPtrTest, AliasConstructCopy) {
    auto d = make_shared<derived>(10, 20);
    int* alias_ptr = &d->derived_val;
    shared_ptr<int> alias(d, alias_ptr);
    EXPECT_EQ(alias.get(), alias_ptr);
    EXPECT_EQ(d.use_count(), 2);
    EXPECT_EQ(alias.use_count(), 2);
    EXPECT_EQ(*alias, 20);
}

TEST(SharedPtrTest, AliasConstructMove) {
    shared_ptr<derived> d(new derived(10, 20));
    derived* raw = d.get();
    int* alias_ptr = &d->derived_val;
    shared_ptr<int> alias(move(d), alias_ptr);
    EXPECT_FALSE(d);
    EXPECT_EQ(alias.get(), alias_ptr);
    EXPECT_EQ(alias.use_count(), 1);
}

TEST(SharedPtrTest, ResetNoArg) {
    shared_ptr<int> sp(new int(1));
    sp.reset();
    EXPECT_FALSE(sp);
    EXPECT_EQ(sp.use_count(), 0);
}

TEST(SharedPtrTest, ResetWithRaw) {
    shared_ptr<int> sp(new int(1));
    sp.reset(new int(2));
    EXPECT_EQ(*sp, 2);
    EXPECT_EQ(sp.use_count(), 1);
}

TEST(SharedPtrTest, ResetWithDeleter) {
    delete_counter = 0;
    shared_ptr<delete_counter_id> sp;
    sp.reset(new delete_counter_id(3), delete_delete_counter);
    EXPECT_TRUE(sp);
    sp.reset();
    EXPECT_EQ(delete_counter, 1);
}

TEST(SharedPtrTest, UseCountAndUnique) {
    shared_ptr<int> sp1;
    EXPECT_EQ(sp1.use_count(), 0);
    EXPECT_TRUE(sp1.unique());
    sp1.reset(new int(1));
    EXPECT_EQ(sp1.use_count(), 1);
    EXPECT_TRUE(sp1.unique());
    shared_ptr<int> sp2 = sp1;
    EXPECT_EQ(sp1.use_count(), 2);
    EXPECT_FALSE(sp1.unique());
}

TEST(SharedPtrTest, Swap) {
    shared_ptr<int> sp1(new int(1));
    shared_ptr<int> sp2(new int(2));
    int* p1 = sp1.get();
    int* p2 = sp2.get();
    sp1.swap(sp2);
    EXPECT_EQ(sp1.get(), p2);
    EXPECT_EQ(sp2.get(), p1);
    EXPECT_EQ(*sp1, 2);
    EXPECT_EQ(*sp2, 1);
    sp1.swap(sp1);
    EXPECT_EQ(sp1.get(), p2);
    EXPECT_EQ(*sp1, 2);
}

TEST(SharedPtrTest, Dereference) {
    shared_ptr<derived> sp(new derived(3, 4));
    EXPECT_EQ(sp.get()->value, 3);
    EXPECT_EQ(sp->derived_val, 4);
    EXPECT_EQ(sp->value, 3);
}

TEST(SharedPtrTest, BoolConversion) {
    shared_ptr<int> empty;
    shared_ptr<int> full(new int(0));
    if (empty) {
        FAIL() << "empty should be false";
    }
    if (!full) {
        FAIL() << "full should be true";
    }
}

TEST(SharedPtrTest, OwnerEqual) {
    shared_ptr<int> sp1(new int(1));
    shared_ptr<int> sp2(sp1);
    shared_ptr<int> sp3(new int(1));
    EXPECT_TRUE(sp1.owner_equal(sp2));
    EXPECT_FALSE(sp1.owner_equal(sp3));
    EXPECT_TRUE(sp1.owner_equal(sp1));
}

TEST(SharedPtrTest, OwnerBefore) {
    shared_ptr<int> sp1(new int(1));
    shared_ptr<int> sp2(new int(2));
    shared_ptr<int> sp3;
    bool b12 = sp1.owner_before(sp2);
    bool b21 = sp2.owner_before(sp1);
    EXPECT_NE(b12, b21);
    EXPECT_FALSE(sp1.owner_before(sp3));
    EXPECT_TRUE(sp3.owner_before(sp1));
}

TEST(SharedPtrTest, ComparisonOperators) {
    shared_ptr<int> sp1(new int(1));
    shared_ptr<int> sp2(sp1);
    shared_ptr<int> sp3(new int(1));
    shared_ptr<int> sp4;

    EXPECT_TRUE(sp1 == sp2);
    EXPECT_FALSE(sp1 != sp2);
    EXPECT_NE(sp1, sp3);
    EXPECT_FALSE(sp1 == sp3);
    EXPECT_FALSE(sp1 == sp4);
    EXPECT_NE(sp1, sp4);

    EXPECT_EQ(sp1 < sp2, false);
    EXPECT_EQ(sp2 < sp1, false);
    bool b13 = sp1 < sp3;
    bool b31 = sp3 < sp1;
    EXPECT_NE(b13, b31);
    EXPECT_LE(sp1, sp1);
    EXPECT_GE(sp1, sp1);
    EXPECT_FALSE(sp1 < sp4);
    EXPECT_TRUE(sp4 < sp1);
}

TEST(MakeSharedTest, SingleObject) {
    auto sp = make_shared<int>(42);
    EXPECT_EQ(*sp, 42);
    EXPECT_EQ(sp.use_count(), 1);
}

TEST(MakeSharedTest, MultipleArgs) {
    auto sp = make_shared<complex_type>(1, 3.14);
    EXPECT_EQ(sp->x, 1);
    EXPECT_DOUBLE_EQ(sp->y, 3.14);
}

TEST(MakeSharedTest, Array) {
    auto sp = make_shared<int[]>(5);
    for (size_t i = 0; i < 5; ++i) {
        sp[i] = static_cast<int>(i * i);
    }
    EXPECT_EQ(sp[0], 0);
    EXPECT_EQ(sp[4], 16);
}

TEST(AllocateSharedTest, Basic) {
    allocator<int> alloc;
    auto sp = allocate_shared<int>(alloc, 33);
    EXPECT_EQ(*sp, 33);
    EXPECT_EQ(sp.use_count(), 1);
}

TEST(EnableSharedFromThisTest, SharedFromThis) {
    auto sp = make_shared<enable_test>(55);
    EXPECT_EQ(sp.use_count(), 1);
    auto sp2 = sp->shared_from_this();
    EXPECT_EQ(sp2.get(), sp.get());
    EXPECT_EQ(sp.use_count(), 2);
}

TEST(EnableSharedFromThisTest, ConstSharedFromThis) {
    auto sp = make_shared<enable_test>(77);
    const auto& csp = sp;
    auto sp2 = csp->shared_from_this();
    EXPECT_EQ(sp2.get(), sp.get());
    EXPECT_EQ(sp.use_count(), 2);
}

TEST(EnableSharedFromThisTest, ThrowIfNotOwned) {
    enable_test et(99);
    EXPECT_THROW(et.shared_from_this(), memory_exception);
    EXPECT_THROW(static_cast<const enable_test&>(et).shared_from_this(), memory_exception);
}

TEST(PointerCastTest, StaticSharedCast) {
    shared_ptr<derived> d(new derived(1, 2));
    shared_ptr<base> b = static_pointer_cast<base>(d);
    EXPECT_EQ(b.get(), static_cast<base*>(d.get()));
    EXPECT_EQ(d.use_count(), 2);
}

TEST(PointerCastTest, ConstSharedCast) {
    shared_ptr<const int> ci(new int(5));
    shared_ptr<int> i = const_pointer_cast<int>(ci);
    EXPECT_EQ(i.get(), ci.get());
    *i = 6;
    EXPECT_EQ(*ci, 6);
}

TEST(PointerCastTest, ReinterpretSharedCast) {
    shared_ptr<int> i(new int(0x41424344));
    shared_ptr<char> c = reinterpret_pointer_cast<char>(i);
    EXPECT_EQ(c.get(), reinterpret_cast<char*>(i.get()));
    EXPECT_EQ(i.use_count(), 2);
}

TEST(PointerCastTest, DynamicSharedCastSuccess) {
    shared_ptr<base> b(new derived(10, 20));
    auto d = dynamic_pointer_cast<derived>(b);
    EXPECT_TRUE(d);
    EXPECT_EQ(d->derived_val, 20);
    EXPECT_EQ(b.use_count(), 2);
}

TEST(PointerCastTest, DynamicSharedCastFailure) {
    shared_ptr<base> b(new base(10));
    auto d = dynamic_pointer_cast<derived>(b);
    EXPECT_FALSE(d);
    EXPECT_EQ(b.use_count(), 1);
}

TEST(TypeTraitsTest, IsSharedPtr) {
    EXPECT_TRUE((is_shared_ptr<shared_ptr<int>>::value));
    EXPECT_FALSE((is_shared_ptr<int*>::value));
    EXPECT_FALSE((is_shared_ptr<unique_ptr<int>>::value));
}

TEST(HashTest, SharedPtrHash) {
    auto sp = make_shared<int>(42);
    size_t h = hash<shared_ptr<int>>()(sp);
    size_t expected = hash<int*>()(sp.get());
    EXPECT_EQ(h, expected);
}

TEST(AtomicSharedPtrTest, ConstructAndLoad) {
    auto sp = make_shared<int>(100);
    atomic<shared_ptr<int>> atom(sp);
    auto loaded = atom.load();
    EXPECT_EQ(loaded.get(), sp.get());
    EXPECT_EQ(loaded.use_count(), 3);
}

TEST(AtomicSharedPtrTest, Store) {
    auto sp1 = make_shared<int>(10);
    auto sp2 = make_shared<int>(20);
    atomic<shared_ptr<int>> atom(sp1);
    atom.store(sp2);
    auto loaded = atom.load();
    EXPECT_EQ(*loaded, 20);
    EXPECT_EQ(loaded.use_count(), 3);
}

TEST(AtomicSharedPtrTest, Exchange) {
    auto sp1 = make_shared<int>(5);
    auto sp2 = make_shared<int>(8);
    atomic<shared_ptr<int>> atom(sp1);
    auto old = atom.exchange(sp2);
    EXPECT_EQ(*old, 5);
    auto current = atom.load();
    EXPECT_EQ(*current, 8);
}

TEST(AtomicSharedPtrTest, CompareExchangeStrong) {
    auto sp1 = make_shared<int>(1);
    auto sp2 = make_shared<int>(2);
    atomic<shared_ptr<int>> atom(sp1);
    shared_ptr<int> expected = sp1;
    bool result = atom.compare_exchange_strong(expected, sp2);
    EXPECT_TRUE(result);
    EXPECT_EQ(*atom.load(), 2);
    EXPECT_EQ(*expected, 1);
}

// 通过测试，但含有内存泄漏
TEST(AtomicSharedPtrTest, DISABLED_CompareExchangeStrongFail) {
    auto sp1 = make_shared<int>(1);
    auto sp2 = make_shared<int>(2);
    atomic<shared_ptr<int>> atom(sp1);
    shared_ptr<int> expected = make_shared<int>(999);
    bool result = atom.compare_exchange_strong(expected, sp2);
    EXPECT_FALSE(result);
    EXPECT_EQ(*atom.load(), 1);
    EXPECT_EQ(expected.get(), sp1.get());
}

TEST(SharedPtrArrayTest, Subscript) {
    auto sp = make_shared<int[]>(3);
    sp[0] = 10;
    sp[1] = 20;
    sp[2] = 30;
    EXPECT_EQ(sp[0], 10);
    EXPECT_EQ(sp[1], 20);
    EXPECT_EQ(sp[2], 30);
}

TEST(WeakPtrTest, DefaultConstruction) {
    weak_ptr<int> wp;
    EXPECT_EQ(wp.use_count(), 0);
    EXPECT_TRUE(wp.expired());
}

TEST(WeakPtrTest, NullptrConstruction) {
    weak_ptr<int> wp(nullptr);
    EXPECT_EQ(wp.use_count(), 0);
    EXPECT_TRUE(wp.expired());
}

TEST(WeakPtrTest, ConstructFromSharedPtr) {
    auto sp = make_shared<int>(42);
    weak_ptr<int> wp(sp);
    EXPECT_EQ(wp.use_count(), 1);
    EXPECT_FALSE(wp.expired());
}

TEST(WeakPtrTest, ConstructFromEmptySharedPtr) {
    shared_ptr<int> sp;
    weak_ptr<int> wp(sp);
    EXPECT_EQ(wp.use_count(), 0);
    EXPECT_TRUE(wp.expired());
}

TEST(WeakPtrTest, CopyConstructor) {
    auto sp = make_shared<int>(10);
    weak_ptr<int> wp1(sp);
    weak_ptr<int> wp2(wp1);
    EXPECT_EQ(wp2.use_count(), 1);
    EXPECT_FALSE(wp2.expired());
}

TEST(WeakPtrTest, CopyConstructorEmpty) {
    weak_ptr<int> wp1;
    weak_ptr<int> wp2(wp1);
    EXPECT_EQ(wp2.use_count(), 0);
    EXPECT_TRUE(wp2.expired());
}

TEST(WeakPtrTest, ConvertingCopyConstructor) {
    auto sp = make_shared<weak_test_derived>(1, 2);
    weak_ptr<weak_test_derived> wpd(sp);
    weak_ptr<weak_test_base> wpb(wpd);
    EXPECT_EQ(wpb.use_count(), 1);
    EXPECT_FALSE(wpb.expired());
    EXPECT_FALSE(wpd.expired());
}

TEST(WeakPtrTest, MoveConstructor) {
    auto sp = make_shared<int>(5);
    weak_ptr<int> wp1(sp);
    weak_ptr<int> wp2(move(wp1));
    EXPECT_EQ(wp2.use_count(), 1);
    EXPECT_EQ(wp1.use_count(), 0);
    EXPECT_TRUE(wp1.expired());
}

TEST(WeakPtrTest, ConvertingMoveConstructor) {
    auto sp = make_shared<weak_test_derived>(3, 4);
    weak_ptr<weak_test_derived> wpd(sp);
    weak_ptr<weak_test_base> wpb(move(wpd));
    EXPECT_EQ(wpb.use_count(), 1);
    EXPECT_EQ(wpd.use_count(), 0);
}

TEST(WeakPtrTest, DestructorDoesNotAffectStrongCount) {
    auto sp = make_shared<int>(7);
    {
        weak_ptr<int> wp(sp);
        EXPECT_EQ(sp.use_count(), 1);
    }
    EXPECT_EQ(sp.use_count(), 1);
    EXPECT_FALSE(sp.unique() ? false : false);
}

TEST(WeakPtrTest, CopyAssignment) {
    auto sp = make_shared<int>(9);
    weak_ptr<int> wp1(sp);
    weak_ptr<int> wp2;
    wp2 = wp1;
    EXPECT_EQ(wp2.use_count(), 1);
    EXPECT_FALSE(wp2.expired());
    wp2 = wp2;
    EXPECT_EQ(wp2.use_count(), 1);
}

TEST(WeakPtrTest, CopyAssignmentFromEmpty) {
    weak_ptr<int> wp1;
    weak_ptr<int> wp2 = wp1;
    EXPECT_TRUE(wp2.expired());
}

TEST(WeakPtrTest, ConvertingCopyAssignment) {
    auto sp = make_shared<weak_test_derived>(5, 6);
    weak_ptr<weak_test_derived> wpd(sp);
    weak_ptr<weak_test_base> wpb;
    wpb = wpd;
    EXPECT_EQ(wpb.use_count(), 1);
    EXPECT_FALSE(wpb.expired());
}

TEST(WeakPtrTest, AssignmentFromSharedPtr) {
    auto sp = make_shared<int>(11);
    weak_ptr<int> wp;
    wp = sp;
    EXPECT_EQ(wp.use_count(), 1);
    EXPECT_FALSE(wp.expired());
    wp = shared_ptr<int>();
    EXPECT_TRUE(wp.expired());
}

TEST(WeakPtrTest, AssignmentFromSharedPtrDerived) {
    auto sp = make_shared<weak_test_derived>(7, 8);
    weak_ptr<weak_test_base> wp;
    wp = sp;
    EXPECT_EQ(wp.use_count(), 1);
    EXPECT_FALSE(wp.expired());
}

TEST(WeakPtrTest, MoveAssignment) {
    auto sp = make_shared<int>(13);
    weak_ptr<int> wp1(sp);
    weak_ptr<int> wp2;
    wp2 = move(wp1);
    EXPECT_EQ(wp2.use_count(), 1);
    EXPECT_TRUE(wp1.expired());
    wp2 = move(wp2);
    EXPECT_EQ(wp2.use_count(), 1);
}

TEST(WeakPtrTest, ConvertingMoveAssignment) {
    auto sp = make_shared<weak_test_derived>(9, 10);
    weak_ptr<weak_test_derived> wpd(sp);
    weak_ptr<weak_test_base> wpb;
    wpb = move(wpd);
    EXPECT_EQ(wpb.use_count(), 1);
    EXPECT_TRUE(wpd.expired());
}

TEST(WeakPtrTest, Reset) {
    auto sp = make_shared<int>(15);
    weak_ptr<int> wp(sp);
    wp.reset();
    EXPECT_TRUE(wp.expired());
    EXPECT_EQ(wp.use_count(), 0);
}

TEST(WeakPtrTest, Swap) {
    auto sp1 = make_shared<int>(1);
    auto sp2 = make_shared<int>(2);
    weak_ptr<int> wp1(sp1);
    weak_ptr<int> wp2(sp2);
    wp1.swap(wp2);
    EXPECT_EQ(wp1.lock(), sp2);
    EXPECT_EQ(wp2.lock(), sp1);
    wp1.swap(wp1);
    EXPECT_EQ(wp1.lock(), sp2);
}

TEST(WeakPtrTest, UseCountAndExpired) {
    auto sp = make_shared<int>(17);
    weak_ptr<int> wp(sp);
    EXPECT_EQ(wp.use_count(), 1);
    EXPECT_FALSE(wp.expired());
    sp.reset();
    EXPECT_EQ(wp.use_count(), 0);
    EXPECT_TRUE(wp.expired());
}

TEST(WeakPtrTest, LockWhileAlive) {
    auto sp = make_shared<int>(19);
    weak_ptr<int> wp(sp);
    auto locked = wp.lock();
    EXPECT_TRUE(locked);
    EXPECT_EQ(locked.get(), sp.get());
    EXPECT_EQ(sp.use_count(), 2);
}

TEST(WeakPtrTest, LockWhenExpired) {
    weak_ptr<int> wp;
    {
        auto sp = make_shared<int>(21);
        wp = sp;
    }
    auto locked = wp.lock();
    EXPECT_FALSE(locked);
    EXPECT_EQ(locked.use_count(), 0);
}

TEST(WeakPtrTest, OwnerEqualWithWeakPtr) {
    auto sp1 = make_shared<int>(23);
    auto sp2 = sp1;
    weak_ptr<int> wp1(sp1);
    weak_ptr<int> wp2(sp2);
    EXPECT_TRUE(wp1.owner_equal(wp2));
    weak_ptr<int> wp3;
    EXPECT_FALSE(wp1.owner_equal(wp3));
}

TEST(WeakPtrTest, OwnerEqualWithSharedPtr) {
    auto sp = make_shared<int>(25);
    weak_ptr<int> wp(sp);
    EXPECT_TRUE(wp.owner_equal(sp));
    EXPECT_FALSE(wp.owner_equal(shared_ptr<int>()));
}

TEST(WeakPtrTest, OwnerBeforeWeakPtr) {
    auto sp1 = make_shared<int>(27);
    auto sp2 = make_shared<int>(28);
    weak_ptr<int> wp1(sp1);
    weak_ptr<int> wp2(sp2);
    weak_ptr<int> wp3;
    bool b12 = wp1.owner_before(wp2);
    bool b21 = wp2.owner_before(wp1);
    EXPECT_NE(b12, b21);
    EXPECT_FALSE(wp1.owner_before(wp3));
    EXPECT_TRUE(wp3.owner_before(wp1));
}

TEST(WeakPtrTest, OwnerBeforeSharedPtr) {
    auto sp = make_shared<int>(29);
    weak_ptr<int> wp(sp);
    weak_ptr<int> wp2;
    EXPECT_EQ(wp.owner_before(sp), false);
    EXPECT_TRUE(wp2.owner_before(sp));
    EXPECT_FALSE(wp.owner_before(shared_ptr<int>()));
}

TEST(OwnerLessTest, SharedPtrSpecialization) {
    owner_less_test t;
    owner_less<shared_ptr<int>> cmp;
    EXPECT_FALSE(cmp(t.sp1, t.sp1));
    EXPECT_FALSE(cmp(t.sp1, t.sp3));
    EXPECT_NE(cmp(t.sp1, t.sp2), cmp(t.sp2, t.sp1));
    EXPECT_TRUE(cmp(shared_ptr<int>(), t.sp1));
}

TEST(OwnerLessTest, WeakPtrSpecialization) {
    owner_less_test t;
    owner_less<weak_ptr<int>> cmp;
    EXPECT_FALSE(cmp(t.wp1, t.wp1));
    EXPECT_FALSE(cmp(t.wp1, t.wp3));
    EXPECT_NE(cmp(t.wp1, t.wp2), cmp(t.wp2, t.wp1));
    EXPECT_TRUE(cmp(weak_ptr<int>(), t.wp1));
}

TEST(OwnerLessTest, VoidSpecializationMixedTypes) {
    owner_less_test t;
    owner_less<void> cmp;
    EXPECT_FALSE(cmp(t.sp1, t.wp1));
    EXPECT_FALSE(cmp(t.wp1, t.sp1));
    EXPECT_NE(cmp(t.sp1, t.wp2), cmp(t.sp2, t.wp1));
    EXPECT_TRUE(cmp(shared_ptr<int>(), t.wp1));
    EXPECT_TRUE(cmp(weak_ptr<int>(), t.sp1));
}

TEST(AtomicWeakPtrTest, DefaultConstructAndLoad) {
    atomic<weak_ptr<int>> atom;
    weak_ptr<int> loaded = atom.load();
    EXPECT_TRUE(loaded.expired());
}

TEST(AtomicWeakPtrTest, ConstructFromWeakPtr) {
    auto sp = make_shared<int>(100);
    weak_ptr<int> wp(sp);
    atomic<weak_ptr<int>> atom(wp);
    auto loaded = atom.load();
    EXPECT_FALSE(loaded.expired());
    EXPECT_EQ(loaded.lock(), sp);
}

TEST(AtomicWeakPtrTest, Store) {
    auto sp1 = make_shared<int>(10);
    auto sp2 = make_shared<int>(20);
    weak_ptr<int> wp1(sp1), wp2(sp2);
    atomic<weak_ptr<int>> atom(wp1);
    atom.store(wp2);
    auto loaded = atom.load();
    EXPECT_EQ(loaded.lock(), sp2);
}

TEST(AtomicWeakPtrTest, Exchange) {
    auto sp1 = make_shared<int>(5);
    auto sp2 = make_shared<int>(8);
    weak_ptr<int> wp1(sp1), wp2(sp2);
    atomic<weak_ptr<int>> atom(wp1);
    auto old = atom.exchange(wp2);
    EXPECT_EQ(old.lock(), sp1);
    auto current = atom.load();
    EXPECT_EQ(current.lock(), sp2);
}

TEST(AtomicWeakPtrTest, CompareExchangeStrongSuccess) {
    auto sp1 = make_shared<int>(1);
    auto sp2 = make_shared<int>(2);
    weak_ptr<int> wp1(sp1), wp2(sp2);
    atomic<weak_ptr<int>> atom(wp1);
    weak_ptr<int> expected = wp1;
    bool result = atom.compare_exchange_strong(expected, wp2);
    EXPECT_TRUE(result);
    EXPECT_EQ(atom.load().lock(), sp2);
    EXPECT_FALSE(expected.expired());
}

TEST(AtomicWeakPtrTest, CompareExchangeStrongFailure) {
    auto sp1 = make_shared<int>(1);
    auto sp2 = make_shared<int>(2);
    weak_ptr<int> wp1(sp1), wp2(sp2);
    atomic<weak_ptr<int>> atom(wp1);
    weak_ptr<int> expected;
    bool result = atom.compare_exchange_strong(expected, wp2);
    EXPECT_FALSE(result);
    EXPECT_EQ(atom.load().lock(), sp1);
    EXPECT_EQ(expected.lock(), sp1);
}

TEST(AtomicWeakPtrTest, IsLockFree) {
    atomic<weak_ptr<int>> atom;
    EXPECT_FALSE(atom.is_lock_free());
}

TEST(AtomicWeakPtrTest, ImplicitConversionOperator) {
    auto sp = make_shared<int>(42);
    atomic<weak_ptr<int>> atom{weak_ptr<int>(sp)};
    weak_ptr<int> wp = atom;
    EXPECT_EQ(wp.lock(), sp);
}

TEST(AllocatedPtrTest, ConstructFromAllocAndPointer) {
    tracking_allocator<int> alloc;
    int* raw = alloc.allocate(1);
    {
        allocated_ptr<tracking_allocator<int>> guard(alloc, raw);
        EXPECT_EQ(guard.get(), raw);
        EXPECT_EQ(alloc.deallocate_count, 0);
    }
    EXPECT_EQ(alloc.deallocate_count, 1);
    EXPECT_EQ(alloc.last_deallocated, raw);
}

TEST(AllocatedPtrTest, ConstructFromRawPtr) {
    tracking_allocator<int> alloc;
    int* raw = alloc.allocate(1);
    {
        allocated_ptr<tracking_allocator<int>> guard(alloc, raw);
        EXPECT_EQ(guard.get(), raw);
    }
    EXPECT_EQ(alloc.deallocate_count, 1);
}

TEST(AllocatedPtrTest, MoveConstructor) {
    tracking_allocator<int> alloc;
    int* raw = alloc.allocate(1);
    {
        allocated_ptr<tracking_allocator<int>> guard1(alloc, raw);
        allocated_ptr<tracking_allocator<int>> guard2(move(guard1));
        EXPECT_EQ(guard2.get(), raw);
        EXPECT_EQ(guard1.get(), nullptr);
    }
    EXPECT_EQ(alloc.deallocate_count, 1);
    EXPECT_EQ(alloc.last_deallocated, raw);
}

TEST(AllocatedPtrTest, DestructorDoesNothingWhenNull) {
    tracking_allocator<int> alloc;
    int* raw = alloc.allocate(1);
    {
        allocated_ptr<tracking_allocator<int>> guard(alloc, raw);
        guard = nullptr;
        EXPECT_EQ(guard.get(), nullptr);
    }
    EXPECT_EQ(alloc.deallocate_count, 0);
    ::operator delete(raw);
}

TEST(AllocatedPtrTest, AssignNullptr) {
    tracking_allocator<int> alloc;
    int* raw = alloc.allocate(1);
    allocated_ptr<tracking_allocator<int>> guard(alloc, raw);
    guard = nullptr;
    EXPECT_EQ(guard.get(), nullptr);
    EXPECT_EQ(alloc.deallocate_count, 0);
    ::operator delete(raw);
}

// 通过测试，但含有内存泄漏
TEST(AllocatedPtrTest, DISABLED_Get) {
    tracking_allocator<int> alloc;
    int* raw = alloc.allocate(1);
    allocated_ptr<tracking_allocator<int>> guard(alloc, raw);
    EXPECT_EQ(guard.get(), raw);
    guard = nullptr;
    EXPECT_EQ(guard.get(), nullptr);
}

TEST(AllocatedPtrTest, AllocateGuarded) {
    tracking_allocator<int> alloc;
    {
        auto guard = allocate_guarded(alloc);
        EXPECT_EQ(alloc.allocate_count, 1);
        EXPECT_NE(guard.get(), nullptr);
        EXPECT_EQ(alloc.deallocate_count, 0);
    }
    EXPECT_EQ(alloc.deallocate_count, 1);
    EXPECT_EQ(alloc.last_deallocated, alloc.last_allocated);
}

TEST(AllocatedPtrTest, WithStdAllocator) {
    allocator<int> alloc;
    int* raw = alloc.allocate(1);
    {
        allocated_ptr<allocator<int>> guard(alloc, raw);
        EXPECT_EQ(guard.get(), raw);
    }
}

TEST(MemoryViewTest, TypeDefinitions) {
    using view_t = memory_view<int>;
    EXPECT_TRUE((is_same_v<view_t::value_type, int>) );
    EXPECT_TRUE((is_same_v<view_t::size_type, size_t>) );
    EXPECT_TRUE((is_same_v<view_t::difference_type, ptrdiff_t>) );
    EXPECT_TRUE((is_same_v<view_t::pointer, int*>) );
    EXPECT_TRUE((is_same_v<view_t::const_pointer, const int*>) );
    EXPECT_TRUE((is_same_v<view_t::reference, int&>) );
    EXPECT_TRUE((is_same_v<view_t::const_reference, const int&>) );
}

TEST(MemoryViewTest, DefaultConstructor) {
    memory_view<int> dv;
    EXPECT_EQ(dv.size(), 0);
    EXPECT_TRUE(dv.empty());

    memory_view<int, 0> sv;
    EXPECT_EQ(sv.size(), 0);
    EXPECT_TRUE(sv.empty());
}

TEST(MemoryViewTest, ConstructFromPointerAndCountDynamic) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    memory_view<int> view(arr.data(), 3);
    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view[0], 1);
    EXPECT_EQ(view[2], 3);
}

TEST(MemoryViewTest, ConstructFromPointerAndCountStatic) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    memory_view<int, 5> view(arr.data(), 5);
    EXPECT_EQ(view.size(), 5);
    EXPECT_EQ(view[4], 5);
}

TEST(MemoryViewTest, ConstructFromIteratorRange) {
    array<int, 5> arr = {10, 20, 30, 40, 50};
    memory_view<int> view(arr.data(), arr.data() + 5);
    EXPECT_EQ(view.size(), 5);
    EXPECT_EQ(view.front(), 10);
    EXPECT_EQ(view.back(), 50);
}

TEST(MemoryViewTest, ConstructFromEmptyIteratorRange) {
    int* p = nullptr;
    memory_view<int> view(p, p);
    EXPECT_EQ(view.size(), 0);
    EXPECT_TRUE(view.empty());
}

TEST(MemoryViewTest, ConstructFromCArray) {
    int arr[] = {5, 6, 7};
    memory_view<int> dyn_view(arr);
    EXPECT_EQ(dyn_view.size(), 3);
    memory_view<int, 3> stat_view(arr);
    EXPECT_EQ(stat_view.size(), 3);
}

TEST(MemoryViewTest, ConstructFromConstCArray) {
    const int arr[] = {8, 9, 10};
    memory_view<const int> dyn_view(arr);
    EXPECT_EQ(dyn_view.size(), 3);
    EXPECT_EQ(dyn_view[1], 9);
}

TEST(MemoryViewTest, ConstructFromArray) {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    memory_view<int> dyn_view(arr);
    EXPECT_EQ(dyn_view.size(), 5);
    memory_view<int, 5> stat_view(arr);
    EXPECT_EQ(stat_view.size(), 5);
}

TEST(MemoryViewTest, ConstructFromConstArray) {
    const array<int, 5> arr = {11, 22, 33, 44, 55};
    memory_view<const int> dyn_view(arr);
    EXPECT_EQ(dyn_view.size(), 5);
    memory_view<const int, 5> stat_view(arr);
    EXPECT_EQ(stat_view.size(), 5);
}

TEST(MemoryViewTest, CopyConstructor) {
    array<int, 5> arr = {0, 1, 2, 3, 4};
    memory_view<int> view1(arr);
    memory_view<int> view2(view1);
    EXPECT_EQ(view2.data(), view1.data());
    EXPECT_EQ(view2.size(), view1.size());
}

TEST(MemoryViewTest, ConvertingConstructorIntToConstInt) {
    array<int, 5> arr = {7, 8, 9, 10, 11};
    memory_view<int> iv(arr);
    memory_view<const int> cv(iv);
    EXPECT_EQ(cv.data(), iv.data());
    EXPECT_EQ(cv.size(), 5);
}

TEST(MemoryViewTest, ConvertingConstructorStaticToDynamic) {
    int arr[] = {1, 2, 3};
    memory_view<int, 3> sv(arr);
    memory_view<int> dv(sv);
    EXPECT_EQ(dv.size(), 3);
    EXPECT_EQ(dv[1], 2);
}

TEST(MemoryViewTest, ConvertingConstructorDynamicToStatic) {
    int arr[] = {10, 20, 30, 40, 50};
    memory_view<int> dv(arr, 5);
    memory_view<int, 5> sv(dv);
    EXPECT_EQ(sv.size(), 5);
    EXPECT_EQ(sv[4], 50);
}

TEST(MemoryViewTest, CopyAssignment) {
    array<int, 5> arr1 = {1, 2, 3, 4, 5};
    array<int, 5> arr2 = {6, 7, 8, 9, 10};
    memory_view<int> v1(arr1);
    memory_view<int> v2(arr2);
    v2 = v1;
    EXPECT_EQ(v2.data(), arr1.data());
    EXPECT_EQ(v2.size(), v1.size());
}

TEST(MemoryViewTest, SizeAndEmpty) {
    int arr[] = {0, 1, 2};
    memory_view<int> view(arr, 3);
    EXPECT_EQ(view.size(), 3);
    EXPECT_FALSE(view.empty());

    memory_view<int> empty_view;
    EXPECT_EQ(empty_view.size(), 0);
    EXPECT_TRUE(empty_view.empty());
}

TEST(MemoryViewTest, SizeBytes) {
    int arr[4];
    memory_view<int> view(arr, 4);
    EXPECT_EQ(view.size_bytes(), 4 * sizeof(int));
}

TEST(MemoryViewTest, FrontAndBack) {
    int arr[] = {100, 200, 300};
    memory_view<int> view(arr);
    EXPECT_EQ(view.front(), 100);
    EXPECT_EQ(view.back(), 300);

    int& front_ref = view.front();
    front_ref = 111;
    EXPECT_EQ(arr[0], 111);

    view.back() = 333;
    EXPECT_EQ(arr[2], 333);
}

TEST(MemoryViewTest, SubscriptOperator) {
    int arr[] = {5, 10, 15, 20};
    memory_view<int> view(arr);
    EXPECT_EQ(view[0], 5);
    EXPECT_EQ(view[3], 20);
    view[1] = 99;
    EXPECT_EQ(arr[1], 99);
}

TEST(MemoryViewTest, Data) {
    int arr[] = {7, 8, 9};
    memory_view<int> view(arr);
    EXPECT_EQ(view.data(), arr);
    *view.data() = 77;
    EXPECT_EQ(arr[0], 77);
}

TEST(MemoryViewTest, IteratorsBeginEnd) {
    int arr[] = {1, 2, 3, 4, 5};
    memory_view<int> view(arr);
    auto it = view.begin();
    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, 1);
    ++it;
    ++it;
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(view.end() - view.begin(), 5);
}

TEST(MemoryViewTest, ReverseIterators) {
    int arr[] = {10, 20, 30};
    memory_view<int> view(arr);
    auto rit = view.rbegin();
    EXPECT_EQ(*rit, 30);
    ++rit;
    EXPECT_EQ(*rit, 20);
    EXPECT_EQ(rit.base(), view.end() - 1);
    auto rend_it = view.rend();
    EXPECT_TRUE(rend_it == view.rbegin() + 3);
}

TEST(MemoryViewTest, FirstStaticSize) {
    int arr[] = {1, 2, 3, 4, 5};
    memory_view<int> view(arr, 5);
    auto sub = view.first<3>();
    EXPECT_EQ(sub.size(), 3);
    EXPECT_EQ(sub[0], 1);
    EXPECT_EQ(sub[2], 3);
    EXPECT_TRUE((is_same_v<decltype(sub), memory_view<int, 3>>) );
}

TEST(MemoryViewTest, FirstDynamicCount) {
    int arr[] = {10, 20, 30, 40};
    memory_view<int> view(arr, 4);
    auto sub = view.first(2);
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[1], 20);
}

TEST(MemoryViewTest, LastStaticSize) {
    int arr[] = {5, 6, 7, 8, 9};
    memory_view<int> view(arr, 5);
    auto sub = view.last<3>();
    EXPECT_EQ(sub.size(), 3);
    EXPECT_EQ(sub[0], 7);
    EXPECT_EQ(sub[2], 9);
}

TEST(MemoryViewTest, LastDynamicCount) {
    int arr[] = {100, 200, 300};
    memory_view<int> view(arr);
    auto sub = view.last(2);
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[0], 200);
    EXPECT_EQ(sub[1], 300);
}

TEST(MemoryViewTest, ViewStaticOffsetAndCount) {
    int arr[] = {0, 1, 2, 3, 4, 5, 6};
    memory_view<int> view(arr, 7);
    auto sub = view.view<2, 3>();
    EXPECT_EQ(sub.size(), 3);
    EXPECT_EQ(sub[0], 2);
    EXPECT_EQ(sub[1], 3);
    EXPECT_EQ(sub[2], 4);
}

TEST(MemoryViewTest, ViewStaticOffsetDynamicCount) {
    int arr[] = {0, 1, 2, 3, 4};
    memory_view<int> view(arr, 5);
    auto sub = view.view<2>();
    EXPECT_EQ(sub.size(), 3);
    EXPECT_EQ(sub[0], 2);
    EXPECT_EQ(sub[2], 4);
}

TEST(MemoryViewTest, ViewDynamicOffsetAndCount) {
    int arr[] = {10, 20, 30, 40, 50};
    memory_view<int> view(arr);
    auto sub = view.view(1, 3);
    EXPECT_EQ(sub.size(), 3);
    EXPECT_EQ(sub[0], 20);
    EXPECT_EQ(sub[2], 40);
}

TEST(MemoryViewTest, ViewDynamicOffsetRest) {
    int arr[] = {1, 2, 3, 4};
    memory_view<int> view(arr);
    auto sub = view.view(2);
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[0], 3);
    EXPECT_EQ(sub[1], 4);
}

TEST(MemoryViewTest, ConstViewReading) {
    const int arr[] = {55, 66, 77};
    memory_view<const int> view(arr);
    EXPECT_EQ(view.front(), 55);
    EXPECT_EQ(view[2], 77);
}

TEST(MemoryViewTest, ByteView) {
    int arr[] = {0x01020304};
    byte_view bv = memory_view<byte_t>(reinterpret_cast<byte_t*>(arr), sizeof(arr));
    EXPECT_EQ(bv.size(), sizeof(int));
    (void) bv[0];
}

TEST(MemoryViewTest, ConstByteView) {
    const int x = 42;
    cbyte_view cbv(reinterpret_cast<const byte_t*>(&x), sizeof(x));
    EXPECT_EQ(cbv.size(), sizeof(int));
    EXPECT_EQ(cbv.data(), reinterpret_cast<const byte_t*>(&x));
}

TEST(MemoryViewTest, UserDefinedIteration) {
    int arr[] = {3, 1, 4, 1, 5};
    memory_view<int> view(arr);
    int sum = 0;
    for (const int& x: view) {
        sum += x;
    }
    EXPECT_EQ(sum, 14);
}

TEST(MemoryViewTest, RangeForWithConstView) {
    const int arr[] = {2, 3, 4};
    memory_view<const int> view(arr);
    int prod = 1;
    for (const auto& x: view) {
        prod *= x;
    }
    EXPECT_EQ(prod, 24);
}

TEST(MemoryViewTest, ReverseRangeFor) {
    int arr[] = {10, 20, 30};
    memory_view<int> view(arr);
    vector<int> reversed;
    for (auto it = view.rbegin(); it != view.rend(); ++it) {
        reversed.push_back(*it);
    }
    EXPECT_EQ(reversed, (vector<int>{30, 20, 10}));
}
