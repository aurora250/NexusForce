#ifndef NEFORCE_CORE_REFLECT_ANY_HPP__
#define NEFORCE_CORE_REFLECT_ANY_HPP__

/**
 * @file any.hpp
 * @brief 类型擦除容器
 *
 * 此文件提供了 meta_any 类型的实现，用于存储任意类型的值，
 * 支持类型安全的存取操作，是反射系统的核心基础。
 */

#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

using type_id = size_t; ///< 类型标识符


/**
 * @struct type_name
 * @brief 类型名称获取器
 * @tparam T 目标类型
 *
 * 提供编译期类型名称字符串。算术类型已通过宏自动特化。
 */
template <typename T>
struct type_name {
    static constexpr string_view value = "unknown";
};

/**
 * @brief type_name 的便捷访问变量模板
 */
template <typename T>
NEFORCE_INLINE17 constexpr string_view type_name_v = type_name<T>::value;

/// @cond
#define __NEFORCE_SPECIALIZE_TYPE_NAME(T)        \
    template <>                                  \
    struct type_name<T> {                        \
        static constexpr string_view value = #T; \
    };

NEFORCE_MACRO_RANGE_ARITHMETIC(__NEFORCE_SPECIALIZE_TYPE_NAME)
#undef __NEFORCE_SPECIALIZE_TYPE_NAME
/// @endcond


/**
 * @brief 计算类型 T 的运行时期类型标识
 */
template <typename T>
NEFORCE_NODISCARD constexpr type_id type_id_for() noexcept {
    if (type_name_v<T>.to_hash() != string_view("unknown").to_hash()) {
        return type_name_v<T>.to_hash();
    } else {
#ifdef NEFORCE_COMPILER_MSVC
        return FNV_hash_string(__FUNCSIG__, sizeof(__FUNCSIG__) - 1);
#else
        return FNV_hash_string(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1);
#endif
    }
}


/**
 * @class meta_any
 * @brief 类型擦除容器
 *
 * 可以存储任意类型的值，并提供类型安全的存取接口。
 */
class meta_any {
public:
    static constexpr size_t SBO_SIZE = sizeof(void*) * 2; ///< SBO 缓冲区大小

private:
    /**
     * @union storage_internal
     * @brief 内部存储联合体
     *
     * 小对象存储在 buffer_ 中，大对象通过 ptr_ 指向堆内存。
     */
    union storage_internal {
        storage_internal() noexcept = default;
        storage_internal(const storage_internal&) = delete;
        storage_internal& operator=(const storage_internal&) = delete;

        void* ptr_ = nullptr;                                               ///< 堆存储指针
        aligned_storage_t<SBO_SIZE, alignof(_NEFORCE max_align_t)> buffer_; ///< SBO 栈缓冲区
    };

    /**
     * @enum operation
     * @brief 管理器操作类型枚举
     */
    enum class operation : uint8_t {
        ACCESS,      ///< 获取值指针
        GET_TYPE_ID, ///< 获取类型 ID
        COPY,        ///< 深拷贝到另一个 meta_any
        DESTROY,     ///< 销毁存储的值
        MOVE,        ///< 移动构造到另一个 meta_any
    };

    /**
     * @union arg_t
     * @brief 操作参数联合体
     */
    union arg_t {
        void* obj_ptr_{};              ///< ACCESS 输出值指针
        meta_any* any_ptr_;            ///< COPY/MOVE 目标 meta_any 指针
        reflect::type_id type_id_val_; ///< GET_TYPE_ID 输出类型 ID
    };

    using manage_func = void (*)(operation, const meta_any*, arg_t*); ///< 管理器函数指针类型

    /**
     * @struct internal_manage
     * @brief SBO 栈存储管理器
     * @tparam T 存储的值类型
     */
    template <typename T>
    struct internal_manage {
        NEFORCE_NODISCARD static reflect::type_id type_id_val() noexcept { return type_id_for<T>(); }

        template <typename... Args>
        static void create(storage_internal& storage, Args&&... args) {
            void* addr = const_cast<void*>(static_cast<const void*>(&storage.buffer_));
            ::new (addr) T(_NEFORCE forward<Args>(args)...);
        }

        static T* access(const storage_internal& storage) noexcept {
            const void* addr = &storage.buffer_;
            return static_cast<T*>(const_cast<void*>(addr));
        }

        template <typename U = T, enable_if_t<is_copy_constructible_v<U>, int> = 0>
        static void copy_op(const meta_any* self, arg_t* arg) {
            auto* ptr = access(self->storage_);
            create(arg->any_ptr_->storage_, *ptr);
            arg->any_ptr_->manage_ = &manage;
            arg->any_ptr_->type_id_ = type_id_val();
        }

        template <typename U = T, enable_if_t<!is_copy_constructible_v<U>, int> = 0>
        static void copy_op(const meta_any* /*unused*/, arg_t* /*unused*/) {}

        template <typename U = T, enable_if_t<is_move_constructible_v<U>, int> = 0>
        static void move_op(const meta_any* self, arg_t* arg) {
            auto* ptr = access(self->storage_);
            create(arg->any_ptr_->storage_, _NEFORCE move(*ptr));
            ptr->~T();
            arg->any_ptr_->manage_ = &manage;
            arg->any_ptr_->type_id_ = type_id_val();
        }

