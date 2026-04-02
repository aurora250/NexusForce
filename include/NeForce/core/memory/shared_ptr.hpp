#ifndef NEFORCE_CORE_MEMORY_SHARED_PTR_HPP__
#define NEFORCE_CORE_MEMORY_SHARED_PTR_HPP__

/**
 * @file shared_ptr.hpp
 * @brief 共享智能指针实现
 *
 * 此文件提供了共享智能指针的完整实现，
 * 支持引用计数、自定义删除器、分配器等功能。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/memory/allocator_traits.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include <new>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SharedPointer 共享智能指针
 * @brief 共享智能指针类和辅助工具
 * @{
 */

/// @cond
NEFORCE_BEGIN_INNER__

template <typename T>
class smart_pointer_atomic;


/**
 * @struct __smart_ptr_counter
 * @brief 智能指针计数器基类
 *
 * 管理共享指针和弱指针的引用计数，提供原子操作保证线程安全。
 */
struct __smart_ptr_counter {
public:
    atomic<unsigned long> strong_count_;  ///< 强引用计数
    atomic<unsigned long> weak_count_;    ///< 弱引用计数

protected:
    /**
     * @brief 删除管理的对象
     */
    virtual void delete_object() noexcept = 0;

    /**
     * @brief 删除控制块自身
     */
    virtual void delete_this() noexcept = 0;

public:
    /**
     * @brief 构造函数
     * @note 初始强引用计数为1（对象存在），弱引用计数为1（控制块存在）
     */
    __smart_ptr_counter() noexcept
    : strong_count_(1), weak_count_(1) {}

    __smart_ptr_counter(__smart_ptr_counter&&) = delete;  ///< 禁止移动构造

    virtual ~__smart_ptr_counter() = default;  ///< 虚析构函数

    /**
     * @brief 增加强引用计数
     */
    void incref_strong() noexcept {
        strong_count_.fetch_add(1, memory_order_relaxed);
    }

    /**
     * @brief 增加弱引用计数
     */
    void incref_weak() noexcept {
        weak_count_.fetch_add(1, memory_order_relaxed);
    }

    /**
     * @brief 减少强引用计数
     * @note 当强引用计数为0时删除对象，并减少弱引用计数
     */
    void decref_strong() noexcept {
        if (strong_count_.fetch_sub(1, memory_order_acq_rel) == 1) {
            delete_object();
            decref_weak();
        }
    }

    /**
     * @brief 减少弱引用计数
     * @note 当弱引用计数为0时删除控制块
     */
    void decref_weak() noexcept {
        if (weak_count_.fetch_sub(1, memory_order_acq_rel) == 1) {
            delete_this();
        }
    }

    /**
     * @brief 尝试增加强引用计数
     * @return 是否成功增加（当强引用计数不为0时）
     *
     * @note 用于从弱指针升级到强指针的场景
     */
    bool try_incref_strong() noexcept {
        auto strong = strong_count_.load(memory_order_relaxed);
        do {
            if (strong == 0) return false;
        } while (!strong_count_.compare_exchange_weak(
            strong, strong + 1,
            memory_order_release,
            memory_order_relaxed
        ));
        return true;
    }

    /**
     * @brief 获取强引用计数
     * @return 强引用计数值
     */
    NEFORCE_NODISCARD uint64_t use_count() const noexcept {
        return strong_count_.load(memory_order_relaxed);
    }

    /**
     * @brief 获取弱引用计数
     * @return 弱引用计数值
     */
    NEFORCE_NODISCARD uint64_t weak_count() const noexcept {
        return weak_count_.load(memory_order_relaxed);
    }
};

/**
 * @brief 智能指针计数器实现（分离分配）
 * @tparam T 对象类型
 * @tparam Deleter 删除器类型
 *
 * 对象和控制块分别分配内存的计数器实现。
 */
template <typename T, typename Deleter>
struct __smart_ptr_counter_impl final : __smart_ptr_counter {
    compressed_pair<Deleter, T*> ptr_pair_{default_construct_tag{}, nullptr};

    explicit __smart_ptr_counter_impl(T* ptr)
    noexcept(is_nothrow_default_constructible_v<Deleter>)
    : ptr_pair_(default_construct_tag{}, ptr) {}

    explicit __smart_ptr_counter_impl(T* ptr, Deleter&& deleter)
    noexcept(is_nothrow_move_constructible_v<Deleter>)
    : ptr_pair_(exact_arg_construct_tag{}, _NEFORCE move(deleter), ptr) {}

    void delete_object() noexcept override {
        ptr_pair_.get_base()(ptr_pair_.value);
        ptr_pair_.value = nullptr;
    }

