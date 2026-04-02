#ifndef NEFORCE_CORE_TYPEINFO_CHECK_TYPE_HPP__
#define NEFORCE_CORE_TYPEINFO_CHECK_TYPE_HPP__

/**
 * @file check_type.hpp
 * @brief 类型信息检查工具
 *
 * 此文件提供了用于检查C++类型信息的工具类。
 * 支持生成类型的可读字符串表示，包括：
 * - 基本类型和复合类型
 * - const/volatile限定符
 * - 指针、引用、数组
 * - 函数类型和函数指针
 * - 成员函数指针
 * - 模板实例化类型
 */

#include <typeinfo>
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__
/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @class output
 * @brief 类型输出辅助类
 *
 * 提供格式化的类型名称输出功能，支持紧凑模式和空格分隔模式。
 * 用于构建类型名称字符串。
 */
class output {
private:
    bool is_compact_; ///< 是否为紧凑模式
    string& str_;     ///< 目标字符串引用

    template <typename T>
    static NEFORCE_CONSTEXPR20 bool check_empty(const T&) noexcept {
        return false;
    }
    static NEFORCE_CONSTEXPR20 bool check_empty(const char* value) noexcept { return !value || value[0] == 0; }

    template <typename T>
    NEFORCE_CONSTEXPR20 void out(const T& value) {
        if (this->check_empty(value)) {
            return;
        }
        if (!this->is_compact_) {
            str_ += " ";
        }
        this->str_ += to_string(value);
        this->is_compact_ = false;
    }

public:
    NEFORCE_CONSTEXPR20 output(string& str) noexcept :
    is_compact_(true),
    str_(str) {}

    NEFORCE_CONSTEXPR20 output& operator()() noexcept { return *this; }

    NEFORCE_CONSTEXPR20 output& compact() noexcept {
        this->is_compact_ = true;
        return *this;
    }

    template <typename T1, typename... T>
    NEFORCE_CONSTEXPR20 output& operator()(const T1& value, const T&... args) {
        this->out(value);
        return operator()(args...);
    }
};

/**
 * @class bracket
 * @brief 括号作用域管理
 * @tparam IsStart 是否开始括号
 *
 * 在构造时添加左括号，析构时添加右括号。
 */
template <bool IsStart>
struct bracket {
    output& out_;

    NEFORCE_CONSTEXPR20 bracket(output& out, const char* = nullptr) :
    out_(out) {
        out_("(").compact();
    }

    NEFORCE_CONSTEXPR20 ~bracket() { out_.compact()(")"); }
};

template <>
struct bracket<false> {
    NEFORCE_CONSTEXPR20 bracket(output& out, const char* str = nullptr) { out(str); }
};

/**
 * @class bound
 * @brief 数组边界管理
 * @tparam N 数组大小
 *
 * 管理数组边界的添加，在析构时添加边界信息。
 */
template <size_t N = 0>
struct bound {
private:
    template <size_t NN>
    NEFORCE_CONSTEXPR20 enable_if_t<NN == 0> __bound_dispatch() const {
        out_("[]");
        return;
    }

    template <size_t NN>
    NEFORCE_CONSTEXPR20 enable_if_t<NN != 0> __bound_dispatch() const {
        out_("[").compact()(NN).compact()("]");
        return;
    }

public:
    output& out_;

    NEFORCE_CONSTEXPR20 bound(output& out) :
    out_(out) {}

    NEFORCE_CONSTEXPR20 ~bound() { __bound_dispatch<N>(); }
};

/**
 * @struct at_destruct
 * @brief 析构时输出字符串
 *
 * 管理字符串输出，在析构时输出指定的字符串。
 */
struct at_destruct {
    output& out_;
    const char* str_;

    NEFORCE_CONSTEXPR20 at_destruct(output& out, const char* str = nullptr) noexcept :
    out_(out),
    str_(str) {}

    NEFORCE_CONSTEXPR20 ~at_destruct() { out_(str_); }

    NEFORCE_CONSTEXPR20 void set_str(const char* str = nullptr) noexcept { str_ = str; }
};


