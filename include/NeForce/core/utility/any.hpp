#ifndef NEFORCE_CORE_UTILITY_ANY_HPP__
#define NEFORCE_CORE_UTILITY_ANY_HPP__

/**
 * @file any.hpp
 * @brief 任意类
 *
 * 此文件提供了类型安全的任意类，可以在运行时存储和访问任何类型的值。
 */

#include <initializer_list>
#include <typeinfo>
#include "NeForce/core/exception/breakpoint.hpp"
#include "NeForce/core/exception/exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API any;

/// @cond

NEFORCE_BEGIN_INNER__

struct any_cast_true_tag {};
struct any_cast_false_tag {};

/**
 * @brief any_cast辅助分发实现
 * @tparam T 目标类型
 * @tparam U 存储类型
 * @param value 指向any的指针
 * @return 指向转换后值的指针
 */
template <typename T, typename U>
const T* __any_cast_aux_dispatch_impl(const _NEFORCE any* value, any_cast_true_tag /*unused*/) noexcept;

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct anycast_exception
 * @brief any转换异常
 */
struct anycast_exception final : typecast_exception {
    explicit anycast_exception(const char* info = "Cast From any Type Failed.") noexcept :
    typecast_exception(info) {}

    explicit anycast_exception(const exception& e) :
    typecast_exception(e) {}

