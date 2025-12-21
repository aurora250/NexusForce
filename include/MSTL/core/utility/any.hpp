#ifndef MSTL_CORE_UTILITY_ANY_HPP__
#define MSTL_CORE_UTILITY_ANY_HPP__
#include <initializer_list>
#include <typeinfo>
#include "../exception/exception.hpp"
#include "../interface/icommon.hpp"
#include "../typeinfo/tags.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API any;

MSTL_BEGIN_INNER__
struct __any_cast_true_tag {};
struct __any_cast_false_tag {};

template <typename T, typename U>
const T* __any_cast_aux_dispatch_impl(const _MSTL any* value, __any_cast_true_tag) noexcept;
MSTL_END_INNER__

MSTL_ERROR_BUILD_DERIVED_CLASS(anycast_exception, typecast_exception, "Cast From any Type Failed.")


class MSTL_API any : public iswappable<any> {
    union storage_internal {
		storage_internal() = default;
		storage_internal(const storage_internal&) = delete;
		storage_internal& operator=(const storage_internal&) = delete;

		void* ptr_ = nullptr;
		aligned_storage_t<sizeof(ptr_), alignof(void*)> buffer_;
    };

    enum ANY_INNER_OPERATION {
        ACCESS, GET_TYPE_INFO, COPY, DESTROY, SWAP
    };

    union ArgT {
        void* obj_ptr_;
        const std::type_info* type_ptr_;
        any* any_ptr_;
    };

    template <typename T>
    struct internal_manage {
        static void manage(ANY_INNER_OPERATION, const any*, ArgT*);

        template <typename U>
        static void create(storage_internal& storage, U&& value) {
            void* ptr = &storage.buffer_;
            ::new (ptr) T(_MSTL forward<U>(value));
        }

        template <typename... Args>
        static void create(storage_internal& storage, Args&&... args) {
            void* ptr = &storage.buffer_;
            ::new (ptr) T(_MSTL forward<Args>(args)...);
        }

        static T* access(const storage_internal& storage) {
            const void* ptr = &storage.buffer_;
            return static_cast<T*>(const_cast<void*>(ptr));
        }
    };

    template <typename T>
    struct external_manage {
        static void manage(ANY_INNER_OPERATION, const any*, ArgT*);

        template<typename U>
        static void create(storage_internal& storage, U&& value) {
            storage.ptr_ = ::new T(_MSTL forward<U>(value));
        }
        template <typename... Args>
        static void create(storage_internal& storage, Args&&... args) {
            storage.ptr_ = ::new T(_MSTL forward<Args>(args)...);
        }
        static T* access(const storage_internal& storage) {
            return static_cast<T*>(storage.ptr_);
        }
    };

    template <typename T>
    using manage_t = conditional_t<
        is_nothrow_move_constructible_v<T> && sizeof(T) <= sizeof(storage_internal) && alignof(T) <= alignof(storage_internal)
        , internal_manage<T>, external_manage<T>>;


    void (* manage_)(ANY_INNER_OPERATION, const any*, ArgT*);
    storage_internal storage_;

    template <typename T, typename U>
    friend const T* _INNER __any_cast_aux_dispatch_impl(const any* value, _INNER __any_cast_true_tag) noexcept;


    template <typename T, typename... Args, typename Manager = manage_t<T>>
    void try_emplace(Args&&... args) {
	    this->reset();
        Manager::create(storage_, _MSTL forward<Args>(args)...);
        manage_ = &Manager::manage;
    }

    template <typename T, typename U, typename... Args, typename Manager = manage_t<T>>
    void try_emplace(std::initializer_list<U> ilist, Args&&... args) {
	    this->reset();
	    Manager::create(storage_, ilist, _MSTL forward<Args>(args)...);
	    manage_ = &Manager::manage;
    }

public:
    any() noexcept : manage_(nullptr) {}
    any(const any& x);
    any& operator =(const any& rhs) { *this = any(rhs); return *this; }
    any(any&& x) noexcept;
    any& operator =(any&& rhs) noexcept;