    void delete_this() noexcept override {
        delete this;
    }
};

/**
 * @brief 智能指针计数器实现（融合分配）
 * @tparam T 对象类型
 * @tparam Deleter 删除器类型
 *
 * 对象和控制块分配在同一块内存中的计数器实现，提高内存局部性。
 */
template <typename T, typename Deleter>
struct __smart_ptr_counter_impl_fused final : __smart_ptr_counter {
    compressed_pair<Deleter, T*> ptr_pair_{default_construct_tag{}, nullptr};
    size_t align_;
    void* mem_;

    explicit __smart_ptr_counter_impl_fused(T* ptr, void* mem, size_t align, Deleter deleter) noexcept
    : ptr_pair_(exact_arg_construct_tag{}, _NEFORCE move(deleter), ptr), align_(align), mem_(mem) {}

    void delete_object() noexcept override {
        ptr_pair_.get_base()(ptr_pair_.value);
        ptr_pair_.value = nullptr;
    }

    void delete_this() noexcept override {
#if NEFORCE_STANDARD_17
        operator delete(mem_, std::align_val_t{ align_ });
#else
        operator delete(mem_);
#endif
    }
};

/**
 * @brief 智能指针计数器实现（带分配器）
 * @tparam T 对象类型
 * @tparam Deleter 删除器类型
 * @tparam Alloc 分配器类型
 *
 * 使用分配器进行内存管理的计数器实现。
 */
template <typename T, typename Deleter, typename Alloc>
struct __smart_ptr_counter_impl_allocated final : __smart_ptr_counter {
    compressed_pair<Deleter, T*> ptr_pair_{default_construct_tag{}, nullptr};
    compressed_pair<Alloc, size_t> size_pair_{default_construct_tag{}, 0};
    void* mem_;

    explicit __smart_ptr_counter_impl_allocated(
        T* ptr, void* mem, const size_t size,
        Deleter deleter, Alloc alloc) noexcept
    : ptr_pair_(exact_arg_construct_tag{}, _NEFORCE move(deleter), ptr),
      size_pair_(exact_arg_construct_tag{}, _NEFORCE move(alloc), size),
      mem_(mem) {}

    void delete_object() noexcept override {
        ptr_pair_.get_base()(ptr_pair_.value);
        ptr_pair_.value = nullptr;
    }

    void delete_this() noexcept override {
        using alloc_traits = allocator_traits<Alloc>;
        using byte_allocator = typename alloc_traits::template alloc_rebind_t<Alloc, byte_t>;
        byte_allocator byte_alloc(size_pair_.get_base());
        allocator_traits<byte_allocator>::deallocate(byte_alloc, static_cast<byte_t*>(mem_), size_pair_.value);
    }
};

NEFORCE_END_INNER__
/// @endcond


template <typename T>
struct enable_shared_from_this;

template <typename T>
class shared_ptr;

template <typename T>
class weak_ptr;

/// @cond
NEFORCE_BEGIN_INNER__

template <typename T>
void __setup_enable_shared_from_impl(T* ptr, __smart_ptr_counter* owner, true_type) noexcept {
    if (ptr) static_cast<enable_shared_from_this<T>*>(ptr)->owner_ = owner;
}

template <typename T>
void __setup_enable_shared_from_impl(T*, __smart_ptr_counter*, false_type) noexcept {}

template <typename T>
void __setup_enable_shared_from(T* ptr, __smart_ptr_counter* owner) noexcept {
    inner::__setup_enable_shared_from_impl(ptr, owner, is_base_of<enable_shared_from_this<T>, T>{});
}

template <typename T>
shared_ptr<T> __make_shared_fused(T* ptr, __smart_ptr_counter* owner) noexcept {
    return shared_ptr<T>(ptr, owner);
}

NEFORCE_END_INNER__
/// @endcond


/**
 * @class shared_ptr
 * @brief 共享智能指针类模板
 * @tparam T 对象类型
 *
 * 实现引用计数的智能指针，多个实例可以共享同一对象的所有权。
 * 当最后一个共享智能指针被销毁时，对象会被自动删除。
 */
template <typename T>
class shared_ptr {
public:
    using element_type = T;  ///< 元素类型

private:
    using owner_type = inner::__smart_ptr_counter;

    template <typename U, typename Deleter>
    using owner_deleter = inner::__smart_ptr_counter_impl<U, Deleter>;

    template <typename U>
    using owner_default = inner::__smart_ptr_counter_impl<U, default_delete<U>>;

    element_type* ptr_ = nullptr;   ///< 管理的对象指针
    owner_type* owner_ = nullptr;  ///< 控制块指针

