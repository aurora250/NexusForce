#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/utility/any.hpp>
#include <NeForce/core/utility/byte_size.hpp>
#include <NeForce/core/utility/color.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/optional.hpp>
#include <NeForce/core/utility/scope.hpp>
#include <NeForce/core/utility/tuple.hpp>
#include <NeForce/core/utility/uuid.hpp>
#include <NeForce/core/utility/variant.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using std::initializer_list;

namespace {
    struct small_type {
        int x;
        small_type(int val = 0) :
        x(val) {}
        bool operator==(const small_type& other) const { return x == other.x; }
    };

    struct large_type {
        double a, b, c, d, e, f, g, h;
        large_type(double v = 0.0) :
        a(v),
        b(v),
        c(v),
        d(v),
        e(v),
        f(v),
        g(v),
        h(v) {}
        bool operator==(const large_type& other) const { return a == other.a; }
    };

    struct non_copyable {
        int val;
        non_copyable(int v) :
        val(v) {}
        non_copyable(const non_copyable&) = delete;
        non_copyable(non_copyable&&) = default;
        non_copyable& operator=(const non_copyable&) = delete;
        non_copyable& operator=(non_copyable&&) = default;
    };

    bool color_equal(const color& c1, const color& c2) {
        return c1.R() == c2.R() && c1.G() == c2.G() && c1.B() == c2.B() && c1.A() == c2.A();
    }

    struct explicit_default {
        explicit explicit_default() = default;
    };
    struct implicit_default {
        implicit_default() = default;
    };

    struct implicit_from_int {
        int val;
        implicit_from_int(int x) :
        val(x) {}
    };
    struct explicit_from_int {
        int val;
        explicit explicit_from_int(int x) :
        val(x) {}
    };

    struct from_tuple {
        int a;
        double b;
        string c;
        from_tuple(int a_, double b_, string c_) :
        a(a_),
        b(b_),
        c(move(c_)) {}
    };

    struct no_default {
        no_default(int) {}
    };

    struct tracked {
        static int copies;
        static int moves;
        int data;
        tracked() :
        data(0) {}
        explicit tracked(int d) :
        data(d) {}
        tracked(const tracked& other) :
        data(other.data) {
            ++copies;
        }
        tracked(tracked&& other) noexcept :
        data(other.data) {
            other.data = 0;
            ++moves;
        }
        tracked& operator=(const tracked& other) {
            data = other.data;
            ++copies;
            return *this;
        }
        tracked& operator=(tracked&& other) noexcept {
            data = other.data;
            other.data = 0;
            ++moves;
            return *this;
        }
        bool operator==(const tracked& other) const { return data == other.data; }
        bool operator<(const tracked& other) const { return data < other.data; }
    };
    int tracked::copies = 0;
    int tracked::moves = 0;

    struct base {};
    struct derived : base {};

    int free_func(int a) { return a * 2; }
} // namespace

TEST(AnyTest, DefaultConstructor) {
    any a;
    EXPECT_FALSE(a.has_value());
}

TEST(AnyTest, ConstructorFromInt) {
    any a(42);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.type(), typeid(int));
    EXPECT_EQ(any_cast<int>(a), 42);
}

TEST(AnyTest, ConstructorFromString) {
    any a(string("hello"));
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.type(), typeid(string));
    EXPECT_EQ(any_cast<string>(a), "hello");
}

TEST(AnyTest, CopyConstructor) {
    any a(10);
    any b(a);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(any_cast<int>(b), 10);
    EXPECT_EQ(any_cast<int>(a), 10);
}

TEST(AnyTest, CopyConstructorEmpty) {
    any a;
    any b(a);
    EXPECT_FALSE(b.has_value());
}

TEST(AnyTest, CopyAssignment) {
    any a(3.14);
    any b;
    b = a;
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(any_cast<double>(b), 3.14);
    b = b;
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(any_cast<double>(b), 3.14);
}

TEST(AnyTest, MoveConstructor) {
    any a(100);
    any b(move(a));
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(any_cast<int>(b), 100);
    EXPECT_FALSE(a.has_value());
}

TEST(AnyTest, MoveAssignment) {
    any a(200);
    any b;
    b = move(a);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(any_cast<int>(b), 200);
    EXPECT_FALSE(a.has_value());
    b = move(b);
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(any_cast<int>(b), 200);
}

TEST(AnyTest, InPlaceConstruct) {
    any a = make_any<string>(string("world"));
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(any_cast<string>(a), "world");
}

TEST(AnyTest, InPlaceConstructMultipleArgs) {
    any a(pass_template_construct_tag<vector<int>>{}, vector<int>{1, 2, 3});
    EXPECT_TRUE(a.has_value());
    auto vec = any_cast<vector<int>>(a);
    EXPECT_EQ(vec, (vector<int>{1, 2, 3}));
}

TEST(AnyTest, InPlaceConstructWithInitializerList) {
    any a(pass_template_construct_tag<vector<int>>{}, initializer_list<int>{5, 6, 7});
    EXPECT_TRUE(a.has_value());
    auto vec = any_cast<vector<int>>(a);
    EXPECT_EQ(vec, (vector<int>{5, 6, 7}));
}

TEST(AnyTest, Emplace) {
    any a;
    auto& ref = a.emplace<small_type>(42);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.type(), typeid(small_type));
    EXPECT_EQ(ref.x, 42);
    int& int_ref = a.emplace<int>(100);
    EXPECT_EQ(int_ref, 100);
}

TEST(AnyTest, EmplaceWithInitializerList) {
    any a;
    auto& ref = a.emplace<vector<int>>(initializer_list<int>{8, 9});
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(ref, (vector<int>{8, 9}));
}

TEST(AnyTest, Reset) {
    any a(5);
    EXPECT_TRUE(a.has_value());
    a.reset();
    EXPECT_FALSE(a.has_value());
    a.reset();
    EXPECT_FALSE(a.has_value());
}

TEST(AnyTest, HasValue) {
    any a;
    EXPECT_FALSE(a.has_value());
    a = 10;
    EXPECT_TRUE(a.has_value());
    a.reset();
    EXPECT_FALSE(a.has_value());
}

TEST(AnyTest, Type) {
    any a(1.2f);
    EXPECT_EQ(a.type(), typeid(float));
    any b;
    EXPECT_EQ(b.type(), typeid(void));
}

TEST(AnyTest, Swap) {
    any a(1);
    any b(string("swap"));
    a.swap(b);
    EXPECT_EQ(any_cast<string>(a), "swap");
    EXPECT_EQ(any_cast<int>(b), 1);
    any empty;
    a.swap(empty);
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(any_cast<string>(empty), "swap");
    empty.swap(empty);
    EXPECT_TRUE(empty.has_value());
}

TEST(AnyTest, MakeAny) {
    auto a = make_any<small_type>(42);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(any_cast<small_type>(a).x, 42);
}

TEST(AnyTest, MakeAnyWithInitializerList) {
    auto a = make_any<vector<int>>(initializer_list<int>{10, 11});
    EXPECT_EQ(any_cast<vector<int>>(a), (vector<int>{10, 11}));
}

TEST(AnyTest, AnyCastPointer) {
    any a(3.14);
    const any* ca = &a;
    const double* d = any_cast<double>(ca);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(*d, 3.14);

    const int* i = any_cast<int>(ca);
    EXPECT_EQ(i, nullptr);
}

TEST(AnyTest, AnyCastNullptr) {
    any* p = nullptr;
    EXPECT_EQ(any_cast<int>(p), nullptr);
}

TEST(AnyTest, AnyCastReferenceSuccess) {
    any a(string("success"));
    string s = any_cast<string>(a);
    EXPECT_EQ(s, "success");
}

TEST(AnyTest, AnyCastReferenceWrongType) {
    any a(10);
    EXPECT_THROW(any_cast<string>(a), anycast_exception);
}

TEST(AnyTest, AnyCastReferenceEmpty) {
    any a;
    EXPECT_THROW(any_cast<int>(a), anycast_exception);
}

TEST(AnyTest, AnyCastNonCopyable) {
    any a(pass_template_construct_tag<double>{}, 5.0);
    double ref = any_cast<double>(a);
}

TEST(AnyTest, LargeObjectHeapStorage) {
    large_type big(1.0);
    any a(big);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(any_cast<large_type>(a).a, 1.0);
    any b(a);
    EXPECT_EQ(any_cast<large_type>(b).a, 1.0);
    any c(move(a));
    EXPECT_EQ(any_cast<large_type>(c).a, 1.0);
    EXPECT_FALSE(a.has_value());
}

TEST(AnyTest, SmallObjectInlineStorage) {
    small_type s(10);
    any a(s);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(any_cast<small_type>(a).x, 10);
    any b(a);
    EXPECT_EQ(any_cast<small_type>(b).x, 10);
}

TEST(AnyTest, AssignmentFromValue) {
    any a;
    a = 42;
    EXPECT_EQ(any_cast<int>(a), 42);
    a = string("assign");
    EXPECT_EQ(any_cast<string>(a), "assign");
}

TEST(AnyTest, ClearViaAssignEmpty) {
    any a(100);
    any b;
    a = b;
    EXPECT_FALSE(a.has_value());
}

TEST(ColorTest, DefaultConstructor) {
    color c;
    EXPECT_EQ(c.R(), 0);
    EXPECT_EQ(c.G(), 0);
    EXPECT_EQ(c.B(), 0);
    EXPECT_EQ(c.A(), 255);
    EXPECT_TRUE(c.is_opaque());
    EXPECT_FALSE(c.is_transparent());
}

TEST(ColorTest, GrayConstructor) {
    color c(128);
    EXPECT_EQ(c.R(), 128);
    EXPECT_EQ(c.G(), 128);
    EXPECT_EQ(c.B(), 128);
    EXPECT_EQ(c.A(), 255);
}

TEST(ColorTest, GrayWithAlphaConstructor) {
    color c(200, 100);
    EXPECT_EQ(c.R(), 200);
    EXPECT_EQ(c.G(), 200);
    EXPECT_EQ(c.B(), 200);
    EXPECT_EQ(c.A(), 100);
}

TEST(ColorTest, RGBConstructor) {
    color c(10, 20, 30);
    EXPECT_EQ(c.R(), 10);
    EXPECT_EQ(c.G(), 20);
    EXPECT_EQ(c.B(), 30);
    EXPECT_EQ(c.A(), 255);
}

TEST(ColorTest, RGBAConstructor) {
    color c(5, 15, 25, 200);
    EXPECT_EQ(c.R(), 5);
    EXPECT_EQ(c.G(), 15);
    EXPECT_EQ(c.B(), 25);
    EXPECT_EQ(c.A(), 200);
}

TEST(ColorTest, ClampingOnConstruction) {
    color c(-10, 300, 128, -5);
    EXPECT_EQ(c.R(), 0);
    EXPECT_EQ(c.G(), 255);
    EXPECT_EQ(c.B(), 128);
    EXPECT_EQ(c.A(), 0);
}

TEST(ColorTest, HexStringConstructor6Digit) {
    color c(string_view("FF8000"));
    EXPECT_EQ(c.R(), 255);
    EXPECT_EQ(c.G(), 128);
    EXPECT_EQ(c.B(), 0);
    EXPECT_EQ(c.A(), 255);
}

TEST(ColorTest, HexStringConstructor8Digit) {
    color c(string_view("102030CC"));
    EXPECT_EQ(c.R(), 16);
    EXPECT_EQ(c.G(), 32);
    EXPECT_EQ(c.B(), 48);
    EXPECT_EQ(c.A(), 204);
}

TEST(ColorTest, HexStringConstructorWithHash) {
    color c("#ABCDEF01");
    EXPECT_EQ(c.R(), 0xAB);
    EXPECT_EQ(c.G(), 0xCD);
    EXPECT_EQ(c.B(), 0xEF);
    EXPECT_EQ(c.A(), 0x01);
}

TEST(ColorTest, InvalidHexStringThrows) {
    EXPECT_THROW(color(string_view("ZZZ")), value_exception);
    EXPECT_THROW(color(string_view("12345")), value_exception);
}

TEST(ColorTest, CopyConstructor) {
    color c1(1, 2, 3, 4);
    color c2(c1);
    EXPECT_TRUE(color_equal(c2, c1));
}

TEST(ColorTest, MoveConstructor) {
    color c1(50, 60, 70, 80);
    color c2(move(c1));
    EXPECT_EQ(c2.R(), 50);
    EXPECT_EQ(c2.G(), 60);
    EXPECT_EQ(c2.B(), 70);
    EXPECT_EQ(c2.A(), 80);
    EXPECT_EQ(c1.R(), 0);
    EXPECT_EQ(c1.G(), 0);
    EXPECT_EQ(c1.B(), 0);
    EXPECT_EQ(c1.A(), 255);
}

TEST(ColorTest, MoveAssignment) {
    color c1(11, 22, 33, 44);
    color c2(99, 88, 77, 66);
    c2 = move(c1);
    EXPECT_EQ(c2.R(), 11);
    EXPECT_EQ(c2.A(), 44);
    c2 = move(c2);
    EXPECT_EQ(c2.R(), 11);
}

TEST(ColorTest, Accessors) {
    color c(10, 20, 30, 40);
    EXPECT_EQ(c.R(), 10);
    EXPECT_EQ(c.G(), 20);
    EXPECT_EQ(c.B(), 30);
    EXPECT_EQ(c.A(), 40);
}

TEST(ColorTest, SetR) {
    color c;
    c.setR(128);
    EXPECT_EQ(c.R(), 128);
    c.setR(300);
    EXPECT_EQ(c.R(), 255);
    c.setR(-1);
    EXPECT_EQ(c.R(), 0);
}

TEST(ColorTest, SetG) {
    color c;
    c.setG(90);
    EXPECT_EQ(c.G(), 90);
    c.setG(-10);
    EXPECT_EQ(c.G(), 0);
}

TEST(ColorTest, SetB) {
    color c;
    c.setB(200);
    EXPECT_EQ(c.B(), 200);
    c.setB(256);
    EXPECT_EQ(c.B(), 255);
}

TEST(ColorTest, SetA) {
    color c;
    c.setA(150);
    EXPECT_EQ(c.A(), 150);
    c.setA(255);
    EXPECT_EQ(c.A(), 255);
    c.setA(0);
    EXPECT_EQ(c.A(), 0);
}

TEST(ColorTest, SetColorRGB) {
    color c;
    c.set_color(1, 2, 3);
    EXPECT_EQ(c.R(), 1);
    EXPECT_EQ(c.G(), 2);
    EXPECT_EQ(c.B(), 3);
    EXPECT_EQ(c.A(), 255);
}