    template <typename T, typename VT = decay_t<T>, typename Manager = manage_t<VT>,
        enable_if_t<is_copy_constructible_v<VT> && !is_same_v<_MSTL_TAG inplace_construct_tag, VT> && !is_same_v<VT, any>, int> = 0>
    explicit any(T&& value) : manage_(&Manager::manage) {
        Manager::create(storage_, _MSTL forward<T>(value));
    }

    template <typename T, typename VT = decay_t<T>,
        enable_if_t<!is_same_v<VT, any> && is_copy_constructible_v<VT>, int> = 0>
    any& operator =(T&& rhs) {
        *this = any(_MSTL forward<T>(rhs));
        return *this;
    }

    template <typename T, typename... Args, typename VT = decay_t<T>, typename Manager = manage_t<VT>,
        enable_if_t<conjunction_v<is_copy_constructible<VT>, is_constructible<VT, Args&&...>>, int> = 0>
    explicit any(_MSTL_TAG inplace_construct_tag, Args&&... args) : manage_(&Manager::manage) {
        Manager::create(storage_, _MSTL forward<Args>(args)...);
    }

    template <typename T, typename U, typename... Args, typename VT = decay_t<T>, typename Manager = manage_t<VT>,
        enable_if_t<conjunction_v<is_copy_constructible<VT>, is_constructible<VT, std::initializer_list<U>&, Args&&...>>, int> = 0>
    explicit any(_MSTL_TAG inplace_construct_tag, std::initializer_list<U> ilist, Args&&... args) : manage_(&Manager::manage) {
	    Manager::create(storage_, ilist, _MSTL forward<Args>(args)...);
    }

    ~any() { reset(); }

    template <typename T, typename... Args, typename VT = decay_t<T>,
        enable_if_t<conjunction_v<is_copy_constructible<VT>, is_constructible<VT, Args&&...>>, int> = 0>
    VT emplace(Args&&... args) {
        this->try_emplace<VT>(_MSTL forward<Args>(args)...);
        return *manage_t<VT>::access(storage_);
    }

    template <typename T, typename U, typename... Args, typename VT = decay_t<T>,
        enable_if_t<conjunction_v<is_copy_constructible<VT>, is_constructible<VT, std::initializer_list<U>&, Args&&...>>, int> = 0>
    VT emplace(std::initializer_list<U> ilist, Args&&... args) {
        this->try_emplace<VT, U>(ilist, _MSTL forward<Args>(args)...);
        return *manage_t<VT>::access(storage_);
    }

    void reset() noexcept {
        if (has_value()) {
            manage_(DESTROY, this, nullptr);
            manage_ = nullptr;
        }
    }
    MSTL_NODISCARD bool has_value() const noexcept { return manage_ != nullptr; }
    MSTL_NODISCARD const std::type_info& type() const noexcept;
    void swap(any& rhs) noexcept;
};

template <typename T, typename... Args,
	enable_if_t<is_constructible_v<any, _MSTL_TAG inplace_construct_tag, Args...>, int> = 0>
any make_any(Args&&... args) {
	return any(_MSTL_TAG inplace_construct_tag{}, _MSTL forward<Args>(args)...);
}

template <typename T, typename U, typename... Args,
    enable_if_t<is_constructible_v<any, _MSTL_TAG inplace_construct_tag, std::initializer_list<U>&, Args...>, int> = 0>
any make_any(std::initializer_list<U> ilist, Args&&... args) {
	return any(_MSTL_TAG inplace_construct_tag{}, ilist, _MSTL forward<Args>(args)...);
}


MSTL_BEGIN_INNER__

