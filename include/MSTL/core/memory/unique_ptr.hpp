#ifndef MSTL_CORE_MEMORY_UNIQUE_PTR_HPP__
#define MSTL_CORE_MEMORY_UNIQUE_PTR_HPP__
#include "../compound/tuple.hpp"
#include "../functional/functor.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct default_delete {
    constexpr default_delete() noexcept = default;

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
	MSTL_CONSTEXPR20 default_delete(const default_delete<U>&) noexcept {}

    MSTL_CONSTEXPR20 void operator()(const T* ptr) const {
	    static_assert(is_allocable_v<T>, "can not delete types which can`t be allocated.");
	    delete ptr;
    }

    template <typename U>
    MSTL_CONSTEXPR20 default_delete<U> rebind() && {
        return default_delete<U>();
    }
};

template <typename T>
struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;

    template <typename U, enable_if_t<is_convertible_v<U(*)[], T(*)[]>, int> = 0>
    MSTL_CONSTEXPR20 default_delete(const default_delete<U[]>&) noexcept {}

    template <typename U, enable_if_t<is_convertible_v<U(*)[], T(*)[]>, int> = 0>
    MSTL_CONSTEXPR20 void operator ()(U* ptr) const{
	    static_assert(is_allocable_v<T>, "can not delete types which can`t be allocated.");
	    delete [] ptr;
	}

    template <typename U>
    MSTL_CONSTEXPR20 default_delete<U[]> rebind() && {
        return default_delete<U[]>();
    }
};


MSTL_BEGIN_INNER__

template <typename T, typename Deleter>
class __unique_ptr_impl {
private:
    template <typename U, typename E, typename = void>
	struct inner_ptr {
	    using type = U*;
	};
    template <typename U, typename E>
	struct inner_ptr<U, E, void_t<typename remove_reference_t<E>::pointer>> {
	    using type = typename remove_reference<E>::type::pointer;
	};

public:
    using DeleterConstraint = enable_if<is_default_constructible_v<Deleter>>;

    using pointer = typename inner_ptr<T, Deleter>::type;

private:
    _MSTL tuple<pointer, Deleter> tup{};

    static_assert(!is_rvalue_reference_v<Deleter>,
        "deleter type of unique_ptr must be a function object type or an lvalue reference type");

public:
    __unique_ptr_impl() = default;
    MSTL_CONSTEXPR20 __unique_ptr_impl(pointer ptr) : tup() { get_ptr() = ptr; }

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_impl(pointer ptr, Del&& del)
	: tup(ptr, _MSTL forward<Del>(del)) {}

    MSTL_CONSTEXPR20 __unique_ptr_impl(__unique_ptr_impl&& ptr) noexcept
    : tup(_MSTL move(ptr.tup)) { ptr.get_ptr() = nullptr; }

    MSTL_CONSTEXPR20 __unique_ptr_impl& operator =(__unique_ptr_impl&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<Deleter>(x.get_deleter());
	    return *this;
    }

    MSTL_CONSTEXPR20 pointer& get_ptr() noexcept { return _MSTL get<0>(tup); }
    MSTL_CONSTEXPR20 pointer get_ptr() const noexcept { return _MSTL get<0>(tup); }
    MSTL_CONSTEXPR20 Deleter& get_deleter() noexcept { return _MSTL get<1>(tup); }
    MSTL_CONSTEXPR20 const Deleter& get_deleter() const noexcept { return _MSTL get<1>(tup); }

    MSTL_CONSTEXPR20 void reset(pointer ptr) noexcept {
	    const pointer old = get_ptr();
	    get_ptr() = ptr;
	    if (old)
	        get_deleter()(old);
    }

    MSTL_CONSTEXPR20 pointer release() noexcept {
	    pointer p = get_ptr();
	    get_ptr() = nullptr;
	    return p;
    }

    MSTL_CONSTEXPR20 void swap(__unique_ptr_impl& x) noexcept {
	    _MSTL swap(get_ptr(), x.get_ptr());
	    _MSTL swap(get_deleter(), x.get_deleter());
    }
};

template <typename T, typename Deleter,
    bool = is_move_constructible_v<Deleter>,
    bool = is_move_assignable_v<Deleter>>
struct __unique_ptr_data : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = default;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = default;
};

template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, true, false> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = default;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = delete;
};

template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, false, true> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = delete;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = default;
};

template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, false, false> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = delete;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = delete;
};

MSTL_END_INNER__


