#ifndef MSTL_CHECK_TYPE_HPP__
#define MSTL_CHECK_TYPE_HPP__
#include "string.hpp"
#ifdef MSTL_COMPILER_GNUC__
#include <cxxabi.h>
#endif // MSTL_COMPILER_GNUC__
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

class output {
private:
    bool is_compact_;
    string& sr_;

    template <typename T>
    static MSTL_CONSTEXPR20 bool check_empty(const T&) { return false; }
    static MSTL_CONSTEXPR20 bool check_empty(const char* val) { return !val || val[0] == 0; }

    template <typename T>
    MSTL_CONSTEXPR20 void out(const T& val) {
        if (this->check_empty(val)) return;
        if (!this->is_compact_) sr_ += " ";
        this->sr_ += _MSTL to_string(val);
        this->is_compact_ = false;
    }

public:
    MSTL_CONSTEXPR20 output(string& sr) : is_compact_(true), sr_(sr) {}
    MSTL_CONSTEXPR20 output& operator ()() { return *this; }
    MSTL_CONSTEXPR20 output& compact(){
        this->is_compact_ = true;
        return *this;
    }

    template <typename T1, typename... T>
    MSTL_CONSTEXPR20 output& operator()(const T1& val, const T&... args) {
        this->out(val);
        return operator()(args...);
    }
};

template <bool>
struct bracket {   // ()
    output& out_;
    MSTL_CONSTEXPR20 bracket(output& out, const char* = nullptr) : out_(out) {
        out_("(").compact();
    }
    MSTL_CONSTEXPR20 ~bracket() {
        out_.compact()(")");
    }
};
template <>
struct bracket<false> {
    MSTL_CONSTEXPR20 bracket(output& out, const char* str = nullptr) { out(str); }
};

template <size_t N = 0>
struct bound {   // [N]
private:
    template <size_t NN, enable_if_t<NN == 0, int> = 0>
    MSTL_CONSTEXPR20 void __bound_dispatch() const { out_("[]"); }
    template <size_t NN, enable_if_t<NN != 0, int> = 0>
    MSTL_CONSTEXPR20 void __bound_dispatch() const { out_("[").compact()(NN).compact()("]"); }

public:
    output& out_;

    MSTL_CONSTEXPR20 bound(output& out) : out_(out) {}
    MSTL_CONSTEXPR20 ~bound() {
        __bound_dispatch<N>();
    }
};

template <bool, typename... P>
struct parameter;   // parameter

template <bool IsStart>
struct parameter<IsStart> {
    output& out_;

    MSTL_CONSTEXPR20 parameter(output& out) : out_(out) {}
    MSTL_CONSTEXPR20 ~parameter() { bracket<IsStart> { out_ }; }
};


template <typename T, bool IsBase = false>
struct check {
    output out_;

    MSTL_CONSTEXPR20 check(const output& out) : out_(out) {
#ifdef MSTL_COMPILER_GNUC__
        auto deleter = [](char* p) { if (p) std::free(p); };
        using FinT = remove_function_qualifiers_t<T>;
        _MSTL unique_ptr<char, decltype(deleter)> real_name {
            ::abi::__cxa_demangle(typeid(FinT).name(), nullptr, nullptr, nullptr), deleter
        };
        out_(real_name.get());
#else
        out_(typeid(T).name());
#endif
    }
};

template <typename T, bool IsBase>
struct check<T[], IsBase> : check<T, true> {   // array
    using base_t = check<T, true>;
    using base_t::out_;
    bound<>         bound_;
    bracket<IsBase> bracket_;

    MSTL_CONSTEXPR20 check(const output& out) : base_t(out), bound_(out_), bracket_(out_) {}
};