    /**
     * @brief 私有构造函数
     * @param ptr 对象指针
     * @param owner 控制块指针
     */
    explicit shared_ptr(T* ptr, owner_type* owner) noexcept
    : ptr_(ptr), owner_(owner) {}

    template <typename U>
    friend class shared_ptr;

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend class inner::smart_pointer_atomic;

    template <typename U>
    friend shared_ptr<U> inner::__make_shared_fused(U*, inner::__smart_ptr_counter*) noexcept;

public:
    /**
     * @brief 默认构造函数
     * @param np 空指针字面量
     *
     * 创建空的共享指针，不管理任何对象。
     */
    shared_ptr(nullptr_t np = nullptr) noexcept {}

    /**
     * @brief 从原始指针构造函数
     * @tparam U 可转换为T*的类型
     * @param ptr 原始指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr(U* ptr)
    : ptr_(ptr), owner_(new owner_default<U>(ptr_)) {
        inner::__setup_enable_shared_from(ptr_, owner_);
    }

    /**
     * @brief 从原始指针和自定义删除器构造函数
     * @tparam U 可转换为T*的类型
     * @tparam Deleter 删除器类型
     * @param ptr 原始指针
     * @param deleter 删除器
     */
    template <typename U, typename Deleter, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(U* ptr, Deleter&& deleter)
    : ptr_(ptr), owner_(new owner_deleter<U, Deleter>(ptr_, _NEFORCE forward<Deleter>(deleter))) {
        inner::__setup_enable_shared_from(ptr_, owner_);
    }

    /**
     * @brief 独享智能指针构造函数
     * @tparam U 可转换为T*的类型
     * @tparam Deleter 删除器类型
     * @param unique 独享智能指针
     */
    template <typename U, typename Deleter, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(unique_ptr<U, Deleter>&& unique)
    : shared_ptr(unique.release(), unique.get_deleter()) {}