template <typename T, typename Deleter = default_delete<T>>
class unique_ptr {
private:
    template <typename U>
	using DeleterConstraint = typename _INNER __unique_ptr_impl<T, U>::DeleterConstraint::type;

    _INNER __unique_ptr_data<T, Deleter> data_{};

public:
    using pointer       = typename _INNER __unique_ptr_impl<T, Deleter>::pointer;
    using element_type  = T;
    using deleter_type  = Deleter;

private:
    template <typename U, typename E>
	using safe_conversion = conjunction<
	    is_convertible<typename unique_ptr<U, E>::pointer, pointer>, negation<is_array<U>>>;

public:
    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
    MSTL_CONSTEXPR20 unique_ptr(pointer p) noexcept : data_(p) {}

    template <typename Del = deleter_type, enable_if_t<is_copy_constructible_v<Del>, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(pointer ptr, const deleter_type& del) noexcept : data_(ptr, del) {}

    template <typename Del = deleter_type, enable_if_t<is_move_constructible_v<Del>, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(pointer ptr, Del&& del) noexcept : data_(ptr, _MSTL move(del)) {}

    template<typename Del = deleter_type, typename DelMoveRef = remove_reference_t<Del>>
    MSTL_CONSTEXPR20 unique_ptr(pointer, enable_if_t<is_lvalue_reference_v<Del>, DelMoveRef&&>) = delete;

    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
	constexpr unique_ptr(nullptr_t = nullptr) noexcept : data_() {}

    MSTL_CONSTEXPR20 unique_ptr(unique_ptr&&) = default;

    template <typename U, typename E, enable_if_t<conjunction_v<safe_conversion<U, E>,
        conditional_t<is_reference_v<Deleter>, is_same<E, Deleter>, is_convertible<E, Deleter>>>, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(unique_ptr<U, E>&& x) noexcept
	    : data_(x.release(), _MSTL forward<E>(x.get_deleter())) {}

    ~unique_ptr() noexcept {
	    static_assert(is_invocable_v<deleter_type&, pointer>, "deleter of unique_ptr must be invocable with a pointer");
	    auto& ptr = data_.get_ptr();
	    if (ptr != nullptr)
	        get_deleter()(_MSTL move(ptr));
	    ptr = pointer();
    }

    unique_ptr& operator =(unique_ptr&&) = default;

    template <typename U, typename E, enable_if_t<conjunction_v<
        safe_conversion<U, E>, is_assignable<deleter_type&, E&&>>, int> = 0>
	MSTL_CONSTEXPR20 unique_ptr& operator =(unique_ptr<U, E>&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<E>(x.get_deleter());
	    return *this;
	}

    MSTL_CONSTEXPR20 unique_ptr& operator =(nullptr_t) noexcept {
	    reset();
	    return *this;
    }

    MSTL_CONSTEXPR20 add_lvalue_reference_t<element_type> operator *() const
    noexcept(noexcept(*_MSTL declval<pointer>())) {
	    return *get();
    }
    MSTL_CONSTEXPR20 pointer operator ->() const noexcept {
	    return get();
    }

    MSTL_CONSTEXPR20 pointer get() const noexcept { return data_.get_ptr(); }
    MSTL_CONSTEXPR20 deleter_type& get_deleter() noexcept { return data_.get_deleter(); }
    MSTL_CONSTEXPR20 const deleter_type& get_deleter() const noexcept { return data_.get_deleter(); }

    MSTL_CONSTEXPR20 explicit operator bool() const noexcept {
        return get() == pointer() ? false : true;
    }

    MSTL_CONSTEXPR20 pointer release() noexcept { return data_.release(); }
    MSTL_CONSTEXPR20 void reset(pointer ptr = pointer()) noexcept {
	    static_assert(is_invocable_v<deleter_type&, pointer>,
	        "deleter of unique_ptr must be invocable with a pointer");
	    data_.reset(_MSTL move(ptr));
    }

    MSTL_CONSTEXPR20 void swap(unique_ptr& x) noexcept {
	    static_assert(is_swappable_v<Deleter>, "deleter must be swappable.");
	    data_.swap(x.data_);
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator =(const unique_ptr&) = delete;
};

template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
    template <typename U>
    using DeleterConstraint = typename _INNER __unique_ptr_impl<T, U>::DeleterConstraint::type;

    _INNER __unique_ptr_data<T, Deleter> data_;

public:
    using pointer	        = typename _INNER __unique_ptr_impl<T, Deleter>::pointer;
    using element_type    = T;
    using deleter_type    = Deleter;