#ifdef NEFORCE_COMPILER_GNUC

/**
 * @brief 获取真实的符号名称
 * @param name 修饰后的名称
 * @return 反修饰后的可读名称
 */
string NEFORCE_API real_symbol_name(string name);

#endif


/**
 * @struct check
 * @brief 类型检查主模板
 * @tparam T 要检查的类型
 * @tparam IsBase 是否为基类部分
 *
 * 递归遍历类型结构，生成完整的类型名称字符串。
 */
template <typename T, bool IsBase = false>
struct check {
    output out_;

    NEFORCE_CONSTEXPR20 check(const output& out) :
    out_(out) {
#ifdef NEFORCE_COMPILER_GNUC
        using FinT = remove_function_qualifiers_t<T>;
        out_(real_symbol_name(typeid(FinT).name()));
#else
        out_(typeid(T).name());
#endif
    }
};

/**
 * @brief 数组类型的特化
 * @tparam T 元素类型
 * @tparam IsBase 是否为基类部分
 */
template <typename T, bool IsBase>
struct check<T[], IsBase> : check<T, true> {
    using base_t = check<T, true>;
    using base_t::out_;

    bound<> bound_;
    bracket<IsBase> bracket_;

    NEFORCE_CONSTEXPR20 check(const output& out) :
    base_t(out),
    bound_(out_),
    bracket_(out_) {}
};