    /**
     * @brief 拷贝构造函数
     * @param other 要拷贝的共享指针
     */
    shared_ptr(const shared_ptr& other) noexcept
    : ptr_(other.ptr_), owner_(other.owner_) {
        if (owner_) owner_->incref_strong();
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 要拷贝的共享指针
     * @return 当前共享指针的引用
     */
    shared_ptr& operator =(const shared_ptr& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        if (owner_) owner_->decref_strong();
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        if (owner_) owner_->incref_strong();
        return *this;
    }

    /**
     * @brief 类型转换拷贝构造函数
     * @tparam U 可转换为T*的类型
     * @param other 要拷贝的共享指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr(const shared_ptr<U>& other) noexcept
    : ptr_(other.ptr_), owner_(other.owner_) {
        if (owner_) owner_->incref_strong();
    }

    /**
     * @brief 移动构造函数
     * @param other 要移动的共享指针
     */
    shared_ptr(shared_ptr&& other) noexcept
    : ptr_(other.ptr_), owner_(other.owner_) {
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的共享指针
     * @return 当前共享指针的引用
     */
    shared_ptr& operator =(shared_ptr&& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        if (owner_) owner_->decref_strong();
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
        return *this;
    }

    /**
     * @brief 类型转换移动构造函数
     * @tparam U 可转换为T*的类型
     * @param other 要移动的共享指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(shared_ptr<U>&& other) noexcept
    : ptr_(other.ptr_), owner_(other.owner_) {
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
    }

    /**
     * @brief 从共享指针和别名指针别名构造函数
     * @tparam U 原始共享指针的类型
     * @param other 原始共享指针
     * @param ptr 别名指针
     * @note 创建的共享指针与参数共享智能指针共享所有权，但指向不同的对象
     */
    template <typename U>
    shared_ptr(const shared_ptr<U>& other, T* ptr) noexcept
    : ptr_(ptr), owner_(other.owner_) {
        if (owner_) owner_->incref_strong();
    }

    /**
     * @brief 移动别名构造函数
     */
    template <typename U>
    shared_ptr(shared_ptr<U>&& other, T* ptr) noexcept
    : ptr_(ptr), owner_(other.owner_) {
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
    }

    /**
     * @brief 类型转换拷贝赋值运算符
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr& operator =(const shared_ptr<U>& other) noexcept {
        if (owner_) owner_->decref_strong();
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        if (owner_) owner_->incref_strong();
        return *this;
    }

    /**
     * @brief 类型转换移动赋值运算符
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr& operator =(shared_ptr<U>&& other) noexcept {
        if (owner_) owner_->decref_strong();
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
        return *this;
    }

    /**
     * @brief 析构函数
     * @note 减少强引用计数，当计数为0时删除对象
     */
    ~shared_ptr() noexcept {
        reset();
    }

    /**
     * @brief 重置共享指针
     */
    void reset() noexcept {
        if (owner_) owner_->decref_strong();
        owner_ = nullptr;
        ptr_ = nullptr;
    }

    /**
     * @brief 重置共享指针并管理新对象
     * @tparam U 可转换为T*的类型
     * @param ptr 新的原始指针
     */
    template <typename U>
    void reset(U* ptr) {
        if (owner_) owner_->decref_strong();
        ptr_ = nullptr;
        owner_ = nullptr;
        ptr_ = ptr;
        owner_ = new owner_default<U>(ptr_);
        inner::__setup_enable_shared_from<T>(ptr_, owner_);
    }

    /**
     * @brief 带自定义删除器重置共享指针并管理新对象
     * @tparam U 可转换为T*的类型
     * @tparam Deleter 删除器类型
     * @param ptr 新的原始指针
     * @param deleter 删除器
     */
    template <typename U, typename Deleter>
    void reset(U* ptr, Deleter deleter) {
        if (owner_) owner_->decref_strong();
        ptr_ = nullptr;
        owner_ = nullptr;
        ptr_ = ptr;
        owner_ = new owner_deleter<U, Deleter>(ptr_, _NEFORCE move(deleter));
        inner::__setup_enable_shared_from<T>(ptr_, owner_);
    }

    /**
     * @brief 获取引用计数
     * @return 强引用计数
     */
    NEFORCE_NODISCARD long use_count() const noexcept {
        return owner_ ? owner_->use_count() : 0;
    }

    /**
     * @brief 检查是否独占所有权
     * @return 是否只有当前共享指针引用对象
     */
    NEFORCE_NODISCARD bool unique() const noexcept {
        return owner_ ? owner_->use_count() == 1 : true;
    }

    /**
     * @brief 交换两个共享指针
     * @param other 要交换的共享指针
     */
    void swap(shared_ptr& other) noexcept {
        if (_NEFORCE addressof(other) == this) return;
        _NEFORCE swap(ptr_, other.ptr_);
        _NEFORCE swap(owner_, other.owner_);
    }

    /**
     * @brief 获取原始指针
     * @return 管理的对象指针
     */
    NEFORCE_NODISCARD T* get() const noexcept {
        return ptr_;
    }

    /**
     * @brief 指针解引用运算符
     * @return 管理的对象指针
     */
    NEFORCE_NODISCARD T* operator ->() const noexcept {
        return ptr_;
    }

    /**
     * @brief 解引用运算符
     * @return 对象的左值引用
     */
    NEFORCE_NODISCARD add_lvalue_reference_t<T> operator *() const noexcept {
        return *ptr_;
    }

    /**
     * @brief 布尔转换运算符
     * @return 是否管理对象
     */
    NEFORCE_NODISCARD explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    /**
     * @brief 检查所有权是否相等
     * @tparam U 比较的共享指针类型
     * @param rhs 要比较的共享指针
     * @return 是否共享同一控制块
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_equal(const shared_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }

    /**
     * @brief 与弱指针检查所有权是否相等
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_equal(const weak_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }

    /**
     * @brief 比较所有权顺序
     * @tparam U 比较的共享指针类型
     * @param rhs 要比较的共享指针
     * @return 当前控制块地址是否小于rhs的控制块地址
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_before(const shared_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }

    /**
     * @brief 与弱指针比较所有权顺序
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_before(const weak_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }
};

/**
 * @brief 相等比较运算符
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator ==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.owner_equal(rhs);
}

/**
 * @brief 不等比较运算符
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator !=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs == rhs);
}

/**
 * @brief 小于比较运算符（基于所有权顺序）
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator <(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.owner_before(rhs);
}

/**
 * @brief 大于比较运算符
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator >(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return rhs < lhs;
}

/**
 * @brief 小于等于比较运算符
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator <=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs > rhs);
}

/**
 * @brief 大于等于比较运算符
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator >=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs < rhs);
}


/**
 * @brief 数组特化的共享指针
 * @tparam T 数组元素类型
 */
template <typename T>
class shared_ptr<T[]> : shared_ptr<T> {
public:
    using shared_ptr<T>::shared_ptr;

    /**
     * @brief 数组下标运算符
     * @param idx 索引
     * @return 数组元素的引用
     */
    add_lvalue_reference_t<T> operator [](size_t idx) {
        return this->get()[idx];
    }
};


/**
 * @struct enable_shared_from_this
 * @brief 启用从this创建共享指针的基类
 * @tparam T 派生类类型
 *
 * 允许在类的成员函数中安全地获取指向自身的共享智能指针。
 */
template <typename T>
struct enable_shared_from_this {
private:
    mutable inner::__smart_ptr_counter* owner_ = nullptr;  ///< 控制块指针

    template <typename U>
    friend void inner::__setup_enable_shared_from_impl(U* ptr, inner::__smart_ptr_counter* owner, true_type) noexcept;

    template <typename U>
    friend void inner::__setup_enable_shared_from(U*, inner::__smart_ptr_counter*) noexcept;

    template <typename U>
    friend class shared_ptr;

    template <typename U>
    friend class weak_ptr;

protected:
    /**
     * @brief 构造函数
     */
    enable_shared_from_this() noexcept {}

    /**
     * @brief 获取指向自身的共享指针
     * @return 指向当前对象的共享指针
     * @throw memory_exception 如果对象不由shared_ptr管理
     */
    shared_ptr<T> shared_from_this() {
        static_assert(is_base_of_v<enable_shared_from_this, T>, "shared from T requires derived class");
        if (!owner_) {
            NEFORCE_THROW_EXCEPTION(memory_exception("smart pointer share failed."));
        }
        owner_->incref_strong();
        return inner::__make_shared_fused(static_cast<T*>(this), owner_);
    }

    /**
     * @brief 获取指向自身的常量共享指针
     * @return 指向当前对象的常量共享指针
     * @throw memory_exception 如果对象不由shared_ptr管理
     */
    shared_ptr<const T> shared_from_this() const {
        static_assert(is_base_of_v<enable_shared_from_this, T>, "shared from T requires derived class");
        if (!owner_) {
            NEFORCE_THROW_EXCEPTION(memory_exception("smart pointer share failed."));
        }
        owner_->incref_strong();
        return inner::__make_shared_fused(static_cast<const T*>(this), owner_);
    }
};


/**
 * @brief 类型特征：是否为shared_ptr
 */
template <typename T>
struct is_shared_ptr : false_type {};

template <typename T>
struct is_shared_ptr<shared_ptr<T>> : true_type {};

template <typename T>
NEFORCE_INLINE17 constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;


/**
 * @brief 融合分配创建共享指针
 * @tparam T 对象类型
 * @tparam Args 参数类型
 * @param args 构造参数
 * @return 共享指针
 * @throw memory_exception 如果构造对象时抛出错误
 *
 * 在单块内存中同时分配控制块和对象，提高内存局部性和性能。
 */
template <typename T, typename... Args>
enable_if_t<!is_unbounded_array_v<T> && is_constructible_v<T, Args...>, shared_ptr<T>>
make_shared(Args&&... args) {
    auto const deleter =
        [](T* ptr) noexcept(is_nothrow_destructible_v<T>) {
            ptr->~T();
        };
    using Counter = inner::__smart_ptr_counter_impl_fused<T, decltype(deleter)>;
    constexpr size_t align = max(alignof(T), alignof(Counter));
    constexpr size_t offset = (sizeof(Counter) + align - 1) & ~(align - 1);
    constexpr size_t size = offset + sizeof(T);
#if NEFORCE_STANDARD_17
    void* mem = operator new(size, std::align_val_t{ align });
    auto* counter = static_cast<Counter*>(mem);
#else
    void* mem = operator new(size + align - 1);
    size_t aligned_addr = (reinterpret_cast<size_t>(mem) + (align - 1)) & ~(align - 1);
    Counter* counter = reinterpret_cast<Counter*>(aligned_addr);
#endif
    T* object = reinterpret_cast<T*>(reinterpret_cast<byte_t*>(counter) + offset);
    try {
        _NEFORCE construct(object, _NEFORCE forward<Args>( args)...);
    } catch (...) {
#if NEFORCE_STANDARD_17
        operator delete(mem, std::align_val_t{ align });
#else
        operator delete(mem);
#endif
        NEFORCE_THROW_EXCEPTION(memory_exception("shared ptr construction failed."));
    }
    _NEFORCE construct(reinterpret_cast<Counter*>(counter), object, mem, align, _NEFORCE move(deleter));
    inner::__setup_enable_shared_from(object, counter);
    return inner::__make_shared_fused(object, counter);
}

/**
 * @brief 创建动态数组的共享指针
 * @tparam T 数组类型
 * @param len 数组长度
 * @return 共享指针
 * @throw memory_exception 如果构造对象时抛出错误
 */
template <typename T>
enable_if_t<is_unbounded_array_v<T>, shared_ptr<T>>
make_shared(const size_t len) {
    using value = remove_extent_t<T>;
    value* tmp = nullptr;
    try {
        tmp = new value[len];
        return shared_ptr<T[]>(tmp, default_delete<value[]>{});
    } catch (...) {
        operator delete [](tmp);
        NEFORCE_THROW_EXCEPTION(memory_exception("shared ptr construction failed."));
    }
    unreachable();
}

/**
 * @brief 使用分配器创建共享指针
 * @tparam T 对象类型
 * @tparam Alloc 分配器类型
 * @tparam Args 参数类型
 * @param alloc 分配器
 * @param args 构造参数
 * @return 共享指针
 * @throw memory_exception 如果构造对象或控制块时抛出错误
 *
 * 使用自定义分配器分配控制块和对象的内存。
 */
template <typename T, typename Alloc, typename... Args>
enable_if_t<!is_array_v<T> && is_constructible_v<T, Args...>, shared_ptr<T>>
allocate_shared(Alloc& alloc, Args&&... args) {
    auto deleter = [](T* p) { p->~T(); };
    using ControlBlock = inner::__smart_ptr_counter_impl_allocated<T, decltype(deleter), Alloc>;

    const size_t align = _NEFORCE max(alignof(ControlBlock), alignof(T));
    const size_t offset = (sizeof(ControlBlock) + align - 1) & ~(align - 1);
    const size_t total_size = offset + sizeof(T);
    const size_t raw_size = total_size + align - 1;

    using alloc_traits = allocator_traits<remove_cv_t<Alloc>>;
    using byte_allocator = typename alloc_traits::template alloc_rebind_t<Alloc, byte_t>;
    byte_allocator byte_alloc(alloc);

    byte_t* raw_mem = allocator_traits<byte_allocator>::allocate(byte_alloc, raw_size);

    const uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw_mem);
    const uintptr_t aligned_addr = (raw_addr + align - 1) & ~static_cast<uintptr_t>(align - 1);
    auto aligned_mem = reinterpret_cast<byte_t*>(aligned_addr);
    T* object_ptr = reinterpret_cast<T*>(aligned_mem + offset);

    try {
        allocator_traits<Alloc>::construct(alloc, object_ptr, _NEFORCE forward<Args>(args)...);
    } catch (...) {
        allocator_traits<byte_allocator>::deallocate(byte_alloc, raw_mem, raw_size);
        NEFORCE_THROW_EXCEPTION(memory_exception("shared ptr ref object construction failed."));
    }

    ControlBlock* ctrl_block = nullptr;
    try {
        ctrl_block = _NEFORCE construct(
            reinterpret_cast<ControlBlock*>(aligned_mem),
            object_ptr, raw_mem, raw_size, deleter, alloc);
    } catch (...) {
        allocator_traits<Alloc>::destroy(alloc, object_ptr);
        allocator_traits<byte_allocator>::deallocate(byte_alloc, raw_mem, raw_size);
        NEFORCE_THROW_EXCEPTION(memory_exception("shared ptr control block construction failed."));
    }

    inner::__setup_enable_shared_from(object_ptr, ctrl_block);
    return inner::__make_shared_fused(object_ptr, ctrl_block);
}


/**
 * @brief 静态类型转换
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源共享指针
 * @return 转换后的共享指针
 */
template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& ptr) {
    return shared_ptr<T>(ptr, static_cast<T*>(ptr.get()));
}

/**
 * @brief CV类型转换
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源共享指针
 * @return 转换后的共享指针
 */
template <typename T, typename U>
shared_ptr<T> const_pointer_cast(const shared_ptr<U>& ptr) {
    return shared_ptr<T>(ptr, const_cast<T*>(ptr.get()));
}

/**
 * @brief 重解释类型转换
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源共享指针
 * @return 转换后的共享指针
 */
template <typename T, typename U>
shared_ptr<T> reinterpret_pointer_cast(const shared_ptr<U>& ptr) {
    return shared_ptr<T>(ptr, reinterpret_cast<T*>(ptr.get()));
}

/**
 * @brief 动态类型转换
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源共享指针
 * @return 转换后的共享指针
 */
template <typename T, typename U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& ptr) {
    T* tmp = dynamic_cast<T*>(ptr.get());
    if (tmp != nullptr) return shared_ptr<T>(ptr, tmp);
    return nullptr;
}