TEST(ColorTest, SetColorRGBA) {
    color c;
    c.set_color(5, 6, 7, 8);
    EXPECT_EQ(c.R(), 5);
    EXPECT_EQ(c.G(), 6);
    EXPECT_EQ(c.B(), 7);
    EXPECT_EQ(c.A(), 8);
}

TEST(ColorTest, SetGray) {
    color c;
    c.set_gray(77);
    EXPECT_EQ(c.R(), 77);
    EXPECT_EQ(c.G(), 77);
    EXPECT_EQ(c.B(), 77);
    EXPECT_EQ(c.A(), 255);
    c.set_gray(22, 33);
    EXPECT_EQ(c.R(), 22);
    EXPECT_EQ(c.A(), 33);
}

TEST(ColorTest, SetOpacity) {
    color c;
    c.set_opacity(0.5);
    EXPECT_EQ(c.A(), 128);
    c.set_opacity(1.0);
    EXPECT_EQ(c.A(), 255);
    c.set_opacity(0.0);
    EXPECT_EQ(c.A(), 0);
    c.set_opacity(1.5);
    EXPECT_EQ(c.A(), 255);
    c.set_opacity(-0.1);
    EXPECT_EQ(c.A(), 0);
}

TEST(ColorTest, OpacityProperty) {
    color c(0, 0, 0, 200);
    EXPECT_NEAR(c.opacity(), 200 / 255.0, 0.01);
}

TEST(ColorTest, TransparentOpaque) {
    EXPECT_FALSE(color().is_transparent());
    EXPECT_TRUE(color().is_opaque());
    color clear(0, 0, 0, 0);
    EXPECT_TRUE(clear.is_transparent());
    EXPECT_FALSE(clear.is_opaque());
}

TEST(ColorTest, NamedColors) {
    EXPECT_TRUE(color_equal(color::black(), color(0, 0, 0, 255)));
    EXPECT_TRUE(color_equal(color::white(), color(255, 255, 255, 255)));
    EXPECT_TRUE(color_equal(color::gray(), color(128, 128, 128, 255)));
    EXPECT_TRUE(color_equal(color::red(), color(255, 0, 0, 255)));
    EXPECT_TRUE(color_equal(color::green(), color(0, 255, 0, 255)));
    EXPECT_TRUE(color_equal(color::blue(), color(0, 0, 255, 255)));
    EXPECT_TRUE(color_equal(color::yellow(), color(255, 255, 0, 255)));
    EXPECT_TRUE(color_equal(color::magenta(), color(255, 0, 255, 255)));
    EXPECT_TRUE(color_equal(color::cyan(), color(0, 255, 255, 255)));
    EXPECT_TRUE(color_equal(color::transparent(), color(0, 0, 0, 0)));
}

TEST(ColorTest, Lerp) {
    color from(0, 0, 0, 255);
    color to(100, 200, 50, 100);
    auto mid = color::lerp(from, to, 0.5);
    EXPECT_EQ(mid.R(), 50);
    EXPECT_EQ(mid.G(), 100);
    EXPECT_EQ(mid.B(), 25);
    EXPECT_EQ(mid.A(), 178);

    auto zero = color::lerp(from, to, 0.0);
    EXPECT_TRUE(color_equal(zero, from));

    auto one = color::lerp(from, to, 1.0);
    EXPECT_TRUE(color_equal(one, to));

    auto over = color::lerp(from, to, 2.0);
    EXPECT_TRUE(color_equal(over, to));
}

TEST(ColorTest, BlendOpaqueOverOpaque) {
    color fg(100, 0, 0, 255);
    color bg(0, 100, 0, 255);
    auto result = fg.blend(bg);
    EXPECT_TRUE(color_equal(result, fg));
}

TEST(ColorTest, BlendTransparentFg) {
    color fg(0, 0, 0, 0);
    color bg(10, 20, 30, 255);
    EXPECT_TRUE(color_equal(fg.blend(bg), bg));
}

TEST(ColorTest, BlendHalfTransparent) {
    color fg(255, 0, 0, 128);
    color bg(0, 255, 0, 255);
    auto res = fg.blend(bg);
    EXPECT_EQ(res.R(), 128);
    EXPECT_EQ(res.G(), 127);
    EXPECT_EQ(res.B(), 0);
    EXPECT_EQ(res.A(), 255);
}

TEST(ColorTest, Addition) {
    color c1(10, 20, 30, 40);
    color c2(5, 5, 5, 5);
    auto sum = c1 + c2;
    EXPECT_EQ(sum.R(), 15);
    EXPECT_EQ(sum.G(), 25);
    EXPECT_EQ(sum.B(), 35);
    EXPECT_EQ(sum.A(), 45);
}

TEST(ColorTest, Subtraction) {
    color c1(50, 60, 70, 80);
    color c2(10, 20, 30, 40);
    auto diff = c1 - c2;
    EXPECT_EQ(diff.R(), 40);
    EXPECT_EQ(diff.G(), 40);
    EXPECT_EQ(diff.B(), 40);
    EXPECT_EQ(diff.A(), 40);
}

TEST(ColorTest, MultiplyByDouble) {
    color c(100, 100, 100, 200);
    auto scaled = c * 0.5;
    EXPECT_EQ(scaled.R(), 50);
    EXPECT_EQ(scaled.G(), 50);
    EXPECT_EQ(scaled.B(), 50);
    EXPECT_EQ(scaled.A(), 100);
}

TEST(ColorTest, MultiplyByInt) {
    color c(10, 20, 30, 40);
    auto scaled = c * 2;
    EXPECT_EQ(scaled.R(), 20);
    EXPECT_EQ(scaled.G(), 40);
    EXPECT_EQ(scaled.B(), 60);
    EXPECT_EQ(scaled.A(), 80);
}

TEST(ColorTest, ScalarColorMultiply) {
    color c(10, 20, 30, 40);
    auto r1 = 0.5 * c;
    auto r2 = 3 * c;
    EXPECT_TRUE(color_equal(r1, c * 0.5));
    EXPECT_TRUE(color_equal(r2, c * 3));
}

TEST(ColorTest, AddAssign) {
    color c(1, 2, 3, 4);
    c += color(1, 1, 1, 1);
    EXPECT_EQ(c.R(), 2);
    c += color(254, 254, 254, 254);
    EXPECT_EQ(c.R(), 255);
}

TEST(ColorTest, SubAssign) {
    color c(100, 100, 100, 100);
    c -= color(50, 60, 70, 80);
    EXPECT_EQ(c.R(), 50);
    c -= color(100, 0, 0, 0);
    EXPECT_EQ(c.R(), 0);
}

TEST(ColorTest, MulAssign) {
    color c(200, 100, 50, 128);
    c *= 0.5;
    EXPECT_EQ(c.R(), 100);
    EXPECT_EQ(c.A(), 64);
}

TEST(ColorTest, Equality) {
    color a(10, 20, 30, 40);
    color b(10, 20, 30, 40);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    color c(10, 20, 30, 41);
    EXPECT_FALSE(a == c);
}

TEST(ColorTest, LessThan) {
    color a(1, 2, 3, 4);
    color b(1, 2, 3, 5);
    EXPECT_TRUE(a < b);
    color c(1, 2, 4, 0);
    EXPECT_TRUE(a < c);
    color d(1, 3, 2, 0);
    EXPECT_TRUE(a < d);
    color e(2, 1, 1, 1);
    EXPECT_TRUE(a < e);
}

TEST(ColorTest, GrayValue) {
    color red(255, 0, 0);
    double expected_red = 0.299 * 255 + 0.587 * 0 + 0.114 * 0;
    EXPECT_NEAR(red.gray_value(), expected_red, 0.01);

    color white(255, 255, 255);
    EXPECT_NEAR(white.gray_value(), 255.0, 0.01);
}

TEST(ColorTest, ToString) {
    color c(0x12, 0x34, 0x56, 0x78);
    EXPECT_EQ(c.to_string(), "12345678");
    color c2(0, 0, 0, 255);
    EXPECT_EQ(c2.to_string(), "000000FF");
}

TEST(ColorTest, ParseValid) {
    auto c = color::parse("AABBCC");
    EXPECT_EQ(c.R(), 0xAA);
    EXPECT_EQ(c.G(), 0xBB);
    EXPECT_EQ(c.B(), 0xCC);
    EXPECT_EQ(c.A(), 255);
}

TEST(ColorTest, ParseWithHash) {
    auto c = color::parse("#aabbccdd");
    EXPECT_EQ(c.R(), 0xAA);
    EXPECT_EQ(c.A(), 0xDD);
}

TEST(ColorTest, ParseInvalidThrows) {
    EXPECT_THROW(color::parse("12345"), value_exception);
    EXPECT_THROW(color::parse("GGGGGG"), value_exception);
}

TEST(ColorTest, ToAnsi256Black) {
    color black = color::black();
    EXPECT_EQ(black.to_ansi_256(), 16);
}

TEST(ColorTest, ToAnsi256White) {
    color white = color::white();
    EXPECT_EQ(white.to_ansi_256(), 231);
}

TEST(ColorTest, ToAnsi256PureRed) {
    color red = color::red();
    int idx = red.to_ansi_256();
    EXPECT_EQ(idx, 196);
}

TEST(ColorTest, ToAnsi256Green) {
    color green(0, 255, 0);
    int idx = green.to_ansi_256();
    EXPECT_EQ(idx, 46);
}

TEST(ColorTest, ToAnsi256Gray) {
    color gray(100, 100, 100);
    int idx = gray.to_ansi_256();
    EXPECT_EQ(idx, 241);
}

TEST(ColorTest, ToAnsiBasicRed) {
    color red = color::red();
    EXPECT_EQ(red.to_ansi_basic(false), 31);
    EXPECT_EQ(red.to_ansi_basic(true), 41);
}

TEST(ColorTest, ToAnsiBasicGray) {
    color dark(0, 0, 0);
    EXPECT_EQ(dark.to_ansi_basic(false), 30);
    color medium(128, 128, 128);
    EXPECT_EQ(medium.to_ansi_basic(false), 37);
}

TEST(ColorTest, ToAnsiForeground) {
    color red = color::red();
    auto fg = red.to_ansi_foreground(true);
    EXPECT_EQ(fg.value(), 4046);
    auto fg_basic = red.to_ansi_foreground(false);
    EXPECT_EQ(fg_basic.value(), 31);
}

TEST(ColorTest, ToAnsiBackground) {
    color red = color::red();
    auto bg = red.to_ansi_background(true);
    EXPECT_EQ(bg.value(), 5046);
    auto bg_basic = red.to_ansi_background(false);
    EXPECT_EQ(bg_basic.value(), 41);
}

TEST(ColorTest, ToInteger32) {
    color c(255, 0, 0);
    EXPECT_EQ(c.to_integer32(true).value(), 196);
    EXPECT_EQ(c.to_integer32(false).value(), 31);
}

TEST(ColorTest, ToPremultiplied) {
    color c(100, 100, 100, 100);
    auto pm = c.to_premultiplied();
    EXPECT_NEAR(pm.R(), 39, 1);
    EXPECT_NEAR(pm.G(), 39, 1);
    EXPECT_NEAR(pm.B(), 39, 1);
    EXPECT_EQ(pm.A(), 100);
}

TEST(ColorTest, FromPremultiplied) {
    color pm(39, 39, 39, 100);
    auto straight = pm.from_premultiplied();
    EXPECT_NEAR(straight.R(), 99, 1);
    EXPECT_NEAR(straight.G(), 99, 1);
    EXPECT_NEAR(straight.B(), 99, 1);
    EXPECT_EQ(straight.A(), 100);
}

TEST(ColorTest, PremultipliedRoundTrip) {
    color original(200, 150, 50, 200);
    auto pm = original.to_premultiplied();
    auto back = pm.from_premultiplied();
    EXPECT_TRUE(color_equal(back, original));
}

TEST(ColorTest, Hash) {
    color c1(10, 20, 30, 40);
    color c2(10, 20, 30, 40);
    EXPECT_EQ(c1.to_hash(), c2.to_hash());
    color c3(10, 20, 30, 41);
    EXPECT_NE(c1.to_hash(), c3.to_hash());
}

TEST(ColorTest, Swap) {
    color a(1, 2, 3, 4);
    color b(5, 6, 7, 8);
    a.swap(b);
    EXPECT_EQ(a.R(), 5);
    EXPECT_EQ(b.R(), 1);
    EXPECT_EQ(a.A(), 8);
    EXPECT_EQ(b.A(), 4);
}

TEST(VariantTraitsTest, VariantAlternative) {
    EXPECT_TRUE((is_same_v<variant_alternative_t<variant<int, double, string>, 0>, int>) );
    EXPECT_TRUE((is_same_v<variant_alternative_t<variant<int, double, string>, 1>, double>) );
    EXPECT_TRUE((is_same_v<variant_alternative_t<variant<int, double, string>, 2>, string>) );
}

TEST(VariantTraitsTest, VariantIndex) {
    EXPECT_EQ((variant_index_v<variant<int, double, string>, int>), 0u);
    EXPECT_EQ((variant_index_v<variant<int, double, string>, double>), 1u);
    EXPECT_EQ((variant_index_v<variant<int, double, string>, string>), 2u);
}

TEST(VariantTest, DefaultConstructor) {
    variant<int, double> v;
    EXPECT_EQ(v.index(), 0u);
    EXPECT_TRUE(v.holds_alternative<int>());
    EXPECT_EQ(v.get<0>(), 0);
}

TEST(VariantTest, ConstructFromValue) {
    variant<int, double, string> v(3.14);
    EXPECT_EQ(v.index(), 1u);
    EXPECT_TRUE(v.holds_alternative<double>());
    EXPECT_DOUBLE_EQ(v.get<1>(), 3.14);
}

TEST(VariantTest, ConstructFromString) {
    variant<int, string> v(string("hello"));
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(v.get<string>(), "hello");
}

TEST(VariantTest, CopyConstructor) {
    variant<int, string> v1(42);
    variant<int, string> v2(v1);
    EXPECT_EQ(v2.index(), 0u);
    EXPECT_EQ(v2.get<0>(), 42);
}

TEST(VariantTest, CopyAssignment) {
    variant<int, string> v1(string("world"));
    variant<int, string> v2;
    v2 = v1;
    EXPECT_EQ(v2.index(), 1u);
    EXPECT_EQ(v2.get<1>(), "world");
    v2 = v2;
    EXPECT_EQ(v2.get<1>(), "world");
}

