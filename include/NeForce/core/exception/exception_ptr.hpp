#ifndef NEFORCE_CORE_EXCEPTION_EXCEPTION_PTR_HPP__
#define NEFORCE_CORE_EXCEPTION_EXCEPTION_PTR_HPP__

/**
 * @file exception_ptr.hpp
 * @brief 异常指针实现
 *
 * 此文件提供了异常指针的实现，用于跨上下文传递异常。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include <typeinfo>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ExceptionHandling 异常处理
 * @brief 异常处理类与工具
 * @{
 */

/**
 * @class exception_wrapper
 * @brief 异常包装器基类
 *
 * 用于实现异常的类型擦除，允许以统一的方式处理不同类型的异常。
 * 提供异常重新抛出、类型查询和克隆功能。
 */
class exception_wrapper {
public:
    virtual ~exception_wrapper() = default;

    /**
     * @brief 重新抛出异常
     *
     * 重新抛出被包装的异常。
     */
    virtual void rethrow() const = 0;

    /**
     * @brief 获取异常类型信息
     * @return 异常的类型信息
     */
    virtual const std::type_info& type() const noexcept = 0;

    /**
     * @brief 克隆异常包装器
     * @return 异常包装器的唯一指针
     *
     * 创建当前异常包装器的深拷贝。
     */
    virtual unique_ptr<exception_wrapper> clone() const = 0;
};

/**
 * @class typed_exception_wrapper
 * @brief 类型化异常包装器模板类
 * @tparam Ex 异常类型
 *
 * 对特定异常类型的包装器实现，存储异常实例并提供类型特定的操作。
 */
template <typename Ex>
class typed_exception_wrapper final : public exception_wrapper {
    Ex exception_;  ///< 存储的异常实例

public:
    /**
     * @brief 拷贝构造函数
     * @param ex 要包装的异常
     */
    typed_exception_wrapper(const Ex& ex)
    : exception_(ex) {}

    /**
     * @brief 移动构造函数
     * @param ex 要包装的异常
     */
    typed_exception_wrapper(Ex&& ex) noexcept
    : exception_(_NEFORCE move(ex)) {}

    /**
     * @brief 重新抛出异常
     * @note 重新抛出存储的异常
     */
    NEFORCE_ALWAYS_INLINE void rethrow() const override {
        throw exception_;
    }

    /**
     * @brief 获取异常类型信息
     * @return 异常的类型信息
     */
    NEFORCE_ALWAYS_INLINE const std::type_info& type() const noexcept override {
        return typeid(Ex);
    }

    /**
     * @brief 克隆异常包装器
     * @return 异常包装器的唯一指针
     */
    NEFORCE_ALWAYS_INLINE unique_ptr<exception_wrapper> clone() const override {
        return _NEFORCE make_unique<typed_exception_wrapper>(exception_);
    }
};


/**
 * @class exception_ptr
 * @brief 异常指针类
 *
 * 用于跨函数边界共享异常对象，支持引用计数和异常重新抛出。
 */
class exception_ptr {
public:
    /**
     * @struct ecb
     * @brief 异常控制块
     *
     * 管理异常包装器的生命周期和引用计数。
     */
    struct ecb {
        unique_ptr<exception_wrapper> wrapper;  ///< 异常包装器
        atomic_int ref_count{1};  ///< 引用计数

        /**
         * @brief 构造函数
         * @param wrapper 异常包装器的唯一指针
         */
        explicit ecb(unique_ptr<exception_wrapper> wrapper)
        : wrapper(_NEFORCE move(wrapper)) {}

        /**
         * @brief 增加引用计数
         */
        NEFORCE_ALWAYS_INLINE void add_ref() noexcept {
            ref_count.fetch_add(1, memory_order_relaxed);
        }

        /**
         * @brief 减少引用计数
         * @note 当引用计数归零时，删除控制块
         */
        NEFORCE_ALWAYS_INLINE void release() noexcept {
            if (ref_count.load(memory_order_acquire) == 1 ||
                ref_count.fetch_sub(1, memory_order_acq_rel) == 1) {
                    delete this;
            }
        }
    };

private:
    ecb* ecb_{nullptr};  ///< 控制块指针

    /**
     * @brief 私有构造函数
     * @param cb 控制块指针
     * @note 用于工厂函数创建异常指针
     */
    explicit exception_ptr(ecb* cb) noexcept : ecb_(cb) {}

    template <typename Ex>
    friend exception_ptr make_exception_ptr(Ex) noexcept;

    friend exception_ptr NEFORCE_API current_exception() noexcept;

    friend void NEFORCE_API rethrow_exception(const exception_ptr &);

    template <typename Ex>
    friend exception_ptr make_exception_ptr(Ex ex) noexcept;

public:
    /**
     * @brief 默认构造函数
     * @param np 空指针字面量
     *
     * 创建一个空的异常指针，不引用任何异常。
     */
    exception_ptr(nullptr_t np = nullptr) noexcept {}

