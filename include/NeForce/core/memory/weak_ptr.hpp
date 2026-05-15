#ifndef NEFORCE_CORE_MEMORY_WEAK_PTR_HPP__
#define NEFORCE_CORE_MEMORY_WEAK_PTR_HPP__

/**
 * @file weak_ptr.hpp
 * @brief 弱智能指针实现
 *
 * 此文件提供了弱智能指针的完整实现，用于观察共享智能指针管理的对象，
 * 但不增加引用计数，避免循环引用问题。
 *
 * 主要用途：
 * - 打破循环引用（如双向链表、观察者模式）
 * - 缓存对象但不影响其生命周期
 * - 安全地检查对象是否存在
 */

#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup WeakPointer 弱智能指针
 * @brief 弱智能指针相关类和辅助工具
 * @{
 */

/**
 * @class weak_ptr
 * @brief 弱智能指针类模板
 * @tparam T 对象类型
 *
 * 弱智能指针用于观察由共享智能指针管理的对象，但不拥有对象的所有权。
 * 不会增加对象的引用计数，因此不会阻止对象被销毁。
 *
 * 特性：
 * - 不增加对象引用计数
 * - 可检测对象是否已被销毁
 * - 可安全地升级为shared_ptr
 * - 支持所有权比较和排序
 * - 线程安全的过期检测
 *
 * @note 弱指针本身是线程安全的，但升级后的共享指针需要单独管理
 */
template <typename T>
class weak_ptr {
public:
    using element_type = T; ///< 元素类型

private:
    element_type* ptr_ = nullptr;                 ///< 观察的对象指针
    inner::__smart_ptr_counter* owner_ = nullptr; ///< 控制块指针

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend class shared_ptr;

    template <typename U>
    friend class inner::smart_pointer_atomic;

public:
    /**
     * @brief 默认构造函数
     */
    weak_ptr(nullptr_t = nullptr) noexcept {}

    /**
     * @brief 共享智能指针构造函数
     * @tparam U 可转换为T*的类型
     * @param shared 共享指针
     *
     * 创建观察sp管理的对象的弱指针。
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const shared_ptr<U>& shared) noexcept :
    ptr_(shared.get()),
    owner_(reinterpret_cast<inner::__smart_ptr_counter*>(shared.owner_)) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    /**
     * @brief 拷贝构造函数
     * @param other 要拷贝的弱指针
     */
    weak_ptr(const weak_ptr& other) noexcept :
    ptr_(other.ptr_),
    owner_(other.owner_) {
        if (owner_ != nullptr) {
            owner_->incref_weak();
        }
    }

    /**
     * @brief 类型转换拷贝构造函数
     * @tparam U 可转换为T*的类型
     * @param other 要拷贝的弱指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const weak_ptr<U>& other) noexcept :
    ptr_(other.ptr_),
    owner_(other.owner_) {
        if (owner_ != nullptr) {
            owner_->incref_weak();
        }
    }

    /**
     * @brief 移动构造函数
     * @param other 要移动的弱指针
     */
    weak_ptr(weak_ptr&& other) noexcept :
    ptr_(other.ptr_),
    owner_(other.owner_) {
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
    }

    /**
     * @brief 类型转换移动构造函数
     * @tparam U 可转换为T*的类型
     * @param other 要移动的弱指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(weak_ptr<U>&& other) noexcept :
    ptr_(other.ptr_),
    owner_(other.owner_) {
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
    }

    /**
     * @brief 析构函数
     * @note 减少弱引用计数，当弱引用计数为0时删除控制块
     */
    ~weak_ptr() { reset(); }

    /**
     * @brief 拷贝赋值运算符
     * @param other 要拷贝的弱指针
     * @return 当前弱指针的引用
     */
    weak_ptr& operator=(const weak_ptr& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        if (owner_ != nullptr) {
            owner_->decref_weak();
        }
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        if (owner_ != nullptr) {
            owner_->incref_weak();
        }
        return *this;
    }

    /**
     * @brief 类型转换拷贝赋值运算符
     * @tparam U 可转换为T*的类型
     * @param other 要拷贝的弱指针
     * @return 当前弱指针的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator=(const weak_ptr<U>& other) noexcept {
        if (owner_) {
            owner_->decref_weak();
        }
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        if (owner_) {
            owner_->incref_weak();
        }
        return *this;
    }

    /**
     * @brief 共享智能指针赋值运算符
     * @tparam U 可转换为T*的类型
     * @param shared 共享指针
     * @return 当前弱指针的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator=(const shared_ptr<U>& shared) noexcept {
        if (owner_) {
            owner_->decref_weak();
        }
        ptr_ = shared.get();
        owner_ = reinterpret_cast<inner::__smart_ptr_counter*>(shared.owner_);
        if (owner_) {
            owner_->incref_weak();
        }
        return *this;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的弱指针
     * @return 当前弱指针的引用
     */
    weak_ptr& operator=(weak_ptr&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        if (owner_ != nullptr) {
            owner_->decref_weak();
        }
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
        return *this;
    }

    /**
     * @brief 类型转换移动赋值运算符
     * @tparam U 可转换为T*的类型
     * @param other 要移动的弱指针
     * @return 当前弱指针的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator=(weak_ptr<U>&& other) noexcept {
        if (owner_) {
            owner_->decref_weak();
        }
        ptr_ = other.ptr_;
        owner_ = other.owner_;
        other.ptr_ = nullptr;
        other.owner_ = nullptr;
        return *this;
    }

    /**
     * @brief 重置弱指针
     *
     * 使弱指针不再观察任何对象，减少弱引用计数。
     */
    void reset() noexcept {
        if (owner_ != nullptr) {
            owner_->decref_weak();
            owner_ = nullptr;
        }
        ptr_ = nullptr;
    }

    /**
     * @brief 交换两个弱指针
     * @param other 要交换的弱指针
     */
    void swap(weak_ptr& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return;
        }
        _NEFORCE swap(ptr_, other.ptr_);
        _NEFORCE swap(owner_, other.owner_);
    }

    /**
     * @brief 获取观察对象的引用计数
     * @return 强引用计数值
     */
    NEFORCE_NODISCARD long use_count() const noexcept {
        return owner_ != nullptr ? static_cast<long>(owner_->use_count()) : 0;
    }

    /**
     * @brief 检查观察的对象是否已被销毁
     * @return 对象是否已被销毁
     */
    NEFORCE_NODISCARD bool expired() const noexcept { return use_count() == 0; }

    /**
     * @brief 尝试获取共享智能指针
     * @return 如果对象仍然存在，返回共享智能指针；否则返回空共享智能指针
     *
     * 如果观察的对象仍然存在，则创建一个新的共享智能指针。
     */
    NEFORCE_NODISCARD shared_ptr<T> lock() const noexcept {
        if (owner_ != nullptr && owner_->try_incref_strong()) {
            return shared_ptr<T>(ptr_, owner_);
        }
        return shared_ptr<T>();
    }

    /**
     * @brief 检查所有权是否相等
     * @tparam U 比较的弱指针类型
     * @param rhs 要比较的弱指针
     * @return 是否共享同一控制块
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_equal(const weak_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }

    /**
     * @brief 与共享指针检查所有权是否相等
     * @tparam U 比较的共享指针类型
     * @param rhs 要比较的共享指针
     * @return 是否共享同一控制块
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_equal(const shared_ptr<U>& rhs) const noexcept {
        return owner_ == reinterpret_cast<inner::__smart_ptr_counter*>(rhs.owner_);
    }

    /**
     * @brief 比较所有权顺序
     * @tparam U 比较的弱指针类型
     * @param rhs 要比较的弱指针
     * @return 当前控制块地址是否小于rhs的控制块地址
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_before(const weak_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }

    /**
     * @brief 与共享指针比较所有权顺序
     * @tparam U 比较的共享指针类型
     * @param rhs 要比较的共享指针
     * @return 当前控制块地址是否小于rhs的控制块地址
     */
    template <typename U>
    NEFORCE_NODISCARD bool owner_before(const shared_ptr<U>& rhs) const noexcept {
        return owner_ < reinterpret_cast<inner::__smart_ptr_counter*>(rhs.owner_);
    }
};


