#ifndef MSTL_CORE_MEMORY_WEAK_PTR_HPP__
#define MSTL_CORE_MEMORY_WEAK_PTR_HPP__

/**
 * @file weak_ptr.hpp
 * @brief MSTL弱智能指针实现
 *
 * 此文件提供了弱智能指针的完整实现，用于观察共享智能指针管理的对象，
 * 但不增加引用计数，避免循环引用问题。
 */

#include "MSTL/core/memory/shared_ptr.hpp"
MSTL_BEGIN_NAMESPACE__

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
    _INNER __smart_ptr_counter* owner_ = nullptr;  ///< 控制块指针

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend class shared_ptr;

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
    : ptr_(sp.get()), owner_(reinterpret_cast<_INNER __smart_ptr_counter*>(sp.owner_)) {
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
        if (_MSTL addressof(wp) == this) return *this;
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
        owner_ = reinterpret_cast<_INNER __smart_ptr_counter*>(sp.owner_);
        if (owner_) owner_->incref_weak();
        return *this;
    }

    /**
     * @brief 移动赋值运算符
     * @param wp 要移动的弱指针
     * @return 当前弱指针的引用
     */
    weak_ptr& operator =(weak_ptr&& wp) noexcept {
        if (_MSTL addressof(wp) == this) return *this;
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
        if (_MSTL addressof(wp) == this) return;
        _MSTL swap(ptr_, wp.ptr_);
        _MSTL swap(owner_, wp.owner_);
    }

    /**
     * @brief 获取观察对象的引用计数
     * @return 强引用计数值
     */
    MSTL_NODISCARD long use_count() const noexcept {
        return owner_ ? static_cast<long>(owner_->use_count()) : 0;
    }

    /**
     * @brief 检查观察的对象是否已被销毁
     * @return 对象是否已被销毁
     */
    MSTL_NODISCARD bool expired() const noexcept {
        return use_count() == 0;
    }

    /**
     * @brief 尝试获取共享智能指针
     * @return 如果对象仍然存在，返回共享智能指针；否则返回空共享智能指针
     *
     * 如果观察的对象仍然存在，则创建一个新的共享智能指针。
     */
    MSTL_NODISCARD shared_ptr<T> lock() const noexcept {
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
    MSTL_NODISCARD bool owner_equal(const weak_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }

    /**
     * @brief 与共享指针检查所有权是否相等
     * @tparam U 比较的共享指针类型
     * @param rhs 要比较的共享指针
     * @return 是否共享同一控制块
     */
    template <typename U>
    MSTL_NODISCARD bool owner_equal(const shared_ptr<U>& rhs) const noexcept {
        return owner_ == reinterpret_cast<_INNER __smart_ptr_counter*>(rhs.owner_);
    }

    /**
     * @brief 比较所有权顺序
     * @tparam U 比较的弱指针类型
     * @param rhs 要比较的弱指针
     * @return 当前控制块地址是否小于rhs的控制块地址
     */
    template <typename U>
    MSTL_NODISCARD bool owner_before(const weak_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }

    /**
     * @brief 与共享指针比较所有权顺序
     * @tparam U 比较的共享指针类型
     * @param rhs 要比较的共享指针
     * @return 当前控制块地址是否小于rhs的控制块地址
     */
    template <typename U>
    MSTL_NODISCARD bool owner_before(const shared_ptr<U>& rhs) const noexcept {
        return owner_ < reinterpret_cast<_INNER __smart_ptr_counter*>(rhs.owner_);
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
    MSTL_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    MSTL_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    MSTL_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
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
    MSTL_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    MSTL_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    MSTL_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
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
    MSTL_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较共享指针和弱指针的所有权
     */
    template <typename T, typename U>
    MSTL_NODISCARD bool operator ()(const shared_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较弱指针和共享指针的所有权
     */
    template <typename T, typename U>
    MSTL_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    /**
     * @brief 比较两个弱指针的所有权
     */
    template <typename T, typename U>
    MSTL_NODISCARD bool operator ()(const weak_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

/** @} */ // WeakPointer

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_WEAK_PTR_HPP__