#define CHECK_TYPE__(OPT)                              \
    template <typename T, bool IsBase>                 \
    struct check<T OPT, IsBase> : check<T, true> {     \
        using base_t = check<T, true>;                 \
        using base_t::out_;                            \
                                                       \
        NEFORCE_CONSTEXPR20 check(const output& out) : \
        base_t(out) {                                  \
            out_(#OPT);                                \
        }                                              \
    };

CHECK_TYPE__(const)
CHECK_TYPE__(volatile)
CHECK_TYPE__(const volatile)
CHECK_TYPE__(&)
CHECK_TYPE__(&&)
CHECK_TYPE__(*)
#undef CHECK_TYPE__


template <bool IsStart, typename... P>
struct parameter;

/**
 * @struct parameter
 * @brief 函数参数处理
 * @tparam IsStart 是否为开始
 * @tparam P 参数类型包
 *
 * 递归处理函数参数列表。
 */
template <bool IsStart, typename P1, typename... P>
struct parameter<IsStart, P1, P...> {
    output& out_;

    NEFORCE_CONSTEXPR20 parameter(output& out) noexcept :
    out_(out) {}

    NEFORCE_CONSTEXPR20 ~parameter() {
        [this](bracket<IsStart>&&) {
            check<P1>{out_};
            parameter<false, P...>{out_.compact()};
        }(bracket<IsStart>{out_, ","});
    }
};

/**
 * @brief 参数处理特化（无参数）
 * @tparam IsStart 是否为开始
 */
template <bool IsStart>
struct parameter<IsStart> {
    output& out_;

    NEFORCE_CONSTEXPR20 parameter(output& out) noexcept :
    out_(out) {}

    NEFORCE_CONSTEXPR20 ~parameter() { bracket<IsStart>{out_}; }
};


#define CHECK_TYPE_ARRAY__(CV_OPT, BOUND_OPT, ...)                                \
    template <typename T, bool IsBase __VA_ARGS__>                                \
    struct check<T CV_OPT[BOUND_OPT], IsBase> : check<T CV_OPT, !is_array_v<T>> { \
        using base_t = check<T CV_OPT, !is_array_v<T>>;                           \
        using base_t::out_;                                                       \
                                                                                  \
        bound<BOUND_OPT> bound_;                                                  \
        bracket<IsBase> bracket_;                                                 \
                                                                                  \
        NEFORCE_CONSTEXPR20 check(const output& out) :                            \
        base_t(out),                                                              \
        bound_(out_),                                                             \
        bracket_(out_) {}                                                         \
    };

#define CHECK_TYPE_ARRAY_CV__(BOUND_OPT, ...)                \
    CHECK_TYPE_ARRAY__(, BOUND_OPT, , ##__VA_ARGS__)         \
    CHECK_TYPE_ARRAY__(const, BOUND_OPT, , ##__VA_ARGS__)    \
    CHECK_TYPE_ARRAY__(volatile, BOUND_OPT, , ##__VA_ARGS__) \
    CHECK_TYPE_ARRAY__(const volatile, BOUND_OPT, , ##__VA_ARGS__)

#ifdef NEFORCE_COMPILER_GNUC
CHECK_TYPE_ARRAY_CV__(0)
#endif
CHECK_TYPE_ARRAY_CV__(N, size_t N)
CHECK_TYPE_ARRAY__(const, , )
CHECK_TYPE_ARRAY__(volatile, , )
CHECK_TYPE_ARRAY__(const volatile, , )

#undef CHECK_TYPE_ARRAY__
#undef CHECK_TYPE_ARRAY_CV__

/**
 * @brief 函数类型的特化
 * @tparam T 返回类型
 * @tparam IsBase 是否为基类部分
 * @tparam P 参数类型包
 */
template <typename T, bool IsBase, typename... P>
struct check<T(P...), IsBase> : check<T, true> {
    using base_t = check<T, true>;
    using base_t::out_;

    parameter<true, P...> parameter_;
    bracket<IsBase> bracket_;

    NEFORCE_CONSTEXPR20 check(const output& out) :
    base_t(out),
    parameter_(out_),
    bracket_(out_) {}
};

/**
 * @brief 成员指针的特化
 * @tparam T 成员类型
 * @tparam IsBase 是否为基类部分
 * @tparam C 类类型
 */
template <typename T, bool IsBase, typename C>
struct check<T C::*, IsBase> : check<T, true> {
    using base_t = check<T, true>;
    using base_t::out_;

    NEFORCE_CONSTEXPR20 check(const output& out) :
    base_t(out) {
        check<C>{out_};
        out_.compact()("::*");
    }
};

/**
 * @brief 成员函数指针的特化
 * @tparam T 返回类型
 * @tparam IsBase 是否为基类部分
 * @tparam C 类类型
 * @tparam P 参数类型包
 */
template <typename T, bool IsBase, typename C, typename... P>
struct check<T (C::*)(P...), IsBase> : check<T(P...), true> {
    using base_t = check<T(P...), true>;
    using base_t::out_;

    NEFORCE_CONSTEXPR20 check(const output& out) :
    base_t(out) {
        check<C>{out_};
        out_.compact()("::*");
    }
};

#define CHECK_TYPE_MEM_FUNC__(...)                                \
    template <typename T, bool IsBase, typename C, typename... P> \
    struct check<T (C::*)(P...) __VA_ARGS__, IsBase> {            \
        at_destruct cv_;                                          \
        check<T(P...), true> base_;                               \
        output& out_ = base_.out_;                                \
                                                                  \
        NEFORCE_CONSTEXPR20 check(const output& out) :            \
        cv_(base_.out_),                                          \
        base_(out) {                                              \
            cv_.set_str(#__VA_ARGS__);                            \
            check<C>{out_};                                       \
            out_.compact()("::*");                                \
        }                                                         \
    };

CHECK_TYPE_MEM_FUNC__(const)
CHECK_TYPE_MEM_FUNC__(volatile)
CHECK_TYPE_MEM_FUNC__(const volatile)
#undef CHECK_TYPE_MEM_FUNC__

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup CheckType 可读类型名
 * @brief 类型信息检查和运行时类型识别
 * @{
 */

/**
 * @brief 检查类型并返回可读字符串
 * @tparam T 要检查的类型
 * @return 类型的可读字符串表示
 */
template <typename T>
NEFORCE_CONSTEXPR20 string check_type() {
    string str;
    inner::check<T>{str};
    return _NEFORCE move(str);
}

/** @} */ // CheckType

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TYPEINFO_CHECK_TYPE_HPP__