/**
 * @struct owner_less
 * @brief 智能指针的所有权比较器
 * @tparam T 智能指针的类型
 */
template <typename T>
struct owner_less;

/**
 * @brief 共享指针的所有权比较器特化
 * @tparam T 共享指针的类型
 *
 * 提供基于控制块地址的智能指针比较，用于关联容器中的排序。
 */
template <typename T>
struct owner_less<shared_ptr<T>> {
    using is_transparent = void; ///< 支持透明比较

    /**
     * @brief 比较两个共享指针的所有权
     */
    NEFORCE_NODISCARD bool operator()(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    NEFORCE_NODISCARD bool operator()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    NEFORCE_NODISCARD bool operator()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

/**
 * @brief 弱指针的所有权比较器特化
 * @tparam T 弱指针的类型
 */
template <typename T>
struct owner_less<weak_ptr<T>> {
    using is_transparent = void; ///< 支持透明比较

    /**
     * @brief 比较两个弱指针的所有权
     */
    NEFORCE_NODISCARD bool operator()(const weak_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    NEFORCE_NODISCARD bool operator()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    NEFORCE_NODISCARD bool operator()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

/**
 * @brief 通用所有权比较器特化
 *
 * 支持不同类型智能指针之间的透明比较。
 */
template <>
struct owner_less<void> {
    using is_transparent = void; ///< 支持透明比较

    /**
     * @brief 比较两个共享指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator()(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator()(const shared_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator()(const weak_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较两个弱指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator()(const weak_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

/** @} */ // WeakPointer

/**
 * @defgroup AtomicOperations 原子操作
 * @brief 原子变量的操作
 * @{
 */

/**
 * @brief weak_ptr的原子特化
 * @tparam T 对象类型
 *
 * 提供weak_ptr的原子操作支持，实现无锁的原子操作。
 */
template <typename T>
struct atomic<weak_ptr<T>> {
public:
    using value_type = weak_ptr<T>;

    static constexpr bool is_always_lock_free = false;

private:
    inner::smart_pointer_atomic<value_type> atomic_;

public:
    /**
     * @brief 检查是否无锁
     * @return 始终返回false
     */
    NEFORCE_NODISCARD bool is_lock_free() const noexcept { return false; }

    constexpr atomic() noexcept = default;

    /**
     * @brief 从weak_ptr构造
     * @param value 初始值
     */
    atomic(value_type value) noexcept :
    atomic_(move(value)) {}

    atomic(const atomic&) = delete;
    void operator=(const atomic&) = delete;

    /**
     * @brief 原子加载
     * @param mo 内存序
     * @return 加载的值
     */
    value_type load(memory_order mo = memory_order_seq_cst) const noexcept { return atomic_.load(mo); }

    /**
     * @brief 隐式转换操作符
     */
    operator value_type() const noexcept { return atomic_.load(memory_order_seq_cst); }

    /**
     * @brief 原子存储
     * @param desired 要存储的值
     * @param mo 内存序
     */
    void store(value_type desired, memory_order mo = memory_order_seq_cst) noexcept { atomic_.swap(desired, mo); }

    /**
     * @brief 赋值操作符
     */
    void operator=(value_type desired) noexcept { atomic_.swap(desired, memory_order_seq_cst); }

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
    bool compare_exchange_strong(value_type& expected, value_type desired, memory_order mo, memory_order mo2) noexcept {
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
    bool compare_exchange_weak(value_type& expected, value_type desired, memory_order mo, memory_order mo2) noexcept {
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
    void wait(value_type mold, memory_order mo = memory_order_seq_cst) const noexcept { atomic_.wait(move(mold), mo); }

    /**
     * @brief 通知一个等待者
     */
    void notify_one() noexcept { atomic_.notify_one(); }

    /**
     * @brief 通知所有等待者
     */
    void notify_all() noexcept { atomic_.notify_all(); }
};

/** @} */ // AtomicOperations

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_WEAK_PTR_HPP__