/** @} */ // SharedPointer

/**
 * @defgroup HashPrimary 哈希模板
 * @brief 哈希函数的模板和基础定义
 * @{
 */

/**
 * @brief shared_ptr的哈希特化
 * @tparam T 元素类型
 */
template <typename T>
struct hash<shared_ptr<T>> {
    NEFORCE_CONSTEXPR20 size_t operator ()(const shared_ptr<T>& ptr) const
    noexcept(noexcept(_NEFORCE declval<_NEFORCE hash<T*>>()(_NEFORCE declval<T*>()))) {
        return hash<T*>()(ptr.get());
    }
};

/** @} */ // HashPrimary

/// @cond
NEFORCE_BEGIN_INNER__

template <typename T>
class smart_pointer_atomic {
public:
      using value_type = T;

private:
    struct atomic_counter {
    public:
        using count_type = inner::__smart_ptr_counter;

    private:
        mutable atomic_base<uintptr_t> value_{0};

        static constexpr uintptr_t lock_bit{1};

        static void dereference(count_type* counter, true_type) noexcept {
            counter->decref_strong();
        }

        static void dereference(count_type* counter, false_type) noexcept {
            counter->decref_weak();
        }

    public:
        constexpr atomic_counter() noexcept = default;

        explicit atomic_counter(count_type* counter) noexcept
        : value_(reinterpret_cast<uintptr_t>(counter)) {
            counter = nullptr;
        }