        template <typename U = T, enable_if_t<!is_move_constructible_v<U>, int> = 0>
        static void move_op(const meta_any* /*unused*/, arg_t* /*unused*/) {}

        static void manage(const operation op, const meta_any* self, arg_t* arg) {
            auto* ptr = access(self->storage_);
            switch (op) {
                case operation::ACCESS: {
                    arg->obj_ptr_ = static_cast<void*>(ptr);
                    break;
                }
                case operation::GET_TYPE_ID: {
                    arg->type_id_val_ = type_id_val();
                    break;
                }
                case operation::COPY: {
                    copy_op(self, arg);
                    break;
                }
                case operation::DESTROY: {
                    ptr->~T();
                    break;
                }
                case operation::MOVE: {
                    move_op(self, arg);
                    break;
                }
            }
        }
    };

    /**
     * @struct external_manage
     * @brief 堆存储管理器
     * @tparam T 存储的值类型
     */
    template <typename T>
    struct external_manage {
        NEFORCE_NODISCARD static reflect::type_id type_id_val() noexcept { return type_id_for<T>(); }

        template <typename... Args>
        static void create(storage_internal& storage, Args&&... args) {
            storage.ptr_ = new T(_NEFORCE forward<Args>(args)...);
        }

        static T* access(const storage_internal& storage) noexcept { return static_cast<T*>(storage.ptr_); }

        template <typename U = T, enable_if_t<is_copy_constructible_v<U>, int> = 0>
        static void copy_op(const meta_any* self, arg_t* arg) {
            auto* ptr = access(self->storage_);
            create(arg->any_ptr_->storage_, *ptr);
            arg->any_ptr_->manage_ = &manage;
            arg->any_ptr_->type_id_ = type_id_val();
        }

        template <typename U = T, enable_if_t<!is_copy_constructible_v<U>, int> = 0>
        static void copy_op(const meta_any* /*unused*/, arg_t* /*unused*/) {}

        static void manage(const operation op, const meta_any* self, arg_t* arg) {
            auto* ptr = access(self->storage_);
            switch (op) {
                case operation::ACCESS: {
                    arg->obj_ptr_ = static_cast<void*>(ptr);
                    break;
                }
                case operation::GET_TYPE_ID: {
                    arg->type_id_val_ = type_id_val();
                    break;
                }
                case operation::COPY: {
                    copy_op(self, arg);
                    break;
                }
                case operation::DESTROY: {
                    delete ptr;
                    break;
                }
                case operation::MOVE: {
                    arg->any_ptr_->storage_.ptr_ = self->storage_.ptr_;
                    arg->any_ptr_->manage_ = &manage;
                    arg->any_ptr_->type_id_ = type_id_val();
                    break;
                }
            }
        }
    };

    /**
     * @brief SBO 阈值选择
     */
    template <typename T>
    struct use_internal_storage : bool_constant<is_nothrow_move_constructible_v<T> && sizeof(T) <= SBO_SIZE &&
                                                alignof(T) <= alignof(storage_internal)> {};

    template <typename T>
    using manage_t = conditional_t<use_internal_storage<T>::value, internal_manage<T>, external_manage<T>>;

    manage_func manage_ = nullptr; ///< 管理器函数指针
    storage_internal storage_;     ///< 存储对象
    reflect::type_id type_id_ = 0; ///< 存储值的类型 ID

    void reset_internal() noexcept {
        if (manage_ != nullptr) {
            manage_(operation::DESTROY, this, nullptr);
            manage_ = nullptr;
        }
        type_id_ = 0;
    }

    template <typename T, typename... Args, typename Manager = manage_t<decay_t<T>>>
    void emplace_impl(Args&&... args) {
        reset_internal();
        Manager::create(storage_, _NEFORCE forward<Args>(args)...);
        manage_ = &Manager::manage;
        type_id_ = Manager::type_id_val();
    }

public:
    /**
     * @brief 默认构造函数
     */
    meta_any() noexcept = default;

    /**
     * @brief 从任意值构造
     * @tparam T 值类型
     * @param value 要存储的值
     */
    template <typename T, typename DecayT = decay_t<T>, typename = enable_if_t<!is_same_v<DecayT, meta_any>>,
              typename = enable_if_t<is_copy_constructible_v<DecayT>>>
    meta_any(T&& value) {
        emplace_impl<T>(_NEFORCE forward<T>(value));
    }