    template <typename U, typename E, typename UP = unique_ptr<U, E>,
        typename UP_pointer = typename UP::pointer,
        typename UP_element_type = typename UP::element_type>
	using safe_conversion = conjunction<is_array<U>, is_same<pointer, element_type*>,
          is_same<UP_pointer, UP_element_type*>, is_convertible<UP_element_type(*)[], element_type(*)[]>>;

    template <typename U>
    using safe_conversion_raw = conjunction<disjunction<disjunction<is_same<U, pointer>, is_same<U, nullptr_t>>,
        conjunction<is_pointer<U>, is_same<pointer, element_type*>,
            is_convertible<remove_pointer_t<U>(*)[],element_type(*)[]>>>>;

    template <typename U, typename Del = Deleter, typename = DeleterConstraint<Del>,
        enable_if_t<safe_conversion_raw<U>::value, int> = 0>
    MSTL_CONSTEXPR20 explicit unique_ptr(U ptr) noexcept : data_(ptr) {}

    template <typename U, typename Del = deleter_type,
        enable_if_t<conjunction_v<safe_conversion_raw<U>, is_copy_constructible<Del>>, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(U ptr, const deleter_type& del) noexcept : data_(ptr, del) {}

    template <typename U, typename Del = deleter_type,
        enable_if_t<conjunction_v<safe_conversion_raw<U>, is_move_constructible<Del>>, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(U ptr, enable_if_t<!is_lvalue_reference_v<Del>, Del&&> del) noexcept
    	: data_(_MSTL move(ptr), _MSTL move(del)) {}

    template <typename U, typename Del = deleter_type, typename DelMoveRef = remove_reference_t<Del>,
        enable_if_t<safe_conversion_raw<U>::value, int> = 0>
	unique_ptr(U, enable_if_t<is_lvalue_reference_v<Del>, DelMoveRef&&>) = delete;

    unique_ptr(unique_ptr&&) = default;

    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
	constexpr unique_ptr(nullptr_t = nullptr) noexcept : data_() {}

    template <typename U, typename E, enable_if_t<conjunction_v<safe_conversion<U, E>,
	       conditional_t<is_reference_v<Deleter>, is_same<E, Deleter>, is_convertible<E, Deleter>>>, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(unique_ptr<U, E>&& x) noexcept
	    : data_(x.release(), _MSTL forward<E>(x.get_deleter())) {}

    MSTL_CONSTEXPR20 ~unique_ptr() {
	    auto& ptr = data_.get_ptr();
	    if (ptr != nullptr)
	        get_deleter()(ptr);
	    ptr = pointer();
    }

    unique_ptr& operator =(unique_ptr&&) = default;

    template <typename U, typename E, enable_if_t<conjunction_v<
        safe_conversion<U, E>, is_assignable<deleter_type&, E&&>, int>> = 0>
    MSTL_CONSTEXPR20 unique_ptr& operator =(unique_ptr<U, E>&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<E>(x.get_deleter());
	    return *this;
	}

    MSTL_CONSTEXPR20 unique_ptr& operator =(nullptr_t) noexcept {
	    reset();
	    return *this;
    }

    MSTL_CONSTEXPR20 add_lvalue_reference_t<element_type> operator [](size_t idx) const {
	    MSTL_DEBUG_VERIFY(get() != pointer(), "_MSTL add_lvalue_reference_t<element_type> failed");
	    return get()[idx];
    }

    MSTL_CONSTEXPR20 pointer get() const noexcept { return data_.get_ptr(); }
    MSTL_CONSTEXPR20 deleter_type& get_deleter() noexcept { return data_.get_deleter(); }
    MSTL_CONSTEXPR20 const deleter_type& get_deleter() const noexcept { return data_.get_deleter(); }

    MSTL_CONSTEXPR20 explicit operator bool() const noexcept {
        return get() == pointer() ? false : true;
    }

    MSTL_CONSTEXPR20 pointer release() noexcept { return data_.release(); }
    template <typename U, enable_if_t<conjunction_v<disjunction<is_same<U, pointer>, conjunction<
        is_same<pointer, element_type*>, is_pointer<U>, is_convertible<remove_pointer_t<U>(*)[],element_type(*)[]>>>>, int> = 0>
    MSTL_CONSTEXPR20 void reset(U ptr) noexcept { data_.reset(_MSTL move(ptr)); }
    MSTL_CONSTEXPR20 void reset(nullptr_t = nullptr) noexcept { reset(pointer()); }