        ~atomic_counter() {
            auto value = value_.load(memory_order_relaxed);
            NEFORCE_CONSTEXPR_ASSERT(!(value & lock_bit));
            if (auto counter = reinterpret_cast<count_type*>(value)) {
                this->dereference(counter, is_shared_ptr<T>());
            }
        }

        atomic_counter(const atomic_counter&) = delete;
        atomic_counter& operator =(const atomic_counter&) = delete;

        count_type* lock(memory_order mo) const noexcept {
            if (mo != memory_order_seq_cst) {
                mo = memory_order_acquire;
            }
            auto cur = value_.load(memory_order_relaxed);
            while (cur & lock_bit) {
                this_thread::relax();
                cur = value_.load(memory_order_relaxed);
            }

            while (!value_.compare_exchange_strong(cur, cur | lock_bit, mo, memory_order_relaxed)) {
                this_thread::relax();
                cur = cur & ~lock_bit;
            }
            return reinterpret_cast<count_type*>(cur);
        }

        void unlock(memory_order mo) const noexcept {
            value_.fetch_sub(1, mo);
        }

        count_type* swap_unlock(count_type* counter, memory_order mo) noexcept {
            if (mo != memory_order_seq_cst) {
                mo = memory_order_release;
            }
            auto addr = reinterpret_cast<uintptr_t>(counter);
            addr = value_.exchange(addr, mo);
            return reinterpret_cast<count_type*>(addr & ~lock_bit);
        }

