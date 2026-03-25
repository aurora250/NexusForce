#ifndef NEFORCE_CORE_MEMORY_WEAK_PTR_HPP__
#define NEFORCE_CORE_MEMORY_WEAK_PTR_HPP__

/**
 * @file weak_ptr.hpp
 * @brief 弱智能指针实现
 *
 * 此文件提供了弱智能指针的完整实现，用于观察共享智能指针管理的对象，
 * 但不增加引用计数，避免循环引用问题。
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
 */
template <typename T>
class weak_ptr {
public:
    using element_type = T;  ///< 元素类型

private:
    element_type* ptr_ = nullptr;   ///< 观察的对象指针
    inner::__smart_ptr_counter* owner_ = nullptr;  ///< 控制块指针

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend class shared_ptr;

    template <typename U>
    friend class inner::smart_pointer_atomic;

public:
    /**
     * @brief 默认构造函数
     * @param np 空指针字面量
     *
     * 创建空的弱指针，不观察任何对象。
     */
    weak_ptr(nullptr_t np = nullptr) noexcept {}

    /**
     * @brief 共享智能指针构造函数
     * @tparam U 可转换为T*的类型
     * @param sp 共享指针
     *
     * 创建观察sp管理的对象的弱指针。
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const shared_ptr<U>& sp) noexcept
    : ptr_(sp.get()), owner_(reinterpret_cast<inner::__smart_ptr_counter*>(sp.owner_)) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    /**
     * @brief 拷贝构造函数
     * @param wp 要拷贝的弱指针
     */
    weak_ptr(const weak_ptr& wp) noexcept
    : ptr_(wp.ptr_), owner_(wp.owner_) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    /**
     * @brief 类型转换拷贝构造函数
     * @tparam U 可转换为T*的类型
     * @param wp 要拷贝的弱指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const weak_ptr<U>& wp) noexcept
    : ptr_(wp.ptr_), owner_(wp.owner_) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    /**
     * @brief 移动构造函数
     * @param wp 要移动的弱指针
     */
    weak_ptr(weak_ptr&& wp) noexcept
    : ptr_(wp.ptr_), owner_(wp.owner_) {
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
    }

    /**
     * @brief 类型转换移动构造函数
     * @tparam U 可转换为T*的类型
     * @param wp 要移动的弱指针
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(weak_ptr<U>&& wp) noexcept
    : ptr_(wp.ptr_), owner_(wp.owner_) {
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
    }

    /**
     * @brief 析构函数
     * @note 减少弱引用计数，当弱引用计数为0时删除控制块
     */
    ~weak_ptr() {
        reset();
    }

    /**
     * @brief 拷贝赋值运算符
     * @param wp 要拷贝的弱指针
     * @return 当前弱指针的引用
     */
    weak_ptr& operator =(const weak_ptr& wp) noexcept {
        if (_NEFORCE addressof(wp) == this) return *this;
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        if (owner_) owner_->incref_weak();
        return *this;
    }

    /**
     * @brief 类型转换拷贝赋值运算符
     * @tparam U 可转换为T*的类型
     * @param wp 要拷贝的弱指针
     * @return 当前弱指针的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator =(const weak_ptr<U>& wp) noexcept {
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        if (owner_) owner_->incref_weak();
        return *this;
    }

    /**
     * @brief 共享智能指针赋值运算符
     * @tparam U 可转换为T*的类型
     * @param sp 共享指针
     * @return 当前弱指针的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator =(const shared_ptr<U>& sp) noexcept {
        if (owner_) owner_->decref_weak();
        ptr_ = sp.get();
        owner_ = reinterpret_cast<inner::__smart_ptr_counter*>(sp.owner_);
        if (owner_) owner_->incref_weak();
        return *this;
    }

    /**
     * @brief 移动赋值运算符
     * @param wp 要移动的弱指针
     * @return 当前弱指针的引用
     */
    weak_ptr& operator =(weak_ptr&& wp) noexcept {
        if (_NEFORCE addressof(wp) == this) return *this;
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
        return *this;
    }

    /**
     * @brief 类型转换移动赋值运算符
     * @tparam U 可转换为T*的类型
     * @param wp 要移动的弱指针
     * @return 当前弱指针的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator =(weak_ptr<U>&& wp) noexcept {
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
        return *this;
    }

    /**
     * @brief 重置弱指针
     *
     * 使弱指针不再观察任何对象，减少弱引用计数。
     */
    void reset() noexcept {
        if (owner_) {
            owner_->decref_weak();
            owner_ = nullptr;
        }
        ptr_ = nullptr;
    }