    ~anycast_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "anycast_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup Any 任意类
 * @brief 任意类型及其工具函数的实现
 * @{
 */

/**
 * @class any
 * @brief 任意类型容器
 *
 * 可以在运行时存储任何类型的值，提供类型安全的访问机制。
 * 支持复制、移动、类型查询和类型转换操作。
 */
class NEFORCE_API any {
    /**
     * @union storage_internal
     * @brief 内部存储联合体
     *
     * 用于存储值，可以是栈存储或堆存储。
     */
    union storage_internal {
        storage_internal() = default;
        storage_internal(const storage_internal&) = delete;
        storage_internal& operator=(const storage_internal&) = delete;

        void* ptr_ = nullptr;                                    ///< 堆存储指针
        aligned_storage_t<sizeof(ptr_), alignof(void*)> buffer_; ///< 栈存储缓冲区
    };

    /**
     * @enum any_operation
     * @brief any操作类型枚举
     *
     * 定义any内部分发时的操作类型。
     */
    enum any_operation {
        ACCESS,        ///< 访问操作
        GET_TYPE_INFO, ///< 获取类型信息操作
        COPY,          ///< 复制操作
        DESTROY,       ///< 销毁操作
        SWAP           ///< 交换操作
    };

    /**
     * @union ArgT
     * @brief 参数传递联合体
     *
     * 用于在管理函数间传递不同类型的数据。
     */
    union ArgT {
        void* obj_ptr_;                  ///< 对象指针
        const std::type_info* type_ptr_; ///< 类型信息指针
        any* any_ptr_;                   ///< any指针
    };

    /**
     * @struct internal_manage
     * @brief 内部管理器模板
     * @tparam T 管理值的类型
     *
     * 管理栈存储的值，使用inplace new在缓冲区中构造对象。
     */
    template <typename T>
    struct internal_manage {
        /**
         * @brief 管理函数
         * @param op 操作类型
         * @param value 操作的any对象
         * @param arg 参数
         */
        static void manage(any_operation op, const any* value, ArgT* arg);

        /**
         * @brief 创建值
         * @tparam Args 参数类型
         * @param storage 存储对象
         * @param args 构造参数
         */
        template <typename... Args>
        static void create(storage_internal& storage, Args&&... args) {
            void* ptr = &storage.buffer_;
            new (ptr) T(_NEFORCE forward<Args>(args)...);
        }

        /**
         * @brief 访问值
         * @param storage 存储对象
         * @return 指向值的指针
         */
        static T* access(const storage_internal& storage) {
            const void* ptr = &storage.buffer_;
            return static_cast<T*>(const_cast<void*>(ptr));
        }
    };

    /**
     * @struct external_manage
     * @brief 外部管理器模板
     * @tparam T 管理值的类型
     *
     * 管理堆分配的值，在堆上分配内存存储对象。
     */
    template <typename T>
    struct external_manage {
        /**
         * @brief 管理函数
         * @param op 操作类型
         * @param value 操作的any对象
         * @param arg 参数
         */
        static void manage(any_operation op, const any* value, ArgT* arg);

        /**
         * @brief 创建值
         * @tparam Args 参数类型
         * @param storage 存储对象
         * @param args 构造参数
         */
        template <typename... Args>
        static void create(storage_internal& storage, Args&&... args) {
            storage.ptr_ = new T(_NEFORCE forward<Args>(args)...);
        }

        /**
         * @brief 访问值
         * @param storage 存储对象
         * @return 指向值的指针
         */
        static T* access(const storage_internal& storage) { return static_cast<T*>(storage.ptr_); }
    };

    /**
     * @brief 管理器类型选择
     * @tparam T 值类型
     *
     * 根据类型特性选择栈存储或堆存储管理。
     */
    template <typename T>
    using manage_t = conditional_t<is_nothrow_move_constructible_v<T> && sizeof(T) <= sizeof(storage_internal) &&
                                           alignof(T) <= alignof(storage_internal),
                                   internal_manage<T>, external_manage<T>>;

    using manage_func = void (*)(any_operation, const any*, ArgT*); ///< 管理器函数指针类型

    manage_func manage_ = nullptr; ///< 管理器函数指针
    storage_internal storage_;     ///< 存储对象

    template <typename T, typename U>
    friend const T* inner::__any_cast_aux_dispatch_impl(const any* value, inner::any_cast_true_tag /*unused*/) noexcept;

    /**
     * @brief 尝试构造值
     * @tparam T 值类型
     * @tparam Args 参数类型
     * @tparam Manager 管理器类型
     * @param args 构造参数
     */
    template <typename T, typename... Args, typename Manager = manage_t<T>>
    void try_emplace(Args&&... args) {
        any::reset();
        Manager::create(storage_, _NEFORCE forward<Args>(args)...);
        manage_ = &Manager::manage;
    }

    /**
     * @brief 尝试使用初始化列表构造值
     * @tparam T 值类型
     * @tparam U 初始化列表元素类型
     * @tparam Args 参数类型
     * @tparam Manager 管理器类型
     * @param ilist 初始化列表
     * @param args 构造参数
     */
    template <typename T, typename U, typename... Args, typename Manager = manage_t<T>>
    void try_emplace(std::initializer_list<U> ilist, Args&&... args) {
        any::reset();
        Manager::create(storage_, ilist, _NEFORCE forward<Args>(args)...);
        manage_ = &Manager::manage;
    }

public:
    /**
     * @brief 默认构造函数
     */
    any() noexcept = default;

    /**
     * @brief 复制构造函数
     * @param other 源any对象
     */
    any(const any& other);

    /**
     * @brief 复制赋值运算符
     * @param other 源any对象
     *
     * @return 当前对象的引用
     */
    any& operator=(const any& other) {
        *this = any(other);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源any对象
     */
    any(any&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源any对象
     * @return 当前对象的引用
     */
    any& operator=(any&& other) noexcept;

    /**
     * @brief 从值构造
     * @tparam T 值类型
     * @param value 要存储的值
     */
    template <typename T, typename VT = decay_t<T>, typename Manager = manage_t<VT>,
              enable_if_t<is_copy_constructible_v<VT> && !is_same_v<inplace_construct_tag, VT> && !is_same_v<VT, any>,
                          int> = 0>
    explicit any(T&& value) :
    manage_(&Manager::manage) {
        Manager::create(storage_, _NEFORCE forward<T>(value));
    }

    /**
     * @brief 从值赋值
     * @tparam T 值类型
     * @param value 要存储的值
     * @return 当前对象的引用
     */
    template <typename T, typename VT = decay_t<T>,
              enable_if_t<!is_same_v<VT, any> && is_copy_constructible_v<VT>, int> = 0>
    any& operator=(T&& value) {
        *this = any(_NEFORCE forward<T>(value));
        return *this;
    }

    /**
     * @brief 就地构造
     * @tparam T 值类型
     * @tparam Args 参数类型
     * @param args 构造参数
     */
    template <typename T, typename... Args, typename VT = decay_t<T>, typename Manager = manage_t<VT>,
              enable_if_t<conjunction_v<is_copy_constructible<VT>, is_constructible<VT, Args&&...>>, int> = 0>
    explicit any(pass_template_construct_tag<T> /*unused*/, Args&&... args) :
    manage_(&Manager::manage) {
        Manager::create(storage_, _NEFORCE forward<Args>(args)...);
    }

    template <typename T, typename U, typename... Args, typename VT = decay_t<T>, typename Manager = manage_t<VT>,
              enable_if_t<conjunction_v<is_copy_constructible<VT>,
                                        is_constructible<VT, std::initializer_list<U>&, Args&&...>>,
                          int> = 0>
    explicit any(pass_template_construct_tag<T> /*unused*/, std::initializer_list<U> ilist, Args&&... args) :
    manage_(&Manager::manage) {
        Manager::create(storage_, ilist, _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 析构函数
     */
    ~any() { reset(); }

    /**
     * @brief 就地构造值
     * @tparam T 值类型
     * @tparam Args 参数类型
     * @param args 构造参数
     * @return 构造的值的引用
     */
    template <typename T, typename... Args, typename DT = decay_t<T>,
              enable_if_t<conjunction_v<is_copy_constructible<DT>, is_constructible<DT, Args&&...>>, int> = 0>
    DT& emplace(Args&&... args) {
        any::try_emplace<DT>(_NEFORCE forward<Args>(args)...);
        return *manage_t<DT>::access(storage_);
    }

    /**
     * @brief 使用初始化列表就地构造
     * @tparam T 值类型
     * @tparam U 初始化列表元素类型
     * @tparam Args 参数类型
     * @param ilist 初始化列表
     * @param args 构造参数
     * @return 构造的值的引用
     */
    template <typename T, typename U, typename... Args, typename DT = decay_t<T>,
              enable_if_t<conjunction_v<is_copy_constructible<DT>,
                                        is_constructible<DT, std::initializer_list<U>&, Args&&...>>,
                          int> = 0>
    DT& emplace(std::initializer_list<U> ilist, Args&&... args) {
        any::try_emplace<DT, U>(ilist, _NEFORCE forward<Args>(args)...);
        return *manage_t<DT>::access(storage_);
    }

    /**
     * @brief 重置any对象为空
     */
    void reset() noexcept {
        if (has_value()) {
            manage_(DESTROY, this, nullptr);
            manage_ = nullptr;
        }
    }

    /**
     * @brief 检查是否包含值
     * @return 是否包含值
     */
    NEFORCE_NODISCARD bool has_value() const noexcept { return manage_ != nullptr; }

    /**
     * @brief 获取存储值的类型信息
     * @return 类型信息引用
     */
    NEFORCE_NODISCARD const std::type_info& type() const noexcept;

    /**
     * @brief 交换两个any对象
     * @param rhs 要交换的any对象
     */
    void swap(any& rhs) noexcept;
};

/**
 * @brief 创建any对象
 * @tparam T 值类型
 * @tparam Args 参数类型
 * @param args 构造参数
 * @return 构造的any对象
 */
template <typename T, typename... Args,
          enable_if_t<is_constructible_v<any, pass_template_construct_tag<T>, Args...>, int> = 0>
any make_any(Args&&... args) {
    return any(pass_template_construct_tag<T>{}, _NEFORCE forward<Args>(args)...);
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename T, typename U>
const T* __any_cast_aux_dispatch_impl(const any* value, any_cast_true_tag /*unused*/) noexcept {
    if (value->manage_ == &any::manage_t<U>::manage) {
        return static_cast<const T*>(any::manage_t<U>::access(value->storage_));
    }
    return nullptr;
}

template <typename T, typename U>
const T* __any_cast_aux_dispatch_impl(const any* /*unused*/, any_cast_false_tag /*unused*/) noexcept {
    return nullptr;
}

template <typename T, typename U>
const T* __any_cast_aux_dispatch(const any* value) noexcept {
    using tag = conditional_t<(is_same_v<decay_t<U>, U> || is_copy_constructible_v<U>), any_cast_true_tag,
                              any_cast_false_tag>;
    return inner::__any_cast_aux_dispatch_impl<T, U>(value, tag{});
}

template <typename T, enable_if_t<is_object_v<T>, int> = 0>
const T* __any_cast_aux(const any* value) noexcept {
    if (value) {
        return __any_cast_aux_dispatch<T, remove_cv_t<T>>(value);
    }
    return nullptr;
}

template <typename T, enable_if_t<!is_object_v<T>, int> = 0>
const T* __any_cast_aux(const any* /*unused*/) noexcept {
    return nullptr;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 从any对象转换常量值
 * @tparam T 目标类型
 * @param value 指向any的指针
 * @return 指向转换后值的常量指针，如果类型不匹配则返回nullptr
 */
template <typename T>
const T* any_cast(const any* value) noexcept {
    return inner::__any_cast_aux<T>(value);
}

/**
 * @brief 从any对象转换值
 * @tparam T 目标类型
 * @param value 指向any的指针
 * @return 指向转换后值的指针，如果类型不匹配则返回nullptr
 */
template <typename T>
T* any_cast(any* value) noexcept {
    return const_cast<T*>(any_cast<T>(const_cast<const any*>(value)));
}

/**
 * @brief 从any对象转换值
 * @tparam T 目标类型
 * @param value any对象
 * @return 转换后的值
 * @throw anycast_exception 如果类型转换失败
 */
template <typename T>
T any_cast(const any& value) {
    using U = remove_cvref_t<T>;
    static_assert(disjunction_v<is_reference<T>, is_copy_constructible<T>, is_constructible<T, const U&>>,
                  "type T must be valid to cast from any.");

    auto ptr = any_cast<U>(&value);
    if (ptr != nullptr) {
        return static_cast<T>(*ptr);
    }
    NEFORCE_THROW_EXCEPTION(anycast_exception());
    unreachable();
}


/// @cond

template <typename T>
void any::internal_manage<T>::manage(const any_operation op, const any* value, ArgT* arg) {
    auto ptr = reinterpret_cast<const T*>(&value->storage_.buffer_);
    switch (op) {
        case ACCESS: {
            arg->obj_ptr_ = const_cast<T*>(ptr);
            break;
        }
        case GET_TYPE_INFO: {
            arg->type_ptr_ = &typeid(T);
            break;
        }
        case COPY: {
            ::new (&arg->any_ptr_->storage_.buffer_) T(*ptr);
            arg->any_ptr_->manage_ = value->manage_;
            break;
        }
        case DESTROY: {
            ptr->~T();
            break;
        }
        case SWAP: {
            ::new (&arg->any_ptr_->storage_.buffer_) T(_NEFORCE move(*const_cast<T*>(ptr)));
            ptr->~T();
            arg->any_ptr_->manage_ = value->manage_;
            const_cast<any*>(value)->manage_ = nullptr;
            break;
        }
        default: {
            unreachable();
        }
    }
}

template <typename T>
void any::external_manage<T>::manage(const any_operation op, const any* value, ArgT* arg) {
    auto ptr = static_cast<const T*>(value->storage_.ptr_);
    switch (op) {
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
        default: {
            unreachable();
        }
    }
}

/// @endcond

/** @} */ // Any

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_ANY_HPP__