    /**
     * @brief 拷贝构造函数
     * @param other 要拷贝的异常指针
     *
     * 增加引用计数，共享异常对象。
     */
    exception_ptr(const exception_ptr& other) noexcept
    : ecb_(other.ecb_) {
        if (ecb_) ecb_->add_ref();
    }

    /**
     * @brief 移动构造函数
     * @param other 要移动的异常指针
     *
     * 转移异常指针的所有权，不增加引用计数。
     */
    exception_ptr(exception_ptr&& other) noexcept
    : ecb_(other.ecb_) {
        other.ecb_ = nullptr;
    }

    /**
     * @brief 析构函数
     *
     * 减少引用计数，当引用计数为0时释放资源。
     */
    ~exception_ptr() noexcept {
        if (ecb_) {
            ecb_->release();
        }
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 要拷贝的异常指针
     * @return 当前异常指针的引用
     */
    exception_ptr& operator =(const exception_ptr& other) noexcept {
        if (this != &other) {
            exception_ptr temp(other);
            swap(temp);
        }
        return *this;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的异常指针
     * @return 当前异常指针的引用
     */
    exception_ptr& operator =(exception_ptr&& other) noexcept {
        if (this != &other) {
            exception_ptr temp(_NEFORCE move(other));
            _NEFORCE swap(ecb_, other.ecb_);
        }
        return *this;
    }

    /**
     * @brief 交换两个异常指针
     * @param other 要交换的异常指针
     */
    void swap(exception_ptr& other) noexcept {
        _NEFORCE swap(ecb_, other.ecb_);
    }

    /**
     * @brief 布尔转换运算符
     * @return 是否持有异常
     *
     * 检查异常指针是否非空。
     */
    explicit operator bool() const noexcept {
        return ecb_ != nullptr;
    }

    /**
     * @brief 相等比较运算符
     * @param rhs 右操作数
     * @return 是否相等
     *
     * 比较两个异常指针是否引用同一个异常对象。
     */
    bool operator ==(const exception_ptr& rhs) const noexcept {
        return ecb_ == rhs.ecb_;
    }

    /**
     * @brief 不等比较运算符
     * @param rhs 右操作数
     * @return 是否不相等
     */
    bool operator !=(const exception_ptr& rhs) const noexcept {
        return !(*this == rhs);
    }

    /**
     * @brief 与空指针比较相等
     * @return 是否为空
     */
    bool operator ==(nullptr_t) const noexcept {
        return !static_cast<bool>(*this);
    }

    /**
     * @brief 空指针与异常指针比较相等
     */
    friend bool operator ==(nullptr_t, const exception_ptr& ptr) noexcept {
        return !ptr;
    }

    /**
     * @brief 与空指针比较不等
     * @return 是否非空
     */
    bool operator !=(nullptr_t) const noexcept {
        return static_cast<bool>(*this);
    }

    /**
     * @brief 空指针与异常指针比较不等
     */
    friend bool operator!=(nullptr_t, const exception_ptr& ptr) noexcept {
        return static_cast<bool>(ptr);
    }

    /**
     * @brief 获取异常类型信息
     * @return 异常的类型信息
     *
     * 如果异常指针为空，返回typeid(void)。
     */
    NEFORCE_NODISCARD const std::type_info& exception_type() const noexcept {
        if (!ecb_ || !ecb_->wrapper) {
            return typeid(void);
        }
        return ecb_->wrapper->type();
    }
};


/**
 * @brief 创建异常指针
 * @tparam Ex 异常类型
 * @param ex 异常对象
 * @return 异常指针
 *
 * 创建引用指定异常的异常指针。
 * 如果内存分配失败，返回空的异常指针。
 */
template <typename Ex>
exception_ptr make_exception_ptr(Ex ex) noexcept {
    try {
        auto wrapper = _NEFORCE make_unique<typed_exception_wrapper<decay_t<Ex>>>(_NEFORCE forward<Ex>(ex));
        unique_ptr<exception_ptr::ecb> control_block(new exception_ptr::ecb(_NEFORCE move(wrapper)));
        exception_ptr result;
        result.ecb_ = control_block.release();
        return result;
    } catch (...) {
        return exception_ptr();
    }
}

/**
 * @brief 获取当前异常
 * @return 当前异常的异常指针
 *
 * 捕获当前异常并创建异常指针。
 * 如果当前没有异常被捕获，返回空的异常指针。
 */
exception_ptr NEFORCE_API current_exception() noexcept;

/**
 * @brief 重新抛出异常
 * @param p 异常指针
 *
 * 重新抛出异常指针引用的异常。
 *
 * @note 如果异常指针为空，进程将被终止。
 */
void NEFORCE_API rethrow_exception(const exception_ptr& p);

/** @} */ // ExceptionHandling

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_EXCEPTION_PTR_HPP__