TEST(VariantTest, MoveConstructor) {
    variant<int, string> v1(string("move"));
    variant<int, string> v2(move(v1));
    EXPECT_EQ(v2.index(), 1u);
    EXPECT_EQ(v2.get<string>(), "move");
}

TEST(VariantTest, MoveAssignment) {
    variant<int, string> v1(10);
    variant<int, string> v2(string("src"));
    v1 = move(v2);
    EXPECT_EQ(v1.index(), 1u);
    EXPECT_EQ(v1.get<string>(), "src");
    v1 = move(v1);
    EXPECT_EQ(v1.get<string>(), "src");
}

TEST(VariantTest, InPlaceConstruct) {
    variant<int, double, string> v(pass_size_construct_tag<1>{}, 1.0);
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(v.get<1>(), 1.0);
}

TEST(VariantTest, InPlaceConstructWithInitializerList) {
    variant<int, string> v(pass_size_construct_tag<1>{}, initializer_list<char>{'a', 'b', 'c'});
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(v.get<1>(), "abc");
}

TEST(VariantTest, GenericConstruction) {
    variant<int, string> v("literal");
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(v.get<string>(), "literal");
}

TEST(VariantTest, VisitMutable) {
    variant<int, double, string> v(42);
    auto result = v.visit([](auto& val) -> string {
        if (is_same_v<decay_t<decltype(val)>, int>) {
            return to_string(val);
        }
        return "other";
    });
    EXPECT_EQ(result, "42");
}

TEST(VariantTest, VisitConst) {
    const variant<int, string> v(string("const"));
    size_t hash = v.visit([](const auto& val) -> size_t { return neforce::hash<decay_t<decltype(val)>>{}(val); });
    EXPECT_NE(hash, 0u);
}

TEST(VariantTest, IndexAndHoldsAlternative) {
    variant<int, double> v(3.14);
    EXPECT_EQ(v.index(), 1u);
    EXPECT_TRUE(v.holds_alternative<double>());
    EXPECT_FALSE(v.holds_alternative<int>());
}

TEST(VariantTest, GetByIndex) {
    variant<int, double> v(10);
    EXPECT_EQ(v.get<0>(), 10);
    v.get<0>() = 20;
    EXPECT_EQ(v.get<0>(), 20);
}

TEST(VariantTest, GetByIndexConst) {
    const variant<int, double> v(5);
    EXPECT_EQ(v.get<0>(), 5);
}

TEST(VariantTest, GetByIndexThrows) {
    variant<int, double> v(3.14);
    EXPECT_THROW(v.get<0>(), value_exception);
}

TEST(VariantTest, GetByType) {
    variant<int, string> v(string("type"));
    EXPECT_EQ(v.get<string>(), "type");
}

TEST(VariantTest, GetByTypeConst) {
    const variant<int, string> v(42);
    EXPECT_EQ(v.get<int>(), 42);
}

TEST(VariantTest, GetIfByIndex) {
    variant<int, double> v(3.14);
    EXPECT_EQ(v.get_if<0>(), nullptr);
    auto ptr = v.get_if<1>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 3.14);
}

TEST(VariantTest, GetIfByIndexConst) {
    const variant<int, double> v(2.71);
    auto ptr = v.get_if<1>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_DOUBLE_EQ(*ptr, 2.71);
}

TEST(VariantTest, GetIfByType) {
    variant<int, string> v(string("test"));
    EXPECT_EQ(v.get_if<int>(), nullptr);
    auto p = v.get_if<string>();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, "test");
}

TEST(VariantTest, EmplaceByIndex) {
    variant<int, string> v(10);
    v.emplace<1>(3, 'x');
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(v.get<1>(), "xxx");
}

TEST(VariantTest, EmplaceByType) {
    variant<int, double> v(3.14);
    v.emplace<int>(42);
    EXPECT_EQ(v.get<int>(), 42);
}

TEST(VariantTest, Swap) {
    variant<int, string> v1(100);
    variant<int, string> v2(string("swap"));
    v1.swap(v2);
    EXPECT_EQ(v1.index(), 1u);
    EXPECT_EQ(v1.get<string>(), "swap");
    EXPECT_EQ(v2.index(), 0u);
    EXPECT_EQ(v2.get<int>(), 100);
    v1.swap(v1);
    EXPECT_EQ(v1.get<string>(), "swap");
}

TEST(VariantTest, Equality) {
    variant<int, string> a(42);
    variant<int, string> b(42);
    EXPECT_TRUE(a == b);
    variant<int, string> c(10);
    EXPECT_FALSE(a == c);
    variant<int, string> d(string("x"));
    EXPECT_FALSE(a == d);
}

TEST(VariantTest, LessThan) {
    variant<int, string> a(10);
    variant<int, string> b(20);
    EXPECT_TRUE(a < b);
    variant<int, string> c(string("a"));
    EXPECT_FALSE(a < c);
}

TEST(VariantTest, Hash) {
    variant<int, string> v1(42);
    variant<int, string> v2(42);
    variant<int, string> v3(43);
    EXPECT_EQ(v1.to_hash(), v2.to_hash());
    EXPECT_NE(v1.to_hash(), v3.to_hash());
}

TEST(VariantExternalGetTest, LvalueRef) {
    variant<int, double> v(3.14);
    auto& val = get<1>(v);
    EXPECT_EQ(val, 3.14);
}

TEST(VariantExternalGetTest, ConstLvalueRef) {
    const variant<int, double> v(2.71);
    auto& val = get<1>(v);
    EXPECT_EQ(val, 2.71);
}

TEST(VariantExternalGetTest, RvalueRef) {
    auto&& val = get<0>(variant<int, string>(10));
    static_assert(is_same_v<decltype(val), int&&>);
    EXPECT_EQ(val, 10);
}

TEST(VariantExternalGetTest, ConstRvalueRef) {
    const variant<int, string> v(string("tmp"));
    auto&& val = get<1>(static_cast<const variant<int, string>&&>(v));
    static_assert(is_same_v<decltype(val), const string&&>);
    EXPECT_EQ(val, "tmp");
}

TEST(OptionalTest, DefaultConstruct) {
    optional<int> op;
    EXPECT_FALSE(op.has_value());
    EXPECT_FALSE(op);
    EXPECT_TRUE(op == none);
    EXPECT_TRUE(none == op);
}

TEST(OptionalTest, ValueConstruct) {
    optional<int> op(42);
    EXPECT_TRUE(op.has_value());
    EXPECT_TRUE(op);
    EXPECT_EQ(op.value(), 42);
    EXPECT_EQ(*op, 42);
}

TEST(OptionalTest, CopyConstruct) {
    optional<string> op1("hello");
    optional<string> op2(op1);
    EXPECT_EQ(op2.value(), "hello");
}

TEST(OptionalTest, MoveConstruct) {
    optional<string> op1("world");
    optional<string> op2(move(op1));
    EXPECT_EQ(op2.value(), "world");
    EXPECT_FALSE(op1.has_value());
}

TEST(OptionalTest, CopyAssignment) {
    optional<int> op1(10);
    optional<int> op2;
    op2 = op1;
    EXPECT_EQ(op2.value(), 10);
    op2 = op2;
    EXPECT_EQ(op2.value(), 10);
}

TEST(OptionalTest, MoveAssignment) {
    optional<int> op1(20);
    optional<int> op2;
    op2 = move(op1);
    EXPECT_EQ(op2.value(), 20);
    EXPECT_FALSE(op1.has_value());
    op2 = move(op2);
    EXPECT_EQ(op2.value(), 20);
}

TEST(OptionalTest, ValueAssignment) {
    optional<int> op;
    op = 5;
    EXPECT_EQ(op.value(), 5);
    op = 10;
    EXPECT_EQ(op.value(), 10);
    op = none;
    EXPECT_FALSE(op.has_value());
}

TEST(OptionalTest, ConvertibleValueConstruct) {
    optional<string> op("literal");
    EXPECT_EQ(op.value(), "literal");
}

TEST(OptionalTest, ExplicitConvertibleConstruct) {
    optional<double> op(3);
    EXPECT_DOUBLE_EQ(op.value(), 3.0);
}

TEST(OptionalTest, OptionalFromOptional) {
    optional<int> op1(1);
    optional<double> op2(op1);
    EXPECT_DOUBLE_EQ(op2.value(), 1.0);
    optional<int> empty;
    optional<double> op3(empty);
    EXPECT_FALSE(op3.has_value());
}

TEST(OptionalTest, OptionalAssignFromOptional) {
    optional<int> op1(1);
    optional<double> op2;
    op2 = op1;
    EXPECT_DOUBLE_EQ(op2.value(), 1.0);
    op2 = optional<int>();
    EXPECT_FALSE(op2.has_value());
}

TEST(OptionalTest, InPlaceConstruct) {
    optional<vector<int>> op(inplace_construct_tag{}, 3, 2);
    EXPECT_EQ(op.value(), (vector<int>{2, 2, 2}));
}

TEST(OptionalTest, InPlaceConstructInitList) {
    optional<vector<int>> op(inplace_construct_tag{}, initializer_list<int>{1, 2, 3});
    EXPECT_EQ(op.value(), (vector<int>{1, 2, 3}));
}

TEST(OptionalTest, Emplace) {
    optional<string> op;
    op.emplace("hello");
    EXPECT_EQ(op.value(), "hello");
    op.emplace(5, 'a');
    EXPECT_EQ(op.value(), "aaaaa");
}

TEST(OptionalTest, Reset) {
    optional<int> op(42);
    EXPECT_TRUE(op.has_value());
    op.reset();
    EXPECT_FALSE(op.has_value());
    op.reset();
    EXPECT_FALSE(op.has_value());
}

TEST(OptionalTest, ValueLvalue) {
    optional<int> op(10);
    EXPECT_EQ(op.value(), 10);
    op.value() = 20;
    EXPECT_EQ(op.value(), 20);
}

TEST(OptionalTest, ValueConstLvalue) {
    const optional<int> op(30);
    EXPECT_EQ(op.value(), 30);
}

TEST(OptionalTest, ValueRvalue) {
    optional<string> op("rvalue");
    string s = move(op).value();
    EXPECT_EQ(s, "rvalue");
}

TEST(OptionalTest, ValueOnEmpty) {
    optional<int> op;
    EXPECT_THROW(op.value(), optional_exception);
    const optional<int> cop;
    EXPECT_THROW(cop.value(), optional_exception);
    EXPECT_THROW(move(op).value(), optional_exception);
}

TEST(OptionalTest, ValueOrLvalue) {
    optional<int> op;
    EXPECT_EQ(op.value_or(100), 100);
    op = 42;
    EXPECT_EQ(op.value_or(100), 42);
}

TEST(OptionalTest, ValueOrRvalue) {
    optional<string> op;
    string def = "default";
    EXPECT_EQ(move(op).value_or(def), def);
    op = "value";
    string s = move(op).value_or(def);
    EXPECT_EQ(s, "value");
}

TEST(OptionalTest, OperatorArrow) {
    optional<string> op("arrow");
    EXPECT_EQ(op->size(), 5u);
    op->clear();
    EXPECT_EQ(op->size(), 0u);
}

TEST(OptionalTest, OperatorDeref) {
    optional<int> op(7);
    EXPECT_EQ(*op, 7);
    *op = 8;
    EXPECT_EQ(*op, 8);
    const optional<int> cop(9);
    EXPECT_EQ(*cop, 9);
    optional<string> movable("move");
    string moved = *move(movable);
    EXPECT_EQ(moved, "move");
}

TEST(OptionalTest, OrElseLvalue) {
    optional<int> op;
    auto res = op.or_else([] { return optional<int>(10); });
    EXPECT_EQ(res.value(), 10);
    op = 5;
    auto res2 = op.or_else([] { return optional<int>(0); });
    EXPECT_EQ(res2.value(), 5);
}

TEST(OptionalTest, OrElseRvalue) {
    optional<string> op;
    auto res = move(op).or_else([] { return optional<string>("fallback"); });
    EXPECT_EQ(res.value(), "fallback");
    op = "keep";
    auto res2 = move(op).or_else([] { return optional<string>("ignored"); });
    EXPECT_EQ(res2.value(), "keep");
}

TEST(OptionalTest, AndThenLvalue) {
    optional<int> op(2);
    auto res = op.and_then([](int& x) { return to_string(x * 2); });
    EXPECT_EQ(res, "4");
    optional<int> empty;
    auto res2 = empty.and_then([](int&) { return string("nope"); });
    EXPECT_EQ(res2, string{});
}

TEST(OptionalTest, AndThenConstLvalue) {
    const optional<int> op(3);
    auto res = op.and_then([](const int& x) { return x * 3; });
    EXPECT_EQ(res, 9);
}

TEST(OptionalTest, AndThenRvalue) {
    optional<string> op("hello");
    auto res = move(op).and_then([](string&& s) { return s + " world"; });
    EXPECT_EQ(res, "hello world");
}

TEST(OptionalTest, TransformLvalue) {
    optional<int> op(2);
    auto res = op.transform([](int& x) { return x * 10; });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), 20);
}

TEST(OptionalTest, TransformConstLvalue) {
    const optional<int> op(5);
    auto res = op.transform([](const int& x) { return static_cast<double>(x) / 2.0; });
    ASSERT_TRUE(res.has_value());
    EXPECT_DOUBLE_EQ(res.value(), 2.5);
}

TEST(OptionalTest, TransformRvalue) {
    optional<string> op("test");
    auto res = move(op).transform([](string&& s) { return s.size(); });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), 4u);
}

TEST(OptionalTest, TransformEmpty) {
    optional<int> op;
    auto res = op.transform([](int&) { return 1; });
    EXPECT_FALSE(res.has_value());
}

TEST(OptionalTest, CompareEqual) {
    optional<int> a(1);
    optional<int> b(1);
    optional<int> c(2);
    optional<int> n;
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == n);
    EXPECT_TRUE(a != n);
    EXPECT_TRUE(n == n);
}

TEST(OptionalTest, CompareLess) {
    optional<int> a(1), b(2), n;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a < n);
    EXPECT_FALSE(n < a);
    EXPECT_FALSE(n < n);
}