        void wait_unlock(memory_order mo) const noexcept {
            const auto old_value = value_.load(memory_order_relaxed);
            const auto unlocked = old_value & ~lock_bit;
            value_.fetch_sub(1, memory_order_release);
            if (value_.load(memory_order_acquire) == unlocked) {
                value_.wait(unlocked, mo);
            }
        }

        void notify_one() noexcept {
            value_.notify_one();
        }

        void notify_all() noexcept {
            value_.notify_all();
        }
    };

    typename T::element_type* ptr_ = nullptr;
    atomic_counter refcount_;

    friend struct atomic<T>;

private:
    static typename atomic_counter::count_type* incref(
        typename atomic_counter::count_type* counter, true_type) {
        if (counter) {
            counter->incref_strong();
        }
        return counter;
    }

    static typename atomic_counter::count_type* incref(
        typename atomic_counter::count_type* counter, false_type) {
        if (counter) {
            counter->incref_weak();
        }
        return counter;
    }

public:
    constexpr smart_pointer_atomic() noexcept = default;

    explicit smart_pointer_atomic(value_type value) noexcept
    : ptr_(value.ptr_), refcount_(value.owner_) {
        value.owner_ = nullptr;
        value.ptr_ = nullptr;
    }

    ~smart_pointer_atomic() = default;

    smart_pointer_atomic(const smart_pointer_atomic&) = delete;
    void operator =(const smart_pointer_atomic&) = delete;

    value_type load(memory_order mo) const noexcept {
        NEFORCE_CONSTEXPR_ASSERT(mo != memory_order_release && mo != memory_order_acq_rel);
        if (mo != memory_order_seq_cst)
            mo = memory_order_acquire;

        value_type value;
        auto counter = refcount_.lock(mo);
        value.ptr_ = ptr_;
        value.owner_ = this->incref(counter, is_shared_ptr<T>());
        refcount_.unlock(memory_order_relaxed);
        return value;
    }

    void swap(value_type& value, memory_order mo) noexcept {
        refcount_.lock(memory_order_acquire);
        _NEFORCE swap(ptr_, value.ptr_);
        auto* old_owner = refcount_.swap_unlock(value.owner_, mo);
        value.owner_ = old_owner;
    }