    MSTL_CONSTEXPR20 void swap(unique_ptr& x) noexcept {
	    static_assert(is_swappable_v<Deleter>, "deleter must be swappable");
	    data_.swap(x.data_);
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator =(const unique_ptr&) = delete;
};

template <typename T, typename Deleter, enable_if_t<is_swappable_v<Deleter> && is_swappable_v<T>, int> = 0>
void swap(unique_ptr<T, Deleter>& lh, unique_ptr<T, Deleter>& rh) noexcept {
    lh.swap(rh);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return lh.get() == rh.get();
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return !lh;
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return !rh;
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return lh.get() != rh.get();
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return static_cast<bool>(lh);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return static_cast<bool>(rh);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    using common_t = common_type_t<typename unique_ptr<T, D>::pointer, typename unique_ptr<U, E>::pointer>;
    return _MSTL less<common_t>()(lh.get(), rh.get());
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(lh.get(), nullptr);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(nullptr, rh.get());
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return rh.get() < lh.get();
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(nullptr, lh.get());
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(rh.get(), nullptr);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return !(lh > rh);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return !(lh > nullptr);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return !(nullptr > rh);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return !(lh < rh);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return !(lh < nullptr);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return !(nullptr < rh);
}


template <typename T, typename U>
unique_ptr<T> static_pointer_cast(const unique_ptr<U>& ptr) = delete;
template <typename T, typename U>
unique_ptr<T> const_pointer_cast(const unique_ptr<U>& ptr) = delete;
template <typename T, typename U>
unique_ptr<T> reinterpret_pointer_cast(const unique_ptr<U>& ptr) = delete;
template <typename T, typename U>
unique_ptr<T> dynamic_pointer_cast(const unique_ptr<U>& ptr) = delete;

template <typename T, typename U>
MSTL_CONSTEXPR20 unique_ptr<T> static_pointer_cast(unique_ptr<U>&& ptr) {
    return unique_ptr<T>(static_cast<T*>(ptr.release()), ptr.get_deleter());
}
template <typename T, typename U>
MSTL_CONSTEXPR20 unique_ptr<T> const_pointer_cast(unique_ptr<U>&& ptr) {
    return unique_ptr<T>(const_cast<T*>(ptr.release()), ptr.get_deleter());
}
template <typename T, typename U>
unique_ptr<T> reinterpret_pointer_cast(unique_ptr<U>&& ptr) {
    return unique_ptr<T>(reinterpret_cast<T*>(ptr.release()), ptr.get_deleter());
}
template <typename T, typename U>
unique_ptr<T> dynamic_pointer_cast(unique_ptr<U>&& ptr) {
    T* tmp = dynamic_cast<T*>(ptr.release());
    if (tmp != nullptr) {
        return unique_ptr<T>(tmp, _MSTL move(ptr.get_deleter()).template rebind<T>());
    }
    return nullptr;
}


template <typename T, typename Deleter>
struct hash<unique_ptr<T, Deleter>> {
    MSTL_CONSTEXPR20 size_t operator ()(const unique_ptr<T, Deleter>& ptr) const
    noexcept(noexcept(_MSTL declval<_MSTL hash<
        typename unique_ptr<T, Deleter>::pointer>>()(_MSTL declval<typename unique_ptr<T, Deleter>::pointer>()))) {
        return hash<T>()(ptr.get());
    }
};


template <typename T, typename... Args, enable_if_t<!is_array_v<T>, int> = 0>
MSTL_CONSTEXPR20 unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(_MSTL forward<Args>(args)...));
}
template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_CONSTEXPR20 unique_ptr<T> make_unique(const size_t len) {
    return unique_ptr<T>(new remove_extent_t<T>[len]());
}
template <typename T, typename... Args, enable_if_t<is_bounded_array_v<T>, int> = 0>
unique_ptr<T> make_unique(Args&&...) = delete;


template <typename T, enable_if_t<!is_array_v<T>, int> = 0>
MSTL_CONSTEXPR20 unique_ptr<T> make_unique_for_overwrite() {
    return unique_ptr<T>(new T());
}
template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_CONSTEXPR20 unique_ptr<T> make_unique_for_overwrite(const size_t len) {
    return unique_ptr<T>(new remove_extent_t<T>[len]());
}
template <typename T, typename... Args, enable_if_t<is_bounded_array_v<T>, int> = 0>
unique_ptr<T> make_unique_for_overwrite(Args&&...) = delete;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_UNIQUE_PTR_HPP__