TEST(OptionalTest, CompareWithNone) {
    optional<int> a;
    optional<int> b(1);
    EXPECT_TRUE(a == none);
    EXPECT_TRUE(none == a);
    EXPECT_FALSE(a != none);
    EXPECT_FALSE(a > none);
    EXPECT_FALSE(a < none);
    EXPECT_TRUE(a <= none);
    EXPECT_TRUE(a >= none);
    EXPECT_TRUE(none <= a);
    EXPECT_TRUE(none >= a);
    EXPECT_FALSE(b == none);
    EXPECT_TRUE(b != none);
    EXPECT_TRUE(b > none);
    EXPECT_TRUE(none < b);
    EXPECT_TRUE(b >= none);
    EXPECT_TRUE(none < b);
}

TEST(OptionalTest, Hash) {
    optional<int> a(42), b(42), c(43), n;
    EXPECT_EQ(a.to_hash(), b.to_hash());
    EXPECT_NE(a.to_hash(), c.to_hash());
    EXPECT_NE(a.to_hash(), n.to_hash());
}

TEST(OptionalTest, Swap) {
    optional<int> a(1), b(2);
    a.swap(b);
    EXPECT_EQ(a.value(), 2);
    EXPECT_EQ(b.value(), 1);
    optional<int> c;
    a.swap(c);
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(c.value(), 2);
    a.swap(a);
    EXPECT_FALSE(a.has_value());
}

TEST(OptionalTest, MakeOptionalValue) {
    auto op = make_optional<int>(42);
    static_assert(is_same_v<decltype(op), optional<int>>);
    EXPECT_EQ(op.value(), 42);
}

TEST(OptionalTest, MakeOptionalInPlace) {
    auto op = make_optional<string>(3, 'x');
    static_assert(is_same_v<decltype(op), optional<string>>);
    EXPECT_EQ(op.value(), "xxx");
}

TEST(OptionalTest, MakeOptionalInitList) {
    auto op = make_optional<vector<int>>({1, 2, 3});
    EXPECT_EQ(op.value(), (vector<int>{1, 2, 3}));
}

TEST(OptionalTest, ExternalGet) {
    optional<int> op(5);
    EXPECT_EQ(get(op), 5);
    get(op) = 10;
    EXPECT_EQ(get(op), 10);
    const optional<int> cop(7);
    EXPECT_EQ(get(cop), 7);
    EXPECT_EQ(get(optional<int>(3)), 3);
}

TEST(OptionalRefTest, DefaultConstruct) {
    optional<int&> ref;
    EXPECT_FALSE(ref.has_value());
}

TEST(OptionalRefTest, BindLvalue) {
    int x = 5;
    optional<int&> ref(x);
    EXPECT_TRUE(ref.has_value());
    EXPECT_EQ(ref.value(), 5);
    ref.value() = 10;
    EXPECT_EQ(x, 10);
}

TEST(OptionalRefTest, CopyConstruct) {
    int x = 3;
    optional<int&> a(x);
    optional<int&> b(a);
    EXPECT_EQ(b.value(), 3);
}

TEST(OptionalRefTest, MoveConstruct) {
    int x = 4;
    optional<int&> a(x);
    optional<int&> b(move(a));
    EXPECT_EQ(b.value(), 4);
}

TEST(OptionalRefTest, AssignCopy) {
    int x = 1, y = 2;
    optional<int&> a(x), b(y);
    a = b;
    EXPECT_EQ(a.value(), 2);
    a.value() = 5;
    EXPECT_EQ(y, 5);
}

TEST(OptionalRefTest, AssignValue) {
    int x = 0, y = 1;
    optional<int&> ref(x);
    ref = y;
    EXPECT_EQ(ref.value(), 1);
    ref.value() = 99;
    EXPECT_EQ(y, 99);
    EXPECT_EQ(x, 0);
}

TEST(OptionalRefTest, AssignNone) {
    int x = 10;
    optional<int&> ref(x);
    ref = none;
    EXPECT_FALSE(ref.has_value());
}

TEST(OptionalRefTest, Emplace) {
    int x = 7, y = 8;
    optional<int&> ref(x);
    ref.emplace(y);
    EXPECT_EQ(ref.value(), 8);
    ref.value() = 42;
    EXPECT_EQ(y, 42);
}

TEST(OptionalRefTest, Reset) {
    int x = 1;
    optional<int&> ref(x);
    ref.reset();
    EXPECT_FALSE(ref.has_value());
}

TEST(OptionalRefTest, ValueAccess) {
    int x = 100;
    optional<int&> ref(x);
    EXPECT_EQ(ref.value(), 100);
    ref.value() = 200;
    EXPECT_EQ(x, 200);
    const optional<int&> cref(x);
    EXPECT_EQ(cref.value(), 200);
}

TEST(OptionalRefTest, ValueOr) {
    int x = 50;
    optional<int&> ref(x);
    EXPECT_EQ(ref.value_or(0), 50);
    optional<int&> empty;
    EXPECT_EQ(empty.value_or(0), 0);
}

TEST(OptionalRefTest, OperatorArrow) {
    struct A {
        int val;
    };
    A a{42};
    optional<A&> ref(a);
    EXPECT_EQ(ref->val, 42);
    ref->val = 99;
    EXPECT_EQ(a.val, 99);
}

TEST(OptionalRefTest, OperatorDeref) {
    int x = 11;
    optional<int&> ref(x);
    EXPECT_EQ(*ref, 11);
    *ref = 22;
    EXPECT_EQ(x, 22);
}

TEST(OptionalRefTest, Compare) {
    int a = 1, b = 2;
    optional<int&> r1(a), r2(b), n;
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
    EXPECT_FALSE(r1 == n);
    EXPECT_TRUE(r1 != n);
    EXPECT_TRUE(n == n);
    EXPECT_TRUE(r1 < r2);
    EXPECT_FALSE(r1 < n);
    EXPECT_FALSE(n < r1);
}

TEST(OptionalRefTest, CompareNone) {
    int x = 0;
    optional<int&> r(x);
    EXPECT_FALSE(r == none);
    EXPECT_TRUE(none != r);
    EXPECT_TRUE(r > none);
    EXPECT_TRUE(none < r);
    EXPECT_FALSE(r < none);
}

TEST(OptionalRefTest, Hash) {
    int x = 42;
    optional<int&> r1(x);
    optional<int&> r2(r1);
    EXPECT_EQ(r1.to_hash(), r2.to_hash());
    optional<int&> n;
    EXPECT_NE(r1.to_hash(), n.to_hash());
}

TEST(OptionalRefTest, Swap) {
    int x = 5, y = 10;
    optional<int&> a(x), b(y);
    a.swap(b);
    EXPECT_EQ(a.value(), 10);
    EXPECT_EQ(b.value(), 5);
    a.swap(a);
    EXPECT_EQ(a.value(), 10);
}

TEST(OptionalRefTest, MonadicOrElse) {
    int x = 1;
    optional<int&> r(x);
    auto res = r.or_else([&x] { return optional<int&>(static_cast<int&>(x)); });
    EXPECT_TRUE(res.has_value());
    optional<int&> empty;
    auto fallback = empty.or_else([&] { return optional<int&>(x); });
    EXPECT_TRUE(fallback.has_value());
    EXPECT_EQ(fallback.value(), 1);
}

TEST(OptionalRefTest, AndThen) {
    int x = 3;
    optional<int&> r(x);
    auto res = r.and_then([](int& v) -> string { return to_string(v); });
    EXPECT_EQ(res, "3");
    optional<int&> empty;
    auto res2 = empty.and_then([](int&) -> string { return "none"; });
    EXPECT_EQ(res2, string{});
}

TEST(OptionalRefTest, Transform) {
    int x = 10;
    optional<int&> r(x);
    auto res = r.transform([](int& v) { return v * 2; });
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), 20);
    optional<int&> empty;
    auto res2 = empty.transform([](int&) { return 1; });
    EXPECT_FALSE(res2.has_value());
}

TEST(OptionalTraitsTest, IsOptional) {
    EXPECT_TRUE(is_optional_v<optional<int>>);
    EXPECT_TRUE(is_optional_v<optional<int&>>);
    EXPECT_FALSE(is_optional_v<int>);
    EXPECT_FALSE(is_optional_v<none_t>);
}