    bool compare_exchange_strong(value_type& expected, value_type desired, memory_order mo1, memory_order mo2) noexcept {
        bool result = true;
        auto* counter = refcount_.lock(memory_order_acquire);
        if (ptr_ == expected.ptr_ && counter == expected.owner_) {
            ptr_ = desired.ptr_;
            auto* old_owner = refcount_.swap_unlock(desired.owner_, mo1);
            desired.owner_ = old_owner;
        } else {
            auto* new_counter = this->incref(counter, is_shared_ptr<T>());
            refcount_.unlock(mo2);
            expected.ptr_ = ptr_;
            expected.owner_ = new_counter;
            result = false;
        }
        return result;
    }

    void wait(value_type mold, memory_order mo) const noexcept {
        auto* counter = refcount_.lock(memory_order_acquire);
        if (ptr_ == mold.ptr_ && counter == mold.owner_) {
            refcount_.wait_unlock(mo);
        } else {
            refcount_.unlock(memory_order_relaxed);
        }
    }

    void notify_one() noexcept {
        refcount_.notify_one();
    }

    void notify_all() noexcept {
        refcount_.notify_all();
    }
};

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup AtomicOperations 原子操作
 * @brief 原子变量的操作
 * @{
 */

/**
 * @brief shared_ptr的原子特化
 * @tparam T 对象类型
 *
 * 提供shared_ptr的原子操作支持，实现无锁的原子操作。
 */
template <typename T>
struct atomic<shared_ptr<T>> {
public:
    using value_type = shared_ptr<T>;

    static constexpr bool is_always_lock_free = false;

private:
    inner::smart_pointer_atomic<value_type> atomic_;

public:
    /**
     * @brief 检查是否无锁
     * @return 始终返回false
     */
    bool is_lock_free() const noexcept {
        return false;
    }

    /**
     * @brief 默认构造函数
     */
    constexpr atomic(nullptr_t = nullptr) noexcept {}

    /**
     * @brief 从shared_ptr构造
     * @param value 初始值
     */
    atomic(value_type value) noexcept
    : atomic_(move(value)) {}

    atomic(const atomic&) = delete;
    void operator =(const atomic&) = delete;

    /**
     * @brief 原子加载
     * @param mo 内存序
     * @return 加载的值
     */
    value_type load(memory_order mo = memory_order_seq_cst) const noexcept {
        return atomic_.load(mo);
    }

    /**
     * @brief 隐式转换操作符
     */
    operator value_type() const noexcept {
        return atomic_.load(memory_order_seq_cst);
    }

    /**
     * @brief 原子存储
     * @param desired 要存储的值
     * @param mo 内存序
     */
    void store(value_type desired, memory_order mo = memory_order_seq_cst) noexcept {
        atomic_.swap(desired, mo);
    }

    /**
     * @brief 赋值操作符
     */
    void operator =(value_type desired) noexcept {
        atomic_.swap(desired, memory_order_seq_cst);
    }

    /**
     * @brief 赋空值操作符
     */
    void operator =(nullptr_t) noexcept {
        store(nullptr);
    }

    /**
     * @brief 交换操作
     * @param desired 要交换的值
     * @param mo 内存序
     * @return 交换前的值
     */
    value_type exchange(value_type desired, memory_order mo = memory_order_seq_cst) noexcept {
        atomic_.swap(desired, mo);
        return desired;
    }

    /**
     * @brief 比较交换强版本
     */
    bool compare_exchange_strong(value_type& expected, value_type desired,
                                 memory_order mo, memory_order mo2) noexcept {
        return atomic_.compare_exchange_strong(expected, desired, mo, mo2);
    }

    /**
     * @brief 简化比较交换强版本
     */
    bool compare_exchange_strong(value_type& expected, value_type desired,
                                 memory_order mo = memory_order_seq_cst) noexcept {
        return compare_exchange_strong(expected, move(desired), mo, cmpexch_failure_order(mo));
    }

    /**
     * @brief 比较交换弱版本
     */
    bool compare_exchange_weak(value_type& expected, value_type desired,
                               memory_order mo, memory_order mo2) noexcept {
        return compare_exchange_strong(expected, move(desired), mo, mo2);
    }

    /**
     * @brief 简化比较交换弱版本
     */
    bool compare_exchange_weak(value_type& expected, value_type desired,
                               memory_order mo = memory_order_seq_cst) noexcept {
        return compare_exchange_strong(expected, move(desired), mo);
    }

    /**
     * @brief 等待值改变
     */
    void wait(value_type mold, memory_order mo = memory_order_seq_cst) const noexcept {
        atomic_.wait(move(mold), mo);
    }

    /**
     * @brief 通知一个等待者
     */
    void notify_one() noexcept {
        atomic_.notify_one();
    }

    /**
     * @brief 通知所有等待者
     */
    void notify_all() noexcept {
        atomic_.notify_all();
    }
};

/** @} */ // AtomicOperations

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_SHARED_PTR_HPP__