template <typename T, typename U>
const T* __any_cast_aux_dispatch_impl(const any* value, __any_cast_true_tag) noexcept {
    if (value->manage_ == &any::manage_t<U>::manage || value->type() == typeid(T))
        return static_cast<const T*>(any::manage_t<U>::access(value->storage_));
    return nullptr;
}

template <typename T, typename U>
const T* __any_cast_aux_dispatch_impl(const any*, __any_cast_false_tag) noexcept {
    return nullptr;
}

template <typename T, typename U>
const T* __any_cast_aux_dispatch(const any* value) noexcept {
    using tag = conditional_t<
        (is_same_v<decay_t<U>, U> || is_copy_constructible_v<U>),
        __any_cast_true_tag, __any_cast_false_tag
    >;
    return _INNER __any_cast_aux_dispatch_impl<T, U>(value, tag{});
}

template <typename T, enable_if_t<is_object_v<T>, int> = 0>
const T* __any_cast_aux(const any* value) noexcept {
    if (value)
        return __any_cast_aux_dispatch<T, remove_cv_t<T>>(value);
    return nullptr;
}

template <typename T, enable_if_t<!is_object_v<T>, int> = 0>
const T* __any_cast_aux(const any*) noexcept {
    return nullptr;
}

MSTL_END_INNER__

template <typename T>
const T* any_cast(const any* value) noexcept {
    return _INNER __any_cast_aux<T>(value);
}

template <typename T>
T* any_cast(any* value) noexcept {
    return const_cast<T*>(any_cast<T>(const_cast<const any*>(value)));
}

template <typename T>
T any_cast(const any& value) {
    using U = remove_cvref_t<T>;
    static_assert(disjunction_v<is_reference<T>, is_copy_constructible<T>, is_constructible<T, const U&>>,
        "type T must be valid to cast from any.");
    auto ptr = any_cast<U>(&value);
    if (ptr)
        return static_cast<T>(*ptr);
    throw_exception(anycast_exception());
    return T();
}

template <typename T>
void any::internal_manage<T>::manage(const ANY_INNER_OPERATION oper, const any* value, ArgT* arg) {
    auto ptr = reinterpret_cast<const T*>(&value->storage_.buffer_);
    switch (oper) {
        case ACCESS: {
            arg->obj_ptr_ = const_cast<T*>(ptr);
            break;
        }
        case GET_TYPE_INFO: {
            arg->type_ptr_ = &typeid(T);
            break;
        }
        case COPY: {
            ::new(&arg->any_ptr_->storage_.buffer_) T(*ptr);
            arg->any_ptr_->manage_ = value->manage_;
            break;
        }
        case DESTROY: {
            ptr->~T();
            break;
        }
        case SWAP: {
            ::new(&arg->any_ptr_->storage_.buffer_) T(_MSTL move(*const_cast<T*>(ptr)));
            ptr->~T();
            arg->any_ptr_->manage_ = value->manage_;
            const_cast<any*>(value)->manage_ = nullptr;
            break;
        }
        default: break;
    }
}

template<typename T>
void any::external_manage<T>::manage(const ANY_INNER_OPERATION oper, const any* value, ArgT* arg) {
    auto ptr = static_cast<const T*>(value->storage_.ptr_);
    switch (oper) {
        case ACCESS: {
            arg->obj_ptr_ = const_cast<T*>(ptr);
            break;
        }
        case GET_TYPE_INFO: {
            arg->type_ptr_ = &typeid(T);
            break;
        }
        case COPY: {
            arg->any_ptr_->storage_.ptr_ = ::new T(*ptr);
            arg->any_ptr_->manage_ = value->manage_;
            break;
        }
        case DESTROY: {
            delete ptr;
            break;
        }
        case SWAP: {
            arg->any_ptr_->storage_.ptr_ = value->storage_.ptr_;
            arg->any_ptr_->manage_ = value->manage_;
            const_cast<any*>(value)->manage_ = nullptr;
            break;
        }
        default: break;
    }
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_ANY_HPP__