TEST(ScopeExitTest, InvokeOnDestruction) {
    int counter = 0;
    auto lambda = [&] { ++counter; };
    {
        scope_exit guard(lambda);
        EXPECT_EQ(counter, 0);
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeExitTest, ReleasePreventsInvocation) {
    int counter = 0;
    {
        auto guard = scope_exit([&] { ++counter; });
        guard.release();
    }
    EXPECT_EQ(counter, 0);
}

TEST(ScopeExitTest, MoveConstructorTransfersOwnership) {
    int counter = 0;
    {
        auto guard1 = scope_exit([&] { ++counter; });
        auto guard2 = move(guard1);
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeExitTest, MoveConstructorSourceReleased) {
    int counter = 0;
    auto guard1 = scope_exit([&] { ++counter; });
    auto guard2 = move(guard1);
    guard2.release();
    EXPECT_EQ(counter, 0);
}

TEST(ScopeExitTest, DoubleReleaseSafe) {
    int counter = 0;
    {
        auto guard = scope_exit([&] { ++counter; });
        guard.release();
        guard.release();
    }
    EXPECT_EQ(counter, 0);
}

TEST(ScopeExitTest, ExceptionDuringConstructionCallsFunc) {
    struct throwing_functor {
        throwing_functor() = default;
        explicit throwing_functor(int) { throw exception("construct failed"); }
        void operator()() { FAIL() << "Should not be called"; }
    };
    bool called = false;
    throwing_functor func;
    try {
        scope_exit<throwing_functor> guard(throwing_functor{0});
    } catch (...) {
    }
}

TEST(ScopeExitTest, ExceptionDuringConstructionCallsPassedFunction) {
    struct create_throws {
        bool* p;
        explicit create_throws(bool* ptr) :
        p(ptr) {
            if (p) {
                throw exception("fail");
            }
        }
        void operator()() { *p = true; }
    };
    bool called = false;
    try {
        scope_exit<create_throws> guard(create_throws{&called});
    } catch (...) {
    }
}

static_assert(!is_copy_constructible_v<scope_exit<void (*)()>>, "scope_exit must not be copyable");
static_assert(!is_copy_assignable_v<scope_exit<void (*)()>>, "scope_exit must not be copy assignable");
static_assert(!is_move_assignable_v<scope_exit<void (*)()>>, "scope_exit must not be move assignable");

TEST(ScopeFailTest, NoExceptionDoesNotInvoke) {
    int counter = 0;
    {
        auto guard = scope_fail([&] { ++counter; });
    }
    EXPECT_EQ(counter, 0);
}

TEST(ScopeFailTest, ExceptionInvokesGuard) {
    int counter = 0;
    try {
        auto guard = scope_fail([&] { ++counter; });
        throw exception("error");
    } catch (...) {
        EXPECT_EQ(counter, 1);
    }
}

TEST(ScopeFailTest, ReleasePreventsInvocation) {
    int counter = 0;
    try {
        auto guard = scope_fail([&] { ++counter; });
        guard.release();
        throw exception("error");
    } catch (...) {
    }
    EXPECT_EQ(counter, 0);
}

TEST(ScopeFailTest, MoveConstructorTransfers) {
    int counter = 0;
    try {
        auto guard1 = scope_fail([&] { ++counter; });
        auto guard2 = move(guard1);
        throw exception("error");
    } catch (...) {
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeFailTest, MovedFromDoesNotFire) {
    int counter = 0;
    try {
        auto guard1 = scope_fail([&] { ++counter; });
        auto guard2 = move(guard1);
        throw exception("error");
    } catch (...) {
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeFailTest, MultipleExceptions) {
    int counter = 0;
    try {
        auto guard = scope_fail([&] { ++counter; });
        try {
            throw 1;
        } catch (int) {
        }
        throw exception("outer");
    } catch (...) {
        EXPECT_EQ(counter, 1);
    }
}

TEST(ScopeFailTest, FiresWhenUncaughtExceptionsIncreases) {
    int counter = 0;
    auto outer = [&] {
        scope_fail guard([&] { ++counter; });
        throw 42;
    };
    try {
        outer();
    } catch (int) {
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeSuccessTest, NormalExitInvokes) {
    int counter = 0;
    {
        auto guard = scope_success([&] { ++counter; });
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeSuccessTest, ExceptionDoesNotInvoke) {
    int counter = 0;
    try {
        auto guard = scope_success([&] { ++counter; });
        throw exception("err");
    } catch (...) {
        EXPECT_EQ(counter, 0);
    }
}

TEST(ScopeSuccessTest, ReleasePreventsInvocation) {
    int counter = 0;
    {
        auto guard = scope_success([&] { ++counter; });
        guard.release();
    }
    EXPECT_EQ(counter, 0);
}

TEST(ScopeSuccessTest, MoveConstructorTransfers) {
    int counter = 0;
    {
        auto guard1 = scope_success([&] { ++counter; });
        auto guard2 = move(guard1);
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeSuccessTest, MovedFromDoesNotFire) {
    int counter = 0;
    {
        auto guard1 = scope_success([&] { ++counter; });
        auto guard2 = move(guard1);
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeSuccessTest, SuccessWhenExceptionCaughtInside) {
    int counter = 0;
    try {
        auto guard = scope_success([&] { ++counter; });
        try {
            throw 1;
        } catch (int) {
        }
    } catch (...) {
    }
    EXPECT_EQ(counter, 1);
}

TEST(ScopeFailExceptionTest, ConstructorThrowsCallsFunc) {
    struct throws_on_copy {
        bool* flag;
        throws_on_copy(bool* f) :
        flag(f) {}
        throws_on_copy(const throws_on_copy&) { throw exception("copy"); }
        void operator()() { *flag = true; }
    };
    bool called = false;
    throws_on_copy func(&called);
    try {
        scope_fail<throws_on_copy> guard(func);
    } catch (...) {
    }
    EXPECT_TRUE(called);
}

TEST(ScopeSuccessExceptionTest, ConstructorThrowsCallsFunc) {
    struct throws_on_copy {
        bool* flag;
        throws_on_copy(bool* f) :
        flag(f) {}
        throws_on_copy(const throws_on_copy&) { throw exception("copy"); }
        void operator()() { *flag = true; }
    };
    bool called = false;
    throws_on_copy func(&called);
    try {
        scope_success<throws_on_copy> guard(func);
    } catch (...) {
    }
    EXPECT_TRUE(called);
}

static_assert(!is_copy_constructible_v<scope_fail<void (*)()>>, "scope_fail not copyable");
static_assert(!is_copy_assignable_v<scope_fail<void (*)()>>, "");
static_assert(!is_move_assignable_v<scope_fail<void (*)()>>, "");

static_assert(!is_copy_constructible_v<scope_success<void (*)()>>, "scope_success not copyable");
static_assert(!is_copy_assignable_v<scope_success<void (*)()>>, "");
static_assert(!is_move_assignable_v<scope_success<void (*)()>>, "");

TEST(ScopeExitTest, ReleaseAfterMove) {
    int counter = 0;
    auto guard1 = scope_exit([&] { ++counter; });
    auto guard2 = move(guard1);
    guard1.release();
    EXPECT_EQ(counter, 0);
}

TEST(ScopeFailTest, CalledExactlyOnce) {
    int counter = 0;
    try {
        auto guard = scope_fail([&] { ++counter; });
        throw 1;
    } catch (int) {
    }
    EXPECT_EQ(counter, 1);
}

TEST(HexadecimalTest, XdigitValueDigits) {
    EXPECT_EQ(hexadecimal::xdigit_value('0'), 0);
    EXPECT_EQ(hexadecimal::xdigit_value('9'), 9);
    EXPECT_EQ(hexadecimal::xdigit_value('a'), 10);
    EXPECT_EQ(hexadecimal::xdigit_value('f'), 15);
    EXPECT_EQ(hexadecimal::xdigit_value('A'), 10);
    EXPECT_EQ(hexadecimal::xdigit_value('F'), 15);
}

TEST(HexadecimalTest, XdigitValueInvalid) {
    EXPECT_EQ(hexadecimal::xdigit_value('g'), hexadecimal::invalid_xdigit);
    EXPECT_EQ(hexadecimal::xdigit_value(' '), hexadecimal::invalid_xdigit);
    EXPECT_EQ(hexadecimal::xdigit_value('\0'), hexadecimal::invalid_xdigit);
}

TEST(HexadecimalTest, XdigitValuePairValid) {
    auto [ok, val] = hexadecimal::xdigit_value('a', '1');
    EXPECT_TRUE(ok);
    EXPECT_EQ(val, 0xA1);
}

TEST(HexadecimalTest, XdigitValuePairInvalidHigh) {
    auto [ok, val] = hexadecimal::xdigit_value('x', '2');
    EXPECT_FALSE(ok);
    EXPECT_EQ(val, hexadecimal::invalid_xdigit);
}

TEST(HexadecimalTest, XdigitValuePairInvalidLow) {
    auto [ok, val] = hexadecimal::xdigit_value('F', 'G');
    EXPECT_FALSE(ok);
    EXPECT_EQ(val, hexadecimal::invalid_xdigit);
}

TEST(HexadecimalTest, DefaultConstruct) {
    hexadecimal h;
    EXPECT_FALSE(h);
    EXPECT_EQ(h.value(), 0);
}

TEST(HexadecimalTest, ConstructFromIntTypes) {
    hexadecimal h16(int16_t(0x7FFF));
    EXPECT_EQ(h16.value(), 0x7FFF);
    hexadecimal h32(int32_t(0x12345678));
    EXPECT_EQ(h32.value(), 0x12345678);
    hexadecimal hu16(uint16_t(0xABCDu));
    EXPECT_EQ(hu16.value(), 0xABCD);
    hexadecimal hu32(uint32_t(0xDEADBEEFu));
    EXPECT_EQ(hu32.value(), 0xDEADBEEF);
    hexadecimal hu64(uint64_t(0x123456789ABCDEF0ULL));
    EXPECT_EQ(hu64.value(), 0x123456789ABCDEF0ULL);
}

TEST(HexadecimalTest, ConstructFromInt64) {
    hexadecimal h(int64_t(-1));
    EXPECT_EQ(h.value(), -1);
    hexadecimal h2(0xABCDEF123456);
    EXPECT_EQ(h2.value(), 0xABCDEF123456);
}

TEST(HexadecimalTest, ConstructFromStringView) {
    hexadecimal h(string_view("1a"));
    EXPECT_EQ(h.value(), 0x1a);
    hexadecimal h2(string_view("0XFF"));
    EXPECT_EQ(h2.value(), 0xFF);
    hexadecimal h3(string_view("-10"));
    EXPECT_EQ(h3.value(), -0x10);
}

TEST(HexadecimalTest, ConstructFromCString) {
    hexadecimal h("dead");
    EXPECT_EQ(h.value(), 0xdead);
    hexadecimal h2("0xBEEF");
    EXPECT_EQ(h2.value(), 0xBEEF);
}

TEST(HexadecimalTest, ConstructFromStdString) {
    string s("ABCDEF");
    hexadecimal h(s);
    EXPECT_EQ(h.value(), 0xABCDEF);
}

TEST(HexadecimalTest, ParseEmpty) {
    hexadecimal h(string_view(""));
    EXPECT_EQ(h.value(), 0);
}

TEST(HexadecimalTest, ParseOnlySpaces) {
    hexadecimal h(string_view("   "));
    EXPECT_EQ(h.value(), 0);
}

TEST(HexadecimalTest, ParseWithSpaces) {
    hexadecimal h(string_view("  AB  "));
    EXPECT_EQ(h.value(), 0xAB);
}

TEST(HexadecimalTest, ParseOverflowPositive) {
    EXPECT_THROW(hexadecimal(string_view("FFFFFFFFFFFFFFFF")), value_exception);
    string big = "10000000000000000";
    EXPECT_THROW(hexadecimal(big.view()), value_exception);
}

TEST(HexadecimalTest, ParseOverflowNegative) {
    hexadecimal h(string_view("-8000000000000000"));
    EXPECT_EQ(h.value(), (numeric_traits<int64_t>::min)());
    EXPECT_THROW(hexadecimal(string_view("-8000000000000001")), value_exception);
}

TEST(HexadecimalTest, ParseInvalidCharacter) { EXPECT_THROW(hexadecimal(string_view("ZZ")), value_exception); }

TEST(HexadecimalTest, ParseWithSign) {
    hexadecimal h("+1A");
    EXPECT_EQ(h.value(), 0x1A);
    hexadecimal h2("-1A");
    EXPECT_EQ(h2.value(), -0x1A);
}

TEST(HexadecimalTest, CopyConstruct) {
    hexadecimal h(0x123);
    hexadecimal h2(h);
    EXPECT_EQ(h2.value(), 0x123);
}

TEST(HexadecimalTest, CopyAssign) {
    hexadecimal h(0x123);
    hexadecimal h2;
    h2 = h;
    EXPECT_EQ(h2.value(), 0x123);
}

TEST(HexadecimalTest, MoveConstruct) {
    hexadecimal h(0xabc);
    hexadecimal h2(move(h));
    EXPECT_EQ(h2.value(), 0xabc);
}

TEST(HexadecimalTest, MoveAssign) {
    hexadecimal h(0xabc);
    hexadecimal h2;
    h2 = move(h);
    EXPECT_EQ(h2.value(), 0xabc);
}

TEST(HexadecimalTest, AssignFromInt64) {
    hexadecimal h;
    h = int64_t(0x1234);
    EXPECT_EQ(h.value(), 0x1234);
}

TEST(HexadecimalTest, BoolConversion) {
    EXPECT_FALSE(hexadecimal());
    EXPECT_TRUE(hexadecimal(1));
    EXPECT_TRUE(hexadecimal(-1));
}

TEST(HexadecimalTest, GetBit) {
    hexadecimal h(0x5);
    EXPECT_TRUE(h.get_bit(0));
    EXPECT_FALSE(h.get_bit(1));
    EXPECT_TRUE(h.get_bit(2));
    EXPECT_FALSE(h.get_bit(63));
}

TEST(HexadecimalTest, GetBitOutOfRange) {
    hexadecimal h;
    EXPECT_THROW((void) h.get_bit(64), value_exception);
    EXPECT_THROW((void) h.get_bit(65), value_exception);
}

TEST(HexadecimalTest, SetBit) {
    hexadecimal h(0);
    h.set_bit(3);
    EXPECT_EQ(h.value(), 8);
    h.set_bit(3, false);
    EXPECT_EQ(h.value(), 0);
}

TEST(HexadecimalTest, SetBitOutOfRange) {
    hexadecimal h;
    EXPECT_THROW(h.set_bit(64), value_exception);
}

TEST(HexadecimalTest, FlipBit) {
    hexadecimal h(0xF);
    h.flip_bit(1);
    EXPECT_EQ(h.value(), 0x0D);
    h.flip_bit(1);
    EXPECT_EQ(h.value(), 0x0F);
    h.flip_bit(63);
    EXPECT_EQ(h.value(), 0x800000000000000FULL);
}

TEST(HexadecimalTest, FlipBitOutOfRange) {
    hexadecimal h;
    EXPECT_THROW(h.flip_bit(128), value_exception);
}

TEST(HexadecimalTest, ToStringPositive) {
    hexadecimal h(0xABCDEF);
    EXPECT_EQ(h.to_string(), "0xabcdef");
}

TEST(HexadecimalTest, ToStringZero) {
    hexadecimal h(0);
    EXPECT_EQ(h.to_string(), "0x0");
}

TEST(HexadecimalTest, ToStringNegative) {
    hexadecimal h(-0x10);
    EXPECT_EQ(h.to_string(), "-0x10");
}

TEST(HexadecimalTest, StaticParse) {
    auto h = hexadecimal::parse("1A2B");
    EXPECT_EQ(h.value(), 0x1A2B);
}

TEST(HexadecimalTest, LiteralsString) {
    auto h = "1a2b3c"_hex;
    EXPECT_EQ(h.value(), 0x1a2b3c);
}

TEST(HexadecimalTest, LiteralsInteger) {
    auto h = 0xABCD_hex;
    EXPECT_EQ(h.value(), 0xABCD);
}

TEST(HexadecimalTest, PackageValueAccess) {
    hexadecimal h(42);
    EXPECT_EQ(h.value(), 42);
}

TEST(ByteSizeTest, DefaultConstructor_ZeroBytes) {
    byte_size bs;
    EXPECT_EQ(bs.bytes(), 0ULL);
    EXPECT_TRUE(bs.is_zero());
}

TEST(ByteSizeTest, ExplicitFromUint64_ExactValue) {
    byte_size bs(1024ULL);
    EXPECT_EQ(bs.bytes(), 1024ULL);
}

TEST(ByteSizeTest, FromValueUnit_BinaryTrue) {
    byte_size bs(decimal_t(1.5), byte_size::unit::KB, true);
    EXPECT_EQ(bs.bytes(), 1536ULL);
}

TEST(ByteSizeTest, FromValueUnit_BinaryFalse) {
    byte_size bs(decimal_t(2.5), byte_size::unit::MB, false);
    EXPECT_EQ(bs.bytes(), 2500000ULL);
}

TEST(ByteSizeTest, FromValueUnit_NegativeValueThrows) {
    EXPECT_THROW(byte_size(decimal_t(-1.0), byte_size::unit::KB, true), value_exception);
}

TEST(ByteSizeTest, FromValueUnit_AutoUnitThrows) {
    EXPECT_THROW(byte_size(decimal_t(10.0), byte_size::unit::AUTO, true), value_exception);
}

TEST(ByteSizeTest, FromValueUnit_OverflowThrows) {
    EXPECT_THROW(byte_size(decimal_t(18.0), byte_size::unit::EB, true), value_exception);
}

TEST(ByteSizeTest, Parse_NoUnitDefaultByte) {
    auto bs = byte_size::parse("1024");
    EXPECT_EQ(bs.bytes(), 1024ULL);
}

TEST(ByteSizeTest, Parse_WithBinaryUnit) {
    auto bs = byte_size::parse("2 KB");
    EXPECT_EQ(bs.bytes(), 2048ULL);
}

TEST(ByteSizeTest, Parse_DecimalMode) {
    auto bs = byte_size::parse("2 KB", false);
    EXPECT_EQ(bs.bytes(), 2000ULL);
}

TEST(ByteSizeTest, Parse_CaseInsensitive) {
    auto bs = byte_size::parse("1.5 gb");
    EXPECT_EQ(bs.bytes(), static_cast<uint64_t>(1.5 * 1024 * 1024 * 1024));
}

TEST(ByteSizeTest, Parse_SingleLetterAlias) {
    auto bs = byte_size::parse("3M");
    EXPECT_EQ(bs.bytes(), 3ULL * 1024 * 1024);
}

TEST(ByteSizeTest, Parse_WithSpaces) {
    auto bs = byte_size::parse("  500  kB  ");
    EXPECT_EQ(bs.bytes(), 500ULL * 1024);
}

TEST(ByteSizeTest, Parse_EmptyStringThrows) { EXPECT_THROW(ignore = byte_size::parse(""), value_exception); }

TEST(ByteSizeTest, Parse_InvalidNumberThrows) { EXPECT_THROW(ignore = byte_size::parse("abc"), value_exception); }

TEST(ByteSizeTest, Parse_NegativeNumberThrows) { EXPECT_THROW(ignore = byte_size::parse("-10 B"), value_exception); }

TEST(ByteSizeTest, Parse_UnknownUnitThrows) { EXPECT_THROW(ignore = byte_size::parse("10 XY"), value_exception); }

TEST(ByteSizeTest, Parse_OnlyUnitThrows) { EXPECT_THROW(ignore = byte_size::parse("KB"), value_exception); }

TEST(ByteSizeTest, As_ToSameUnit) {
    byte_size bs(1024ULL);
    auto val = bs.as(byte_size::unit::B);
    EXPECT_DOUBLE_EQ(static_cast<double>(val), 1024.0);
}

TEST(ByteSizeTest, As_ToHigherUnitBinary) {
    byte_size bs(2048ULL);
    auto val = bs.as(byte_size::unit::KB, true);
    EXPECT_DOUBLE_EQ(static_cast<double>(val), 2.0);
}

TEST(ByteSizeTest, As_DecimalMode) {
    byte_size bs(2000ULL);
    auto val = bs.as(byte_size::unit::KB, false);
    EXPECT_DOUBLE_EQ(static_cast<double>(val), 2.0);
}

TEST(ByteSizeTest, As_AutoUnitThrows) {
    byte_size bs(100ULL);
    EXPECT_THROW(ignore = bs.as(byte_size::unit::AUTO, true), value_exception);
}

TEST(ByteSizeTest, ToStringDefault_Zero) {
    byte_size bs;
    EXPECT_EQ(bs.to_string(), "0.00 B");
}

TEST(ByteSizeTest, ToStringDefault_AutoUnit) {
    byte_size bs(1024ULL);
    EXPECT_EQ(bs.to_string(), "1.00 KiB");
}

TEST(ByteSizeTest, ToStringDefault_LargeValue) {
    byte_size bs(3ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(bs.to_string(), "3.00 GiB");
}

TEST(ByteSizeTest, ToString_SpecificUnit) {
    byte_size bs(1536ULL);
    EXPECT_EQ(bs.to_string(byte_size::unit::B, 0, true), "1536 B");
    EXPECT_EQ(bs.to_string(byte_size::unit::KB, 2, true), "1.50 KiB");
}

TEST(ByteSizeTest, ToString_DecimalMode) {
    byte_size bs(1500ULL);
    EXPECT_EQ(bs.to_string(byte_size::unit::KB, 1, false), "1.5 kB");
}

TEST(ByteSizeTest, ToString_Precision) {
    byte_size bs(1025ULL);
    EXPECT_EQ(bs.to_string(byte_size::unit::KB, 4, true), "1.0010 KiB");
}

TEST(ByteSizeTest, ToString_MaxValueAuto) {
    byte_size bs(numeric_traits<uint64_t>::max());
    auto str = bs.to_string();
    EXPECT_FALSE(str.empty());
}

TEST(ByteSizeTest, Addition) {
    byte_size a(1000ULL);
    byte_size b(24ULL);
    byte_size c = a + b;
    EXPECT_EQ(c.bytes(), 1024ULL);
}

TEST(ByteSizeTest, Subtraction) {
    byte_size a(2048ULL);
    byte_size b(1024ULL);
    byte_size c = a - b;
    EXPECT_EQ(c.bytes(), 1024ULL);
}

TEST(ByteSizeTest, SubtractionUnderflowThrows) {
    byte_size a(500ULL);
    byte_size b(1000ULL);
    EXPECT_THROW(ignore = a - b, value_exception);
}

TEST(ByteSizeTest, Multiplication) {
    byte_size a(512ULL);
    byte_size c = a * 2;
    EXPECT_EQ(c.bytes(), 1024ULL);
}

TEST(ByteSizeTest, MultiplicationOverflowThrows) {
    byte_size a(numeric_traits<uint64_t>::max());
    EXPECT_THROW(ignore = a * 2, value_exception);
}

TEST(ByteSizeTest, Division) {
    byte_size a(2048ULL);
    byte_size c = a / 2;
    EXPECT_EQ(c.bytes(), 1024ULL);
}

TEST(ByteSizeTest, DivisionByZeroThrows) {
    byte_size a(100ULL);
    EXPECT_THROW(ignore = a / 0, value_exception);
}

TEST(ByteSizeTest, FriendMultiplication) {
    byte_size a(512ULL);
    byte_size c = 3 * a;
    EXPECT_EQ(c.bytes(), 1536ULL);
}

TEST(ByteSizeTest, CompoundAddition) {
    byte_size a(1000ULL);
    a += byte_size(24ULL);
    EXPECT_EQ(a.bytes(), 1024ULL);
}

TEST(ByteSizeTest, CompoundSubtraction) {
    byte_size a(2048ULL);
    a -= byte_size(1024ULL);
    EXPECT_EQ(a.bytes(), 1024ULL);
}

TEST(ByteSizeTest, CompoundSubtractionUnderflowThrows) {
    byte_size a(500ULL);
    EXPECT_THROW(a -= byte_size(1000ULL), value_exception);
}

TEST(ByteSizeTest, CompoundMultiplication) {
    byte_size a(512ULL);
    a *= 3;
    EXPECT_EQ(a.bytes(), 1536ULL);
}

TEST(ByteSizeTest, CompoundDivision) {
    byte_size a(2048ULL);
    a /= 4;
    EXPECT_EQ(a.bytes(), 512ULL);
}

TEST(ByteSizeTest, CompoundDivisionByZeroThrows) {
    byte_size a(100ULL);
    EXPECT_THROW(a /= 0, value_exception);
}

TEST(ByteSizeTest, Equality) {
    byte_size a(1024ULL);
    byte_size b(1024ULL);
    byte_size c(2048ULL);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(ByteSizeTest, LessThan) {
    byte_size a(100ULL);
    byte_size b(200ULL);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(ByteSizeTest, IsZero) {
    EXPECT_TRUE(byte_size().is_zero());
    EXPECT_TRUE(byte_size(0ULL).is_zero());
    EXPECT_FALSE(byte_size(1ULL).is_zero());
}

TEST(ByteSizeTest, ToHash) {
    byte_size a(12345ULL);
    byte_size b(12345ULL);
    EXPECT_EQ(a.to_hash(), b.to_hash());
    EXPECT_NE(a.to_hash(), byte_size(54321ULL).to_hash());
}

TEST(ByteSizeTest, UnpackageTrait) {
    bool is_same = is_same_v<unpackage<byte_size>::type, uint64_t>;
    EXPECT_TRUE(is_same);
}

TEST(ByteSizeLiterals, ByteLiteral) {
    auto bs = 1024_B;
    EXPECT_EQ(bs.bytes(), 1024ULL);
}

TEST(ByteSizeLiterals, ByteDecimalLiteral) {
    auto val = 1024.0_B;
    EXPECT_EQ(val.bytes(), 1024ULL);
}

TEST(ByteSizeLiterals, KBLiterals) {
    EXPECT_EQ((1_KB).bytes(), 1024ULL);
    EXPECT_EQ((1.5_KB).bytes(), 1536ULL);
}

TEST(ByteSizeLiterals, MBLiterals) {
    EXPECT_EQ((1_MB).bytes(), 1024ULL * 1024);
    EXPECT_EQ((2.0_MB).bytes(), 2 * 1024 * 1024);
}

TEST(ByteSizeLiterals, GBLiterals) {
    EXPECT_EQ((1_GB).bytes(), 1024ULL * 1024 * 1024);
    EXPECT_EQ((0.5_GB).bytes(), 536870912ULL);
}

TEST(ByteSizeLiterals, TBLiterals) { EXPECT_EQ((1_TB).bytes(), 1024ULL * 1024 * 1024 * 1024); }

TEST(ByteSizeLiterals, PBLiterals) { EXPECT_EQ((1_PB).bytes(), 1024ULL * 1024 * 1024 * 1024 * 1024); }

TEST(ByteSizeLiterals, EBLiterals) { EXPECT_EQ((1_EB).bytes(), 1024ULL * 1024 * 1024 * 1024 * 1024 * 1024); }

TEST(ByteSizeTest, MaxUint64RoundTrip) {
    byte_size bs(numeric_traits<uint64_t>::max());
    EXPECT_EQ(bs.bytes(), numeric_traits<uint64_t>::max());
}

TEST(ByteSizeTest, AsLargeValuePrecision) {
    byte_size bs(numeric_traits<uint64_t>::max());
    auto val = bs.as(byte_size::unit::EB, true);
    EXPECT_GT(val, 15.9L);
    EXPECT_LE(val, 16.0L);
}

TEST(ByteSizeTest, FromValueUnit_NearMaxUint64) {
    constexpr uint64_t max_val = numeric_traits<uint64_t>::max();
    constexpr uint64_t base = 1024;
    constexpr uint64_t quotient = max_val / base;
    constexpr uint64_t expected = quotient * base;
    byte_size bs(static_cast<uint64_t>(expected));
    EXPECT_EQ(bs.bytes(), expected);
}

TEST(UuidTest, DefaultConstructor_CreatesNilUuid) {
    uuid u;
    EXPECT_EQ(u.version(), 0);
    for (auto b: u.bytes()) {
        EXPECT_EQ(b, 0);
    }
    EXPECT_EQ(u.to_string(), "00000000-0000-0000-0000-000000000000");
}

TEST(UuidTest, ByteArrayConstructor_CopiesCorrectly) {
    const byte_t raw[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                            0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uuid u{memory_view<const byte_t, 16>(raw)};
    const auto view = u.bytes();
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(view[i], raw[i]);
    }
}

TEST(UuidTest, StringConstructor_36Chars_Valid) {
    uuid u("00112233-4455-6677-8899-aabbccddeeff");
    EXPECT_EQ(u.to_string(), "00112233-4455-6677-8899-aabbccddeeff");
    EXPECT_EQ(u.bytes()[0], 0x00);
    EXPECT_EQ(u.bytes()[15], 0xFF);
}

TEST(UuidTest, StringConstructor_32Chars_Valid) {
    uuid u("00112233445566778899aabbccddeeff");
    EXPECT_EQ(u.to_string(), "00112233-4455-6677-8899-aabbccddeeff");
}

TEST(UuidTest, StringConstructor_InvalidFormat_Throws) {
    EXPECT_THROW(uuid(""), value_exception);
    EXPECT_THROW(uuid("short"), value_exception);
    EXPECT_THROW(uuid("too-long-string-xxxxxxxxxxxxxx"), value_exception);
    EXPECT_THROW(uuid("gggggggg-gggg-gggg-gggg-gggggggggggg"), value_exception);
    EXPECT_THROW(uuid("00112233-4455-6677-8899-aabbccddeeff00"), value_exception);
    EXPECT_THROW(uuid("00112233-4455-6677-8899-zabbccddeeff"), value_exception);
}

TEST(UuidTest, GenerateV4_SetsVersionAndVariant) {
    uuid u;
    u.generate_v4();
    EXPECT_EQ(u.version(), 4);
    EXPECT_TRUE(u.is_v4());
    EXPECT_FALSE(u.is_v7());
    const auto& b = u.bytes();
    EXPECT_EQ((b[8] >> 6) & 0x3, 0x2);
    EXPECT_NE(u.to_hash(), 0);
}

TEST(UuidTest, GenerateV7_SetsVersionAndVariant) {
    uuid u;
    u.generate_v7();
    EXPECT_EQ(u.version(), 7);
    EXPECT_TRUE(u.is_v7());
    EXPECT_FALSE(u.is_v4());
    const auto& b = u.bytes();
    EXPECT_EQ((b[8] >> 6) & 0x3, 0x2);
}

TEST(UuidTest, GenerateV7_HasValidTimestamp) {
    uuid u;
    u.generate_v7();
    auto ts = u.timestamp_v7();
    ASSERT_TRUE(ts.has_value());
    EXPECT_GT(*ts, 0u);
    uint64_t expected_ts = 0;
    for (int i = 0; i < 6; ++i) {
        expected_ts = (expected_ts << 8) | u.bytes()[i];
    }
    EXPECT_EQ(*ts, expected_ts);
}

TEST(UuidTest, TimestampV7_OnNonV7_ReturnsNull) {
    uuid u;
    u.generate_v4();
    EXPECT_FALSE(u.timestamp_v7().has_value());

    uuid nil;
    EXPECT_FALSE(nil.timestamp_v7().has_value());
}

TEST(UuidTest, StaticV4_ReturnsValidV4) {
    uuid u = uuid::v4();
    EXPECT_EQ(u.version(), 4);
    EXPECT_TRUE(u.is_v4());
}

TEST(UuidTest, StaticV7_ReturnsValidV7) {
    uuid u = uuid::v7();
    EXPECT_EQ(u.version(), 7);
    EXPECT_TRUE(u.is_v7());
}

TEST(UuidTest, ToString_FormatCorrect) {
    uuid u("01234567-89ab-cdef-0123-456789abcdef");
    string s = u.to_string();
    EXPECT_EQ(s.length(), 36u);
    EXPECT_EQ(s[8], '-');
    EXPECT_EQ(s[13], '-');
    EXPECT_EQ(s[18], '-');
    EXPECT_EQ(s[23], '-');
    EXPECT_EQ(s, "01234567-89ab-cdef-0123-456789abcdef");
}

TEST(UuidTest, Bytes_ReturnsCorrectView) {
    byte_t raw[16] = {0};
    for (int i = 0; i < 16; ++i) {
        raw[i] = static_cast<byte_t>(i);
    }
    uuid id{memory_view<const byte_t, 16>(raw)};
    auto view = id.bytes();
    ASSERT_EQ(view.size(), 16u);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(view[i], raw[i]);
    }
}

TEST(UuidTest, BeginEnd_IteratesCorrectly) {
    uuid u("00112233-4455-6677-8899-aabbccddeeff");
    size_t count = 0;
    for (auto b: u) {
        (void) b;
        ++count;
    }
    EXPECT_EQ(count, 16u);
    EXPECT_EQ(*u.begin(), *u.bytes().begin());
}

TEST(UuidTest, ToHash_Consistent) {
    uuid u("00112233-4455-6677-8899-aabbccddeeff");
    size_t h1 = u.to_hash();
    size_t h2 = u.to_hash();
    EXPECT_EQ(h1, h2);

    uuid u2("10112233-4455-6677-8899-aabbccddeeff");
    EXPECT_NE(u.to_hash(), u2.to_hash());
}

TEST(UuidTest, UserLiteral_Valid) {
    auto u = "00112233-4455-6677-8899-aabbccddeeff"_uuid;
    EXPECT_EQ(u.to_string(), "00112233-4455-6677-8899-aabbccddeeff");
}

TEST(UuidTest, UserLiteral_Invalid_Throws) { EXPECT_THROW(ignore = "invalid"_uuid, value_exception); }

TEST(UuidTest, Equality_And_Hashing) {
    uuid a("11111111-1111-1111-1111-111111111111");
    uuid b("11111111-1111-1111-1111-111111111111");
    uuid c("22222222-2222-2222-2222-222222222222");

    EXPECT_EQ(a.to_hash(), b.to_hash());
    EXPECT_NE(a.to_hash(), c.to_hash());
    EXPECT_TRUE(equal(a.begin(), a.end(), b.begin()));
    EXPECT_FALSE(equal(a.begin(), a.end(), c.begin()));
}

TEST(UuidTest, CopyAndAssignment) {
    uuid a = uuid::v4();
    uuid b(a);
    EXPECT_EQ(a.to_hash(), b.to_hash());
    EXPECT_EQ(a.to_string(), b.to_string());

    uuid c;
    c = a;
    EXPECT_EQ(a.to_hash(), c.to_hash());
}

TEST(PairTypeTraits, FirstSecondType) {
    EXPECT_TRUE((is_same_v<pair<int, double>::first_type, int>) );
    EXPECT_TRUE((is_same_v<pair<int, double>::second_type, double>) );
}

TEST(PairTypeTraits, TupleSize) {
    EXPECT_EQ((tuple_size<pair<int, double>>::value), 2u);
    EXPECT_EQ((tuple_size<pair<tracked, tracked>>::value), 2u);
#ifdef NEFORCE_STANDARD_14
    EXPECT_EQ((tuple_size_v<pair<int, double>>), 2u);
#endif
}

TEST(PairTypeTraits, TupleElement) {
    EXPECT_TRUE((is_same_v<tuple_element_t<0, pair<int, double>>, int>) );
    EXPECT_TRUE((is_same_v<tuple_element_t<1, pair<int, double>>, double>) );
    EXPECT_TRUE((is_same_v<tuple_extract_base_t<0, int, double>, tuple<int, double>>) );
}

TEST(PairTypeTraits, StdTupleSizeAndElement) {
#ifdef NEFORCE_STANDARD_17
    EXPECT_EQ((tuple_size<pair<int, double>>::value), 2u);
    EXPECT_TRUE((is_same_v<tuple_element_t<0, pair<int, double>>, int>) );
    EXPECT_TRUE((is_same_v<tuple_element_t<1, pair<int, double>>, double>) );
#endif
}

TEST(PairConstruction, DefaultConstructorImplicit) {
    pair<implicit_default, implicit_default> p;
    (void) p;
}

TEST(PairConstruction, DefaultConstructorExplicit) {
    static_assert(!is_convertible_v<tuple<>, pair<explicit_default, explicit_default>>);
    pair<explicit_default, explicit_default> p;
    (void) p;
}

TEST(PairConstruction, ValueConstructorImplicit) {
    pair<int, double> p(1, 2.0);
    EXPECT_EQ(p.first, 1);
    EXPECT_DOUBLE_EQ(p.second, 2.0);
    pair<implicit_from_int, implicit_from_int> p2 = {3, 4};
    EXPECT_EQ(p2.first.val, 3);
    EXPECT_EQ(p2.second.val, 4);
}

TEST(PairConstruction, ValueConstructorExplicit) {
    static_assert(!is_convertible_v<int, explicit_from_int>);
    pair<explicit_from_int, explicit_from_int> p{1, 2};
    EXPECT_EQ(p.first.val, 1);
    EXPECT_EQ(p.second.val, 2);
}

TEST(PairConstruction, ValueConstructorPerfectForwarding) {
    string s = "hello";
    pair<string, string> p(move(s), "world");
    EXPECT_EQ(p.first, "hello");
    EXPECT_EQ(p.second, "world");
    EXPECT_TRUE(s.empty());
}

TEST(PairConstruction, CopyConstructorSameType) {
    pair<int, int> p1(10, 20);
    pair<int, int> p2(p1);
    EXPECT_EQ(p2.first, 10);
    EXPECT_EQ(p2.second, 20);
}

TEST(PairConstruction, MoveConstructorSameType) {
    pair<int, int> p1(10, 20);
    pair<int, int> p2(move(p1));
    EXPECT_EQ(p2.first, 10);
    EXPECT_EQ(p2.second, 20);
}

TEST(PairConstruction, CopyConstructorDifferentTypeConvertible) {
    pair<int, double> p1(1, 2.5);
    pair<double, int> p2(p1);
    pair<long, float> p3(p1);
    EXPECT_EQ(p3.first, 1L);
    EXPECT_FLOAT_EQ(p3.second, 2.5f);
}

TEST(PairConstruction, CopyConstructorExplicitWhenNarrowing) {
    pair<int, double> p1(1, 2.5);
    pair<char, int> p2(p1);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);
}

TEST(PairConstruction, MoveConstructorDifferentType) {
    pair<string, string> p1("a", "b");
    pair<string, string> p2(move(p1));
    EXPECT_EQ(p2.first, "a");
    EXPECT_EQ(p2.second, "b");
    EXPECT_TRUE(p1.first.empty());
}

TEST(PairAssignment, CopyAssignmentSameType) {
    pair<int, int> p1(1, 2);
    pair<int, int> p2(3, 4);
    p2 = p1;
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);
}

TEST(PairAssignment, MoveAssignmentSameType) {
    pair<int, int> p1(1, 2);
    pair<int, int> p2(3, 4);
    p2 = move(p1);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);
}

TEST(PairAssignment, CopyAssignmentDifferentType) {
    pair<int, double> p1(1, 2.5);
    pair<long, float> p2(9L, 9.9f);
    p2 = p1;
    EXPECT_EQ(p2.first, 1L);
    EXPECT_FLOAT_EQ(p2.second, 2.5f);
}

TEST(PairAssignment, MoveAssignmentDifferentType) {
    pair<string, string> p1("x", "y");
    pair<string, string> p2("a", "b");
    p2 = move(p1);
    EXPECT_EQ(p2.first, "x");
    EXPECT_EQ(p2.second, "y");
}

TEST(PairAssignment, VolatileCopyAssignmentDeleted) {
    using VP = pair<int, int>;
    static_assert(!is_assignable_v<volatile VP&, const volatile VP&>);
}

TEST(PairComparison, Equal) {
    pair<int, int> a(1, 2), b(1, 2), c(1, 3);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    pair<string, int> s1("hi", 1), s2("hi", 1), s3("hi", 2);
    EXPECT_EQ(s1, s2);
    EXPECT_NE(s1, s3);
}

TEST(PairComparison, Less) {
    pair<int, int> a(1, 2), b(1, 3), c(2, 0);
    EXPECT_LT(a, b);
    EXPECT_LT(a, c);
    EXPECT_FALSE(b < a);
    pair<string, int> s1("a", 5), s2("b", 1), s3("a", 10);
    EXPECT_LT(s1, s2);
    EXPECT_LT(s1, s3);
}

TEST(PairHash, ToHash) {
    pair<int, int> p(1, 2);
    size_t h = p.to_hash();
    size_t expected = hash<int>()(1) ^ hash<int>()(2);
    EXPECT_EQ(h, expected);
    pair<string, int> ps("hello", 42);
    size_t hs = ps.to_hash();
    size_t exp_s = hash<string>()("hello") ^ hash<int>()(42);
    EXPECT_EQ(hs, exp_s);
}

TEST(PairSwap, MemberSwap) {
    pair<int, int> a(1, 2), b(3, 4);
    a.swap(b);
    EXPECT_EQ(a.first, 3);
    EXPECT_EQ(a.second, 4);
    EXPECT_EQ(b.first, 1);
    EXPECT_EQ(b.second, 2);
    EXPECT_TRUE(noexcept(a.swap(b)));
}

TEST(MakePair, Basic) {
    auto p = make_pair(1, 2.0);
    EXPECT_TRUE((is_same_v<decltype(p), pair<int, double>>) );
    EXPECT_EQ(p.first, 1);
    EXPECT_DOUBLE_EQ(p.second, 2.0);
    string s = "hello";
    auto p2 = make_pair(move(s), 42);
    EXPECT_TRUE((is_same_v<decltype(p2), pair<string, int>>) );
    EXPECT_EQ(p2.first, "hello");
    EXPECT_EQ(p2.second, 42);
    EXPECT_TRUE(s.empty());
}

TEST(MakePair, ReferenceWrapper) {
    int a = 10, b = 20;
    auto p = make_pair(ref(a), ref(b));
    EXPECT_TRUE((is_same_v<decltype(p), pair<int&, int&>>) );
    p.first = 100;
    EXPECT_EQ(a, 100);
    p.second = 200;
    EXPECT_EQ(b, 200);
}

TEST(PairGet, IndexNonConstLvalue) {
    pair<int, int> p(1, 2);
    int& a = get<0>(p);
    int& b = get<1>(p);
    a = 10;
    b = 20;
    EXPECT_EQ(p.first, 10);
    EXPECT_EQ(p.second, 20);
}

TEST(PairGet, IndexConstLvalue) {
    const pair<int, int> p(1, 2);
    const int& a = get<0>(p);
    const int& b = get<1>(p);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
}

TEST(PairGet, IndexRvalue) {
    pair<int, int> p(1, 2);
    int&& a = get<0>(move(p));
    int&& b = get<1>(move(p));
    a = 10;
    b = 20;
    EXPECT_EQ(a, 10);
    EXPECT_EQ(b, 20);
}

TEST(PairGet, IndexConstRvalue) {
    const pair<int, int> p(1, 2);
    const int&& a = get<0>(move(p));
    const int&& b = get<1>(move(p));
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
}

TEST(PairGet, TypeNonConstLvalue) {
    pair<int, double> p(1, 2.5);
    int& a = get<int>(p);
    double& b = get<double>(p);
    a = 10;
    b = 3.14;
    EXPECT_EQ(p.first, 10);
    EXPECT_DOUBLE_EQ(p.second, 3.14);
}

TEST(PairGet, TypeConstLvalue) {
    const pair<int, double> p(1, 2.5);
    const int& a = get<int>(p);
    const double& b = get<double>(p);
    EXPECT_EQ(a, 1);
    EXPECT_DOUBLE_EQ(b, 2.5);
}

TEST(PairGet, TypeRvalue) {
    pair<string, string> p("hello", "world");
    string&& a = get<0>(move(p));
    EXPECT_EQ(a, "hello");
    string&& b = get<1>(move(p));
    EXPECT_EQ(b, "world");
    a.clear();
    b.clear();
}

TEST(PairGet, TypeConstRvalue) {
    const pair<string, int> p("hi", 7);
    const string&& a = get<string>(move(p));
    const int&& b = get<int>(move(p));
    EXPECT_EQ(a, "hi");
    EXPECT_EQ(b, 7);
}

#ifdef NEFORCE_STANDARD_17
TEST(PairStructuredBinding, Basic) {
    pair<int, double> p(42, 3.14);
    auto& [a, b] = p;
    EXPECT_EQ(a, 42);
    EXPECT_DOUBLE_EQ(b, 3.14);
    a = 100;
    EXPECT_EQ(p.first, 100);
}
#endif

TEST(PairTupleAlias, TupleElementT) {
    EXPECT_TRUE((is_same_v<tuple_element_t<0, pair<int, double>>, int>) );
    EXPECT_TRUE((is_same_v<tuple_element_t<1, pair<int, double>>, double>) );
}

TEST(TupleEmpty, DefaultConstruction) {
    tuple<> t;
    (void) t;
}

TEST(TupleEmpty, CopyAndMove) {
    tuple<> t1, t2(t1);
    tuple<> t3(move(t1));
    t2 = t1;
    t3 = move(t1);
}

TEST(TupleEmpty, ExactArgConstructTag) {
    tuple<> t(exact_arg_construct_tag{});
    (void) t;
}

TEST(TupleEmpty, Comparison) {
    tuple<> a, b;
    EXPECT_TRUE(a.equal_to(b));
    EXPECT_FALSE(a.less_than(b));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a < b);
}

TEST(TupleEmpty, Hash) {
    tuple<> t;
    EXPECT_EQ(t.to_hash(), constants::FNV_OFFSET_BASIS);
}

TEST(TupleEmpty, Swap) {
    tuple<> a, b;
    a.swap(b);
    b.swap(a);
}

TEST(TupleMain, DefaultConstruction) {
    tuple<int, double> t1;
    EXPECT_EQ(get<0>(t1), 0);
    EXPECT_DOUBLE_EQ(get<1>(t1), 0.0);
    tuple<explicit_default, int> t2;
    (void) t2;
    static_assert(!is_default_constructible_v<tuple<no_default, int>>);
}

TEST(TupleMain, ValueConstructor) {
    tuple<int, string> t1(42, "hello");
    EXPECT_EQ(get<0>(t1), 42);
    EXPECT_EQ(get<1>(t1), "hello");

    tuple<implicit_from_int, double> t2(10, 2.5);
    EXPECT_EQ(get<0>(t2).val, 10);

    static_assert(!is_convertible_v<int, explicit_from_int>);
    tuple<explicit_from_int, int> t3{5, 10};
    EXPECT_EQ(get<0>(t3).val, 5);
}

TEST(TupleMain, PerfectForwardingConstructor) {
    string s = "world";
    tuple<int, string> t(10, move(s));
    EXPECT_EQ(get<1>(t), "world");
    EXPECT_TRUE(s.empty());
}

TEST(TupleMain, CopyConstructorFromSameType) {
    tuple<int, double> a(1, 3.14);
    tuple<int, double> b(a);
    EXPECT_EQ(get<0>(b), 1);
    EXPECT_DOUBLE_EQ(get<1>(b), 3.14);
}

TEST(TupleMain, MoveConstructorFromSameType) {
    tuple<int, double> a(1, 3.14);
    tuple<int, double> b(move(a));
    EXPECT_EQ(get<0>(b), 1);
    EXPECT_DOUBLE_EQ(get<1>(b), 3.14);
}

TEST(TupleMain, CopyConstructorFromDifferentTuple) {
    tuple<int, double> a(1, 3.14);
    tuple<long, float> b(a);
    EXPECT_EQ(get<0>(b), 1L);
    EXPECT_FLOAT_EQ(get<1>(b), 3.14f);
}

TEST(TupleMain, MoveConstructorFromDifferentTuple) {
    tuple<string, int> a("hi", 5);
    tuple<string, int> b(move(a));
    EXPECT_EQ(get<0>(b), "hi");
    EXPECT_EQ(get<1>(b), 5);
}

TEST(TupleMain, ConstructorFromPair) {
    pair<int, double> p(7, 8.8);
    tuple<int, double> t1(p);
    EXPECT_EQ(get<0>(t1), 7);
    EXPECT_DOUBLE_EQ(get<1>(t1), 8.8);

    pair<string, int> p2("a", 1);
    tuple<string, int> t2(move(p2));
    EXPECT_EQ(get<0>(t2), "a");
    EXPECT_EQ(get<1>(t2), 1);
}

TEST(TupleMain, CopyAssignmentSameType) {
    tuple<int, double> a(1, 2.0), b(3, 4.0);
    b = a;
    EXPECT_EQ(get<0>(b), 1);
    EXPECT_DOUBLE_EQ(get<1>(b), 2.0);
}

TEST(TupleMain, MoveAssignmentSameType) {
    tuple<int, double> a(1, 2.0), b(3, 4.0);
    b = move(a);
    EXPECT_EQ(get<0>(b), 1);
    EXPECT_DOUBLE_EQ(get<1>(b), 2.0);
}

TEST(TupleMain, CopyAssignmentFromDifferentTuple) {
    tuple<int, double> a(1, 2.5);
    tuple<long, float> b(0L, 0.0f);
    b = a;
    EXPECT_EQ(get<0>(b), 1L);
    EXPECT_FLOAT_EQ(get<1>(b), 2.5f);
}

TEST(TupleMain, MoveAssignmentFromDifferentTuple) {
    tuple<string, int> a("x", 9);
    tuple<string, int> b("y", 0);
    b = move(a);
    EXPECT_EQ(get<0>(b), "x");
    EXPECT_EQ(get<1>(b), 9);
}

TEST(TupleMain, AssignmentFromPair) {
    pair<int, double> p(10, 20.5);
    tuple<int, double> t(0, 0.0);
    t = p;
    EXPECT_EQ(get<0>(t), 10);
    EXPECT_DOUBLE_EQ(get<1>(t), 20.5);

    pair<string, int> p2("hello", 42);
    tuple<string, int> t2("", 0);
    t2 = move(p2);
    EXPECT_EQ(get<0>(t2), "hello");
    EXPECT_EQ(get<1>(t2), 42);
}

TEST(TupleMain, VolatileCopyAssignmentDeleted) {
    using T = tuple<int, double>;
    static_assert(!is_assignable_v<volatile T&, const volatile T&>);
}

TEST(TupleMain, Comparison) {
    tuple<int, string> a(1, "a"), b(1, "a"), c(2, "a"), d(1, "b");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(a < d);
    EXPECT_FALSE(c < a);
    EXPECT_TRUE(a.less_than(c));
}

TEST(TupleMain, Swap) {
    tuple<int, double> a(1, 1.5), b(2, 3.5);
    a.swap(b);
    EXPECT_EQ(get<0>(a), 2);
    EXPECT_DOUBLE_EQ(get<1>(a), 3.5);
    EXPECT_EQ(get<0>(b), 1);
    EXPECT_DOUBLE_EQ(get<1>(b), 1.5);
}

TEST(TupleMain, GetRest) {
    tuple<int, double, string> t(1, 2.5, "hi");
    auto& rest = t.get_rest();
    EXPECT_EQ(get<0>(rest), 2.5);
    EXPECT_EQ(get<1>(rest), "hi");
    const auto& crest = as_const(t).get_rest();
    EXPECT_EQ(get<0>(crest), 2.5);
}

TEST(TupleMain, Hash) {
    tuple<int, string> t(10, "abc");
    size_t h = t.to_hash();
    size_t expected = hash<int>()(10) ^ hash<string>()("abc");
    EXPECT_EQ(h, expected);

    tuple<> empty_t;
    EXPECT_EQ(empty_t.to_hash(), constants::FNV_OFFSET_BASIS);
}

TEST(TupleGet, Lvalue) {
    tuple<int, double> t(10, 3.14);
    int& a = get<0>(t);
    double& b = get<1>(t);
    a = 100;
    b = 2.71;
    EXPECT_EQ(get<0>(t), 100);
    EXPECT_DOUBLE_EQ(get<1>(t), 2.71);
}

TEST(TupleGet, ConstLvalue) {
    const tuple<int, double> t(10, 3.14);
    const int& a = get<0>(t);
    const double& b = get<1>(t);
    EXPECT_EQ(a, 10);
    EXPECT_DOUBLE_EQ(b, 3.14);
}

TEST(TupleGet, Rvalue) {
    tuple<int, double> t(10, 3.14);
    int&& a = get<0>(move(t));
    double&& b = get<1>(move(t));
    a = 20;
    b = 2.0;
    EXPECT_EQ(a, 20);
    EXPECT_DOUBLE_EQ(b, 2.0);
}

TEST(TupleGet, ConstRvalue) {
    const tuple<int, double> t(30, 6.28);
    const int&& a = get<0>(move(t));
    const double&& b = get<1>(move(t));
    EXPECT_EQ(a, 30);
    EXPECT_DOUBLE_EQ(b, 6.28);
}

TEST(MakeTuple, Basic) {
    auto t = make_tuple(1, 2.5, string("hi"));
    EXPECT_TRUE((is_same_v<decltype(t), tuple<int, double, string>>) );
    EXPECT_EQ(get<0>(t), 1);
    EXPECT_DOUBLE_EQ(get<1>(t), 2.5);
    EXPECT_EQ(get<2>(t), "hi");
}

TEST(MakeTuple, UnwrapReferenceWrapper) {
    int a = 10, b = 20;
    auto t = make_tuple(ref(a), ref(b));
    EXPECT_TRUE((is_same_v<decltype(t), tuple<int&, int&>>) );
    get<0>(t) = 100;
    get<1>(t) = 200;
    EXPECT_EQ(a, 100);
    EXPECT_EQ(b, 200);
}

TEST(MakeTuple, PerfectForwarding) {
    string s = "bye";
    auto t = make_tuple(move(s));
    EXPECT_TRUE((is_same_v<decltype(t), tuple<string>>) );
    EXPECT_EQ(get<0>(t), "bye");
    EXPECT_TRUE(s.empty());
}

TEST(Tie, Basic) {
    int a = 0;
    double b = 0.0;
    auto t = tie(a, b);
    get<0>(t) = 42;
    get<1>(t) = 3.14;
    EXPECT_EQ(a, 42);
    EXPECT_DOUBLE_EQ(b, 3.14);
    EXPECT_TRUE((is_same_v<decltype(t), tuple<int&, double&>>) );
}

TEST(ForwardAsTuple, Basic) {
    int x = 5;
    string str = "tmp";
    auto t = forward_as_tuple(x, str);
    static_assert(is_same_v<decltype(t), tuple<int&, string&>>);
    get<0>(t) = 10;
    EXPECT_EQ(x, 10);
    string s = get<1>(t);
    EXPECT_EQ(s, "tmp");
}

TEST(TupleCat, Basic) {
    tuple<int, double> t1(1, 2.5);
    tuple<string> t2("hello");
    tuple<char> t3('x');
    auto res = tuple_cat(t1, t2, t3);
    EXPECT_TRUE((is_same_v<decltype(res), tuple<int, double, string, char>>) );
    EXPECT_EQ(get<0>(res), 1);
    EXPECT_DOUBLE_EQ(get<1>(res), 2.5);
    EXPECT_EQ(get<2>(res), "hello");
    EXPECT_EQ(get<3>(res), 'x');
}

TEST(TupleCat, WithEmptyTuples) {
    tuple<> e1, e2;
    tuple<int> t(42);
    auto r1 = tuple_cat(e1, t, e2);
    EXPECT_EQ(get<0>(r1), 42);
    auto r2 = tuple_cat(e1, e2);
    EXPECT_TRUE((is_same_v<decltype(r2), tuple<>>) );
}

TEST(TupleCat, Moving) {
    tuple<string> t("data");
    auto res = tuple_cat(move(t));
    EXPECT_EQ(get<0>(res), "data");
}

TEST(MakeFromTuple, Basic) {
    tuple<int, double, string> t(1, 2.5, "test");
    auto obj = make_from_tuple<from_tuple>(t);
    EXPECT_EQ(obj.a, 1);
    EXPECT_DOUBLE_EQ(obj.b, 2.5);
    EXPECT_EQ(obj.c, "test");
}

TEST(MakeFromTuple, MoveTuple) {
    tuple<int, double, string> t(1, 2.5, "test");
    auto obj = make_from_tuple<from_tuple>(move(t));
    EXPECT_EQ(obj.a, 1);
    EXPECT_DOUBLE_EQ(obj.b, 2.5);
    EXPECT_EQ(obj.c, "test");
}

TEST(TupleTraits, TupleSize) {
    EXPECT_EQ((tuple_size<tuple<>>::value), 0u);
    EXPECT_EQ((tuple_size<tuple<int, double, string>>::value), 3u);
#ifdef NEFORCE_STANDARD_14
    EXPECT_EQ((tuple_size_v<tuple<int, double>>), 2u);
#endif
}

TEST(TupleTraits, TupleElement) {
    EXPECT_TRUE((is_same_v<tuple_element_t<0, tuple<int, double, string>>, int>) );
    EXPECT_TRUE((is_same_v<tuple_element_t<1, tuple<int, double, string>>, double>) );
    EXPECT_TRUE((is_same_v<tuple_element_t<2, tuple<int, double, string>>, string>) );
    using ex_base = tuple_extract_base_t<1, int, double, string>;
    EXPECT_TRUE((is_same_v<ex_base, tuple<double, string>>) );
}

#ifdef NEFORCE_STANDARD_17
TEST(TupleTraits, StdCompatibility) {
    EXPECT_EQ((tuple_size<tuple<int, double>>::value), 2u);
    EXPECT_TRUE((is_same_v<tuple_element_t<0, tuple<int, double>>, int>) );
}
#endif

#ifdef NEFORCE_STANDARD_17
TEST(TupleStructuredBindings, Basic) {
    tuple<int, double, string> t(1, 2.5, "abc");
    auto& [a, b, c] = t;
    EXPECT_EQ(a, 1);
    EXPECT_DOUBLE_EQ(b, 2.5);
    EXPECT_EQ(c, "abc");
    a = 10;
    EXPECT_EQ(get<0>(t), 10);
}
#endif

TEST(ReferenceWrapper, ConstructFromLvalue) {
    int x = 42;
    reference_wrapper<int> r(x);
    EXPECT_EQ(r.get(), 42);
    EXPECT_EQ(&r.get(), &x);
}

TEST(ReferenceWrapper, ConstructFromConstLvalue) {
    const int x = 100;
    reference_wrapper<const int> r(x);
    EXPECT_EQ(r.get(), 100);
    EXPECT_EQ(&r.get(), &x);
}

TEST(ReferenceWrapper, ConstructFromDerivedRef) {
    derived d;
    reference_wrapper<base> r(d);
    EXPECT_EQ(&r.get(), &d);
}

TEST(ReferenceWrapper, NoConstructFromRvalue) {
    static_assert(!is_constructible_v<reference_wrapper<int>, int&&>);
    static_assert(!is_constructible_v<reference_wrapper<const int>, const int&&>);
}

TEST(ReferenceWrapper, CopyConstructible) {
    int v = 5;
    reference_wrapper<int> r1(v);
    reference_wrapper<int> r2(r1);
    EXPECT_EQ(&r2.get(), &v);
}

TEST(ReferenceWrapper, NoConstructFromSameReferenceWrapperViaTemplate) {
    static_assert(is_constructible_v<reference_wrapper<int>, reference_wrapper<int>>);
    static_assert(is_copy_constructible_v<reference_wrapper<int>>);
}

TEST(ReferenceWrapper, GetReturnsReference) {
    int x = 7;
    reference_wrapper<int> r(x);
    int& vr = r.get();
    vr = 10;
    EXPECT_EQ(x, 10);
}

TEST(ReferenceWrapper, ImplicitConversion) {
    int x = 20;
    reference_wrapper<int> r(x);
    int& ref = r;
    ref = 30;
    EXPECT_EQ(x, 30);

    const int y = 100;
    reference_wrapper<const int> rc(y);
    const int& cref = rc;
    EXPECT_EQ(cref, 100);
}

TEST(ReferenceWrapper, FunctionArgument) {
    auto add_one = [](int& a) { a++; };
    int val = 0;
    reference_wrapper<int> r(val);
    add_one(r);
    EXPECT_EQ(val, 1);
}

TEST(ReferenceWrapper, CallOperatorLambda) {
    auto f = [](int a, int b) { return a + b; };
    reference_wrapper<decltype(f)> r(f);
    EXPECT_EQ(r(3, 4), 7);
}

TEST(ReferenceWrapper, CallOperatorFreeFunction) {
    reference_wrapper<int(int)> r(free_func);
    EXPECT_EQ(r(5), 10);
}

TEST(ReferenceWrapper, CallOperatorMutableLambda) {
    int counter = 0;
    auto inc = [&counter]() { return ++counter; };
    reference_wrapper<decltype(inc)> r(inc);
    EXPECT_EQ(r(), 1);
    EXPECT_EQ(r(), 2);
}

TEST(Ref, CreateFromLvalue) {
    int a = 5;
    auto r = ref(a);
    EXPECT_TRUE((is_same_v<decltype(r), reference_wrapper<int>>) );
    EXPECT_EQ(r.get(), 5);
}

TEST(Ref, RefuseConstRvalue) { static_assert(!is_invocable_v<reference_wrapper<int>, int&&>); }

TEST(Ref, RewrapReferenceWrapper) {
    int x = 42;
    auto r1 = ref(x);
    auto r2 = ref(r1);
    EXPECT_EQ(&r1.get(), &r2.get());
}

TEST(Cref, CreateFromConstLvalue) {
    int a = 10;
    auto r = cref(a);
    EXPECT_TRUE((is_same_v<decltype(r), reference_wrapper<const int>>) );
    EXPECT_EQ(r.get(), 10);
}

TEST(Cref, RefuseConstRvalue) { static_assert(!is_invocable_v<reference_wrapper<const int>, int&&>); }

TEST(Cref, RewrapReferenceWrapper) {
    int x = 1;
    auto r1 = ref(x);
    auto r2 = cref(r1);
    EXPECT_TRUE((is_same_v<decltype(r2), reference_wrapper<const int>>) );
    EXPECT_EQ(&r1.get(), &r2.get());
}

TEST(UnwrapReference, PlainType) {
    EXPECT_TRUE((is_same_v<unwrap_reference<int>::type, int>) );
    EXPECT_TRUE((is_same_v<unwrap_reference<const double>::type, const double>) );
}

TEST(UnwrapReference, ReferenceWrapper) {
    EXPECT_TRUE((is_same_v<unwrap_reference<reference_wrapper<int>>::type, int&>) );
    EXPECT_TRUE((is_same_v<unwrap_reference<reference_wrapper<const double>>::type, const double&>) );
}

TEST(UnwrapReferenceT, Alias) {
    EXPECT_TRUE((is_same_v<unwrap_reference_t<int>, int>) );
    EXPECT_TRUE((is_same_v<unwrap_reference_t<reference_wrapper<char>>, char&>) );
}

TEST(UnwrapRefDecay, Basic) {
    int x = 0;
    reference_wrapper<int> r(x);
    using T1 = unwrap_ref_decay_t<decltype(r)>;
    EXPECT_TRUE((is_same_v<T1, int&>) );

    using T2 = unwrap_ref_decay_t<const reference_wrapper<int>&>;
    EXPECT_TRUE((is_same_v<T2, int&>) );

    using T3 = unwrap_ref_decay_t<int>;
    EXPECT_TRUE((is_same_v<T3, int>) );

    using T4 = unwrap_ref_decay_t<const int&>;
    EXPECT_TRUE((is_same_v<T4, int>) );
}

TEST(UnwrapRefDecay, ReferenceWrapperConst) {
    int v = 10;
    reference_wrapper<const int> rc(v);
    using T = unwrap_ref_decay_t<decltype(rc)>;
    EXPECT_TRUE((is_same_v<T, const int&>) );
}