#define CHECK_TYPE__(OPT) \
    template <typename T, bool IsBase> \
    struct check<T OPT, IsBase> : check<T, true> { \
        using base_t = check<T, true>; \
        using base_t::out_; \
        MSTL_CONSTEXPR20 check(const output& out) : base_t(out) { out_(#OPT); } \
    };

CHECK_TYPE__(const)
CHECK_TYPE__(volatile)
CHECK_TYPE__(const volatile)
CHECK_TYPE__(&)
CHECK_TYPE__(&&)
CHECK_TYPE__(*)
#undef CHECK_TYPE__


template <bool IsStart, typename P1, typename... P>
struct parameter<IsStart, P1, P...> {
    output& out_;
    MSTL_CONSTEXPR20 parameter(output& out) : out_(out) {}
    MSTL_CONSTEXPR20 ~parameter() {
        [this](bracket<IsStart>&&) {
            check<P1> { out_ };
            parameter<false, P...> { out_.compact() };
        } (bracket<IsStart> { out_, "," });
    }
};

#define CHECK_TYPE_ARRAY__(CV_OPT, BOUND_OPT, ...) \
    template <typename T, bool IsBase __VA_ARGS__> \
    struct check<T CV_OPT [BOUND_OPT], IsBase> : check<T CV_OPT, !is_array_v<T>> { \
        using base_t = check<T CV_OPT, !is_array_v<T>>; \
        using base_t::out_; \
        bound<BOUND_OPT> bound_; \
        bracket<IsBase>  bracket_; \
        MSTL_CONSTEXPR20 check(const output& out) : base_t(out) \
            , bound_  (out_) \
            , bracket_(out_) \
        {} \
    };

#define CHECK_TYPE_ARRAY_CV__(BOUND_OPT, ...) \
    CHECK_TYPE_ARRAY__(, BOUND_OPT, ,##__VA_ARGS__) \
    CHECK_TYPE_ARRAY__(const, BOUND_OPT, ,##__VA_ARGS__) \
    CHECK_TYPE_ARRAY__(volatile, BOUND_OPT, ,##__VA_ARGS__) \
    CHECK_TYPE_ARRAY__(const volatile, BOUND_OPT, ,##__VA_ARGS__)

#if defined(MSTL_COMPILE_GNUC__)
CHECK_TYPE_ARRAY_CV__(0)
#endif
CHECK_TYPE_ARRAY_CV__(N, size_t N)
CHECK_TYPE_ARRAY__(const, , )
CHECK_TYPE_ARRAY__(volatile, , )
CHECK_TYPE_ARRAY__(const volatile, , )

#undef CHECK_TYPE_ARRAY__
#undef CHECK_TYPE_ARRAY_CV__

template <typename T, bool IsBase, typename... P>
struct check<T(P...), IsBase> : check<T, true> {   // function
    using base_t = check<T, true>;
    using base_t::out_;
    parameter<true, P...> parameter_;
    bracket<IsBase>       bracket_;

    MSTL_CONSTEXPR20 check(const output& out)
    : base_t(out), parameter_(out_), bracket_(out_) {}
};

template <typename T, bool IsBase, typename C>
struct check<T C::*, IsBase> : check<T, true> {   // member function
    using base_t = check<T, true>;
    using base_t::out_;

    MSTL_CONSTEXPR20 check(const output& out) : base_t(out) {
        check<C> { out_ };
        out_.compact()("::*");
    }
};

template <typename T, bool IsBase, typename C, typename... P>
struct check<T(C::*)(P...), IsBase> : check<T(P...), true> {   // member object function
    using base_t = check<T(P...), true>;
    using base_t::out_;

    MSTL_CONSTEXPR20 check(const output& out) : base_t(out) {
        check<C> { out_ };
        out_.compact()("::*");
    }
};

struct at_destruct {
    output& out_;
    const char* str_;

    MSTL_CONSTEXPR20 at_destruct(output& out, const char* str = nullptr): out_(out), str_(str) {}
    MSTL_CONSTEXPR20 ~at_destruct() { out_(str_); }
    MSTL_CONSTEXPR20 void set_str(const char* str = nullptr) { str_ = str; }
};

#define CHECK_TYPE_MEM_FUNC__(...) \
    template <typename T, bool IsBase, typename C, typename... P> \
    struct check<T(C::*)(P...) __VA_ARGS__, IsBase> { \
        at_destruct cv_; \
        check<T(P...), true> base_; \
        output& out_ = base_.out_; \
        MSTL_CONSTEXPR20 check(const output& out) \
        : cv_(base_.out_), base_(out) { \
            cv_.set_str(#__VA_ARGS__); \
            check<C> { out_ }; \
            out_.compact()("::*"); \
        } \
    };

CHECK_TYPE_MEM_FUNC__(const)
CHECK_TYPE_MEM_FUNC__(volatile)
CHECK_TYPE_MEM_FUNC__(const volatile)
#undef CHECK_TYPE_MEM_FUNC__

MSTL_END_INNER__

template <typename T>
MSTL_CONSTEXPR20 string check_type() {
    string str;
    _INNER check<T> { str };
    return str;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CHECK_TYPE_HPP__