    /**
     * @brief 原地构造值
     * @tparam T 值类型
     * @tparam Args 构造函数参数类型
     * @param args 构造函数参数
     *
     * 在 meta_any 存储空间内直接构造 T，无需拷贝或移动。
     */
    template <typename T, typename... Args>
    void emplace(Args&&... args) {
        emplace_impl<T>(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源对象
     */
    meta_any(const meta_any& other) {
        if (other.manage_ != nullptr) {
            arg_t arg;
            arg.any_ptr_ = this;
            other.manage_(operation::COPY, &other, &arg);
        }
    }

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    meta_any(meta_any&& other) noexcept {
        if (other.manage_ != nullptr) {
            arg_t arg;
            arg.any_ptr_ = this;
            other.manage_(operation::MOVE, &other, &arg);
            other.manage_ = nullptr;
            other.type_id_ = 0;
        }
    }

    /**
     * @brief 析构函数
     */
    ~meta_any() { reset_internal(); }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    meta_any& operator=(const meta_any& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        reset_internal();
        if (other.manage_ != nullptr) {
            arg_t arg;
            arg.any_ptr_ = this;
            other.manage_(operation::COPY, &other, &arg);
        }
        return *this;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    meta_any& operator=(meta_any&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        reset_internal();
        if (other.manage_ != nullptr) {
            arg_t arg;
            arg.any_ptr_ = this;
            other.manage_(operation::MOVE, &other, &arg);
            other.manage_ = nullptr;
            other.type_id_ = 0;
        }
        return *this;
    }

    /**
     * @brief 获取存储值的类型 ID
     * @return 类型 ID，空对象返回 0
     */
    NEFORCE_NODISCARD reflect::type_id type_id() const noexcept { return type_id_; }

    /**
     * @brief 检查是否包含值
     * @return 包含值返回 true
     */
    NEFORCE_NODISCARD bool has_value() const noexcept { return manage_ != nullptr; }

    /**
     * @brief 布尔转换运算符
     * @return 包含值返回 true
     */
    explicit operator bool() const noexcept { return has_value(); }

    /**
     * @brief 尝试转换为指定类型的指针
     * @tparam T 目标类型
     * @return 类型匹配返回指针，否则返回 nullptr
     */
    template <typename T>
    NEFORCE_NODISCARD T* cast() noexcept {
        if (!manage_ || type_id_ != type_id_for<T>()) {
            return nullptr;
        }
        arg_t arg;
        manage_(operation::ACCESS, this, &arg);
        return static_cast<T*>(arg.obj_ptr_);
    }

    /**
     * @brief 尝试转换为指定类型的常量指针
     * @tparam T 目标类型
     * @return 类型匹配返回指针，否则返回 nullptr
     */
    template <typename T>
    NEFORCE_NODISCARD const T* cast() const noexcept {
        if (!manage_ || type_id_ != type_id_for<T>()) {
            return nullptr;
        }
        arg_t arg;
        manage_(operation::ACCESS, this, &arg);
        return static_cast<const T*>(arg.obj_ptr_);
    }

    /**
     * @brief 获取存储值的引用
     * @tparam T 目标类型
     * @return 存储值的引用
     * @throws typecast_exception 类型不匹配时抛出
     */
    template <typename T>
    NEFORCE_NODISCARD T& get() {
        if (auto* ptr = cast<T>()) {
            return *ptr;
        }
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    /**
     * @brief 获取存储值的常量引用
     * @tparam T 目标类型
     * @return 存储值的常量引用
     * @throws typecast_exception 类型不匹配时抛出
     */
    template <typename T>
    NEFORCE_NODISCARD const T& get() const {
        if (auto* ptr = cast<T>()) {
            return *ptr;
        }
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    /**
     * @brief 检查是否可以转换为指定类型
     * @tparam T 目标类型
     * @return 可以转换返回 true
     */
    template <typename T>
    NEFORCE_NODISCARD bool can_cast() const noexcept {
        return cast<T>() != nullptr;
    }

    /**
     * @brief 转换为指定类型的值
     * @tparam T 目标类型
     * @return 转换后的值
     * @throws typecast_exception 类型不匹配时抛出
     */
    template <typename T>
    NEFORCE_NODISCARD T convert() const {
        if (auto* ptr = cast<T>()) {
            return *ptr;
        }
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    /**
     * @brief 获取存储值的原始指针（不检查类型）
     * @return 原始对象指针，空返回 nullptr
     */
    NEFORCE_NODISCARD void* raw() noexcept {
        if (manage_ == nullptr) {
            return nullptr;
        }
        arg_t arg;
        manage_(operation::ACCESS, this, &arg);
        return arg.obj_ptr_;
    }

    /**
     * @brief 获取存储值的常量原始指针（不检查类型）
     */
    NEFORCE_NODISCARD const void* raw() const noexcept {
        if (manage_ == nullptr) {
            return nullptr;
        }
        arg_t arg;
        manage_(operation::ACCESS, this, &arg);
        return arg.obj_ptr_;
    }

    /**
     * @brief 重置为空状态
     */
    void reset() noexcept { reset_internal(); }

    /**
     * @brief 交换两个 meta_any
     * @param other 要交换的对象
     */
    void swap(meta_any& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return;
        }
        meta_any tmp(_NEFORCE move(*this));
        *this = _NEFORCE move(other);
        other = _NEFORCE move(tmp);
    }
};

/**
 * @brief 交换两个 meta_any
 * @param lhs 左操作数
 * @param rhs 右操作数
 */
inline void swap(meta_any& lhs, meta_any& rhs) noexcept { lhs.swap(rhs); }

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_ANY_HPP__