    /**
     * @brief 交换两个弱指针
     * @param wp 要交换的弱指针
     */
    void swap(weak_ptr& wp) noexcept {
        if (_NEFORCE addressof(wp) == this) return;
        _NEFORCE swap(ptr_, wp.ptr_);
        _NEFORCE swap(owner_, wp.owner_);
    }

    /**
     * @brief 获取观察对象的引用计数
     * @return 强引用计数值
     */
    NEFORCE_NODISCARD long use_count() const noexcept {
        return owner_ ? static_cast<long>(owner_->use_count()) : 0;
    }

    /**
     * @brief 检查观察的对象是否已被销毁
     * @return 对象是否已被销毁
     */
    NEFORCE_NODISCARD bool expired() const noexcept {
        return use_count() == 0;
    }

    /**
     * @brief 尝试获取共享智能指针
     * @return 如果对象仍然存在，返回共享智能指针；否则返回空共享智能指针
     *
     * 如果观察的对象仍然存在，则创建一个新的共享智能指针。
     */
    NEFORCE_NODISCARD shared_ptr<T> lock() const noexcept {
        if (owner_ && owner_->try_incref_strong()) {
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
    using is_transparent = void;  ///< 支持透明比较

    /**
     * @brief 比较两个共享指针的所有权
     */
    NEFORCE_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    NEFORCE_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    NEFORCE_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

/**
 * @brief 弱指针的所有权比较器特化
 * @tparam T 弱指针的类型
 */
template <typename T>
struct owner_less<weak_ptr<T>> {
    using is_transparent = void;  ///< 支持透明比较

    /**
     * @brief 比较两个弱指针的所有权
     */
    NEFORCE_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    NEFORCE_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    NEFORCE_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
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
    using is_transparent = void;  ///< 支持透明比较

    /**
     * @brief 比较两个共享指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较两个弱指针的所有权
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};


template <typename T>
struct atomic<weak_ptr<T>> {
public:
    using value_type = weak_ptr<T>;

    static constexpr bool is_always_lock_free = false;

private:
    inner::smart_pointer_atomic<value_type> atomic_;

public:
    bool is_lock_free() const noexcept {
        return false;
    }

    constexpr atomic() noexcept = default;

    atomic(value_type value) noexcept
    : atomic_(move(value)) {}

    atomic(const atomic&) = delete;
    void operator =(const atomic&) = delete;

    value_type load(memory_order mo = memory_order_seq_cst) const noexcept {
        return atomic_.load(mo);
    }

    operator value_type() const noexcept {
        return atomic_.load(memory_order_seq_cst);
    }

    void store(value_type desired, memory_order mo = memory_order_seq_cst) noexcept {
        atomic_.swap(desired, mo);
    }

    void operator =(value_type desired) noexcept {
        atomic_.swap(desired, memory_order_seq_cst);
    }

    value_type exchange(value_type desired, memory_order mo = memory_order_seq_cst) noexcept {
        atomic_.swap(desired, mo);
        return desired;
    }

    bool compare_exchange_strong(value_type& expected, value_type desired,
                                 memory_order mo, memory_order mo2) noexcept {
        return atomic_.compare_exchange_strong(expected, desired, mo, mo2);
    }

    bool compare_exchange_strong(value_type& expected, value_type desired,
                                 memory_order mo = memory_order_seq_cst) noexcept {
        memory_order mo2;
        switch (mo) {
            case memory_order_acq_rel: {
                mo2 = memory_order_acquire;
                break;
            }
            case memory_order_release: {
                mo2 = memory_order_relaxed;
                break;
            }
            default: {
                mo2 = mo;
            }
        }
        return compare_exchange_strong(expected, move(desired), mo, mo2);
    }

    bool compare_exchange_weak(value_type& expected, value_type desired,
                               memory_order mo, memory_order mo2) noexcept {
        return compare_exchange_strong(expected, move(desired), mo, mo2);
    }

    bool compare_exchange_weak(value_type& expected, value_type desired,
                               memory_order mo = memory_order_seq_cst) noexcept {
        return compare_exchange_strong(expected, move(desired), mo);
    }

    void wait(value_type mold, memory_order mo = memory_order_seq_cst) const noexcept {
        atomic_.wait(move(mold), mo);
    }

    void notify_one() noexcept {
        atomic_.notify_one();
    }

    void notify_all() noexcept {
        atomic_.notify_all();
    }
};


/** @} */ // WeakPointer

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_WEAK_PTR_HPP__
